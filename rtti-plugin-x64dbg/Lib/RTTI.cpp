#include "Rtti.h"
#include "CompleteObjectLocator.h"

namespace RTTI {;

constexpr size_t gScanBufSize	= 0x20000;
constexpr size_t gScanBufAlign	= 0x10000;
static AlignedBufferPtr gScanBuffer;

//// static ////////////////////////////////////////////////////////////////////////////////////////////////
void TypeInfo::ScanSection(const char *modName, duint modBase, duint secBase, size_t size)
{
	if (!gScanBuffer)
		gScanBuffer = AlignedBufferPtr((BYTE*)_aligned_malloc(gScanBufSize, gScanBufAlign));
	duint memVal = 0;
	duint pCOL = 0;
	int numVFuncs = 0;
	int numCOLs = 0;
	Log("=====================================================================================\n");
	Log("[%s:0x%p] size: 0x%X\n", modName, secBase, size);
	for (duint offset = 0; offset < size; offset += sizeof(duint)) {
		// Read mem buffer in large chunks for better performance
		const duint memAddr = secBase + offset;
		const duint bufOffset = offset % gScanBufSize;
		if (!bufOffset) {
			const duint offsetEnd = std::min(size, offset + gScanBufSize);
			const duint readSize = offsetEnd - offset;
			if (!DbgMemRead(memAddr, gScanBuffer.get(), readSize)) {
				Log("ERROR trying to read 0x%X bytes at 0x%p!\n", readSize, memAddr);
				break;
			}
		}
		if (bufOffset + sizeof(duint) > gScanBufSize) {
			Log("ERROR trying to read past end of buffer! bufOffset: 0x%X\n", bufOffset);
			break;
		}
		memVal = *reinterpret_cast<duint*>(gScanBuffer.get()+bufOffset);
		if (!memVal || !DbgMemIsValidReadPtr(memVal)) {
			if (pCOL) {
				// end of vftable
				Log("  numVFuncs: %d\n", numVFuncs);
				pCOL = 0;
			}
			continue;
		}
		TypeInfo rtti = TypeInfo::FromCompleteObjectLocatorAddr(modName, modBase, memVal, false);
		if (!rtti.IsValid()) {
			if (pCOL)
				numVFuncs++;
		} else {
			numCOLs++;
			if (pCOL) {
				// end of vftable
				Log("  numVFuncs: %d\n", numVFuncs);
			}
			pCOL = memVal;
			// vftable starts after pCOL
			const duint pvftable = memAddr + sizeof(duint);
			numVFuncs = 0;
			Log("%s\n", rtti.GetClassHierarchyString().c_str());
			Log("  pCOL: 0x%p\n", (void*)pCOL);
			Log("  pVFT: 0x%p\n", (void*)pvftable);
			// get xrefs to the pVFT, these are usually in constructors and destructors
			// FIXME: this only works if xAnalyzer has been run on this module
			/*XREF_INFO xref{0};
			if (DbgXrefGet(pvftable, &xref)) {
				Log("  pVFT xrefs:");
				for (int i = 0; i < xref.refcount; ++i)
					if (xref.references[i].type == XREF_DATA)
						LogNoTime(" 0x%p", (void *) xref.references[i].addr);
				LogNoTime("\n");
			}*/
		}
	}
	if (pCOL) {
		Log("  numVFuncs: %d\n", numVFuncs);
	}
	Log("[%s:0x%p] COLs found: %d\n", modName, secBase, numCOLs);
}
//// static ////////////////////////////////////////////////////////////////////////////////////////////////
TypeInfo TypeInfo::FromObjectThisAddr(duint addr, bool log)
{
	TypeInfo rtti;
	rtti.m_isValid = rtti.InitFromThisAddr(addr, log);
	return rtti;
}
//// static ////////////////////////////////////////////////////////////////////////////////////////////////
TypeInfo TypeInfo::FromCompleteObjectLocatorAddr(const char *modName, duint modBase, duint addr, bool log)
{
	TypeInfo rtti(modName, modBase);
	rtti.m_isValid = rtti.InitFromCOLAddr(addr, log);
	return rtti;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
TypeInfo::TypeInfo(const char *modName, duint modBase)
: m_moduleName(modName)
, m_moduleBase(modBase)
{
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
const std::string &TypeInfo::GetTypeName() const
{
	static const std::string invalid("[invalid]");
	return m_completeObjectLocator ? m_completeObjectLocator->getCompleteType()->getName() : invalid;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool TypeInfo::InitFromCOLAddr(duint addr, bool log)
{
	if (log) dprintf("=====================================================================================\n");
	if (!(m_completeObjectLocator = CompleteObjectLocator::loadFromAddr(m_moduleBase, addr, log)))
		return false;
	m_pcol = addr;
	return Init(log);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool TypeInfo::InitFromThisAddr(duint addr, bool log)
{
	if (log) dprintf("=====================================================================================\n");
	m_this = addr;
	if (!GetVFTableFromThis(log))
		return false;
	m_moduleBase = DbgFunctions()->ModBaseFromAddr(m_pvftable);
	char modName[256] = {0};
	if (DbgFunctions()->ModNameFromAddr(m_pvftable, modName, true))
		m_moduleName.assign(modName);
	if (!GetCOLFromVFTable(log))
		return false;
	return Init(log);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool TypeInfo::GetVFTableFromThis(bool log)
{
	// this usually points to pvftable, but not always
	m_ppvftable = m_this;
	// first, deref the value at this if it's a valid address
	duint vftable = 0;
	if (!DbgMemRead(m_ppvftable, &m_pvftable, sizeof(duint))) {
		if (log) dprintf("Couldn't read m_ppvftable: 0x%p.\n", (void*)m_ppvftable);
		return false;
	}
	if (!DbgMemRead(m_pvftable, &vftable, sizeof(duint))) {
		if (log) dprintf("Couldn't read m_pvftable: 0x%p.\n", (void*)m_pvftable);
		return false;
	}
	// If **this is a vbtable, the low 32bits should be zero
	// (as the first entry is the offset from vbptr to the beginning of the complete object)
	if ((vftable & 0xFFFFFFFF) == 0) {
#ifdef _WIN64
		// offset is the second entry in the vbtable
		DWORD offset = (DWORD)(vftable >> 32);
#else
		// we only read the first DWORD, we need the second for the offset
		DWORD offset = 0;
		if (!DbgMemRead(m_pvftable+sizeof(DWORD), &offset, sizeof(DWORD))) {
			if (log) dprintf("Couldn't read vbtable entry at: 0x%p.\n", (void*)(m_pvftable+sizeof(DWORD)));
			return false;
		}
#endif
		// now that we have the offset from this, we can get the pvftable
		m_ppvftable = m_this+offset;
		if (!DbgMemRead(m_ppvftable, &m_pvftable, sizeof(duint))) {
			if (log) dprintf("Couldn't read (vbtable offset) m_ppvftable: 0x%p.\n", (void*)m_ppvftable);
			return false;
		}
	}
	// todo: check that it's an addr in this module's .rdata (which is where vftables are)
	if (!DbgMemIsValidReadPtr(m_pvftable)) {
		if (log) dprintf("m_pvftable (0x%p) invalid address.\n", (void *) m_pvftable);
		return false;
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool TypeInfo::GetCOLFromVFTable(bool log)
{
	// pointer to a complete object locator is stored before start of vftable
	duint ppCompleteObjectLocator = (duint)SUBPTR(m_pvftable, sizeof(duint));
	duint pcol = 0;
	if (!DbgMemRead(ppCompleteObjectLocator, &pcol, sizeof(duint))) {
		if (log) dprintf("Couldn't read ppCompleteObjectLocator: 0x%p.\n", (void*)ppCompleteObjectLocator);
		return false;
	}
	if (!(m_completeObjectLocator = CompleteObjectLocator::loadFromAddr(m_moduleBase, pcol, log)))
		return false;
	m_pcol = pcol;
	m_completeThis = m_ppvftable - m_completeObjectLocator->getOffset();
	m_completeObjectLocator->updateFromCompleteObject(m_completeThis);
	return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool TypeInfo::Init(bool log)
{
	if (log) {
		dprintf("module base:  0x%p %s\n", (void*)m_moduleBase, m_moduleName.c_str());
		if (m_completeThis) {
			dprintf("this:         0x%p\n", (void *) m_this);
			dprintf("completeThis: 0x%p\n", (void *) m_completeThis);
			dprintf("pVFTable:     0x%p\n", (void *) m_pvftable);
		}
		m_completeObjectLocator->print();
	}
	if (m_completeThis)
		m_classHierarchyStr = "[" + AddrToStr(m_this) + "] " + m_moduleName + " ";
	m_classHierarchyStr += m_completeObjectLocator->getClassHierarchyStr();
	return true;
}

};
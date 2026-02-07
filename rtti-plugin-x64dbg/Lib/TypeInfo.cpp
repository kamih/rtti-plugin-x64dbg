#include "TypeInfo.h"
#include "CompleteObjectLocator.h"

namespace RTTI {;

constexpr size_t gScanBufSize	= 0x20000;
constexpr size_t gScanBufAlign	= 0x10000;
static AlignedBufferPtr gScanBuffer;

//// static ////////////////////////////////////////////////////////////////////////////////////////////////
void TypeInfo::scanSection(const char *modName, duint modBase, duint secBase, size_t size)
{
	if (!gScanBuffer)
		gScanBuffer = AlignedBufferPtr((BYTE*)_aligned_malloc(gScanBufSize, gScanBufAlign));
	duint memVal = 0;
	duint pCOL = 0;
	int numVFuncs = 0;
	int numCOLs = 0;
	log("=====================================================================================\n");
	log("[%s:0x%p] size: 0x%X\n", modName, secBase, size);
	for (duint offset = 0; offset < size; offset += sizeof(duint)) {
		// Read mem buffer in large chunks for better performance
		const duint memAddr = secBase + offset;
		const duint bufOffset = offset % gScanBufSize;
		if (!bufOffset) {
			const duint offsetEnd = std::min(size, offset + gScanBufSize);
			const duint readSize = offsetEnd - offset;
			if (!DbgMemRead(memAddr, gScanBuffer.get(), readSize)) {
				log("ERROR trying to read 0x%X bytes at 0x%p!\n", readSize, memAddr);
				break;
			}
		}
		if (bufOffset + sizeof(duint) > gScanBufSize) {
			log("ERROR trying to read past end of buffer! bufOffset: 0x%X\n", bufOffset);
			break;
		}
		memVal = *reinterpret_cast<duint*>(gScanBuffer.get()+bufOffset);
		if (!memVal || !DbgMemIsValidReadPtr(memVal)) {
			if (pCOL) {
				// end of vftable
				log("  numVFuncs: %d\n", numVFuncs);
				pCOL = 0;
			}
			continue;
		}
		TypeInfo rtti = TypeInfo::fromCompleteObjectLocatorAddr(modName, modBase, memVal, false);
		if (!rtti.valid()) {
			if (pCOL)
				numVFuncs++;
		} else {
			numCOLs++;
			if (pCOL) {
				// end of vftable
				log("  numVFuncs: %d\n", numVFuncs);
			}
			pCOL = memVal;
			// vftable starts after pCOL
			const duint pvftable = memAddr + sizeof(duint);
			numVFuncs = 0;
			log("%s\n", rtti.getClassHierarchyString().c_str());
			log("  pCOL: 0x%p\n", (void*)pCOL);
			log("  pVFT: 0x%p\n", (void*)pvftable);
			// get xrefs to the pVFT, these are usually in constructors and destructors
			// FIXME: this only works if xAnalyzer has been run on this module
			/*XREF_INFO xref{0};
			if (DbgXrefGet(pvftable, &xref)) {
				log("  pVFT xrefs:");
				for (int i = 0; i < xref.refcount; ++i)
					if (xref.references[i].type == XREF_DATA)
						logNoTime(" 0x%p", (void *) xref.references[i].addr);
				logNoTime("\n");
			}*/
		}
	}
	if (pCOL) {
		log("  numVFuncs: %d\n", numVFuncs);
	}
	log("[%s:0x%p] COLs found: %d\n", modName, secBase, numCOLs);
}
//// static ////////////////////////////////////////////////////////////////////////////////////////////////
TypeInfo TypeInfo::fromObjectThisAddr(duint addr, bool log)
{
	TypeInfo rtti;
	rtti.mValid = rtti.initFromThisAddr(addr, log);
	return rtti;
}
//// static ////////////////////////////////////////////////////////////////////////////////////////////////
TypeInfo TypeInfo::fromCompleteObjectLocatorAddr(const char *modName, duint modBase, duint addr, bool log)
{
	TypeInfo rtti(modName, modBase);
	rtti.mValid = rtti.initFromCOLAddr(addr, log);
	return rtti;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
TypeInfo::TypeInfo(const char *modName, duint modBase)
: mModuleName(modName)
, mModuleBase(modBase)
{
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
const std::string &TypeInfo::getTypeName() const
{
	static const std::string invalid("[invalid]");
	return mCOL ? mCOL->getCompleteType()->getName() : invalid;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool TypeInfo::initFromCOLAddr(duint addr, bool log)
{
	if (log) dprintf("=====================================================================================\n");
	if (!(mCOL = CompleteObjectLocator::loadFromAddr(mModuleBase, addr, log)))
		return false;
	mCOLp = addr;
	return initFinish(log);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool TypeInfo::initFromThisAddr(duint addr, bool log)
{
	if (log) dprintf("=====================================================================================\n");
	mThis = addr;
	if (!getVFTableFromThis(log))
		return false;
	mModuleBase = DbgFunctions()->ModBaseFromAddr(mVFTablep);
	char modName[256] = {0};
	if (DbgFunctions()->ModNameFromAddr(mVFTablep, modName, true))
		mModuleName.assign(modName);
	if (!getCOLFromVFTable(log))
		return false;
	return initFinish(log);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool TypeInfo::getVFTableFromThis(bool log)
{
	// this usually points to pvftable, but not always
	mVFtablepp = mThis;
	// first, deref the value at this if it's a valid address
	duint vftable = 0;
	if (!DbgMemRead(mVFtablepp, &mVFTablep, sizeof(duint))) {
		if (log) dprintf("Couldn't read mVFtablepp: 0x%p.\n", (void*)mVFtablepp);
		return false;
	}
	if (!DbgMemRead(mVFTablep, &vftable, sizeof(duint))) {
		if (log) dprintf("Couldn't read mVFTablep: 0x%p.\n", (void*)mVFTablep);
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
		mVFtablepp = mThis+offset;
		if (!DbgMemRead(mVFtablepp, &mVFTablep, sizeof(duint))) {
			if (log) dprintf("Couldn't read (vbtable offset) mVFtablepp: 0x%p.\n", (void*)mVFtablepp);
			return false;
		}
	}
	// todo: check that it's an addr in this module's .rdata (which is where vftables are)
	if (!DbgMemIsValidReadPtr(mVFTablep)) {
		if (log) dprintf("mVFTablep (0x%p) invalid address.\n", (void *) mVFTablep);
		return false;
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool TypeInfo::getCOLFromVFTable(bool log)
{
	// pointer to a complete object locator is stored before start of vftable
	duint ppCompleteObjectLocator = (duint)SUBPTR(mVFTablep, sizeof(duint));
	duint pcol = 0;
	if (!DbgMemRead(ppCompleteObjectLocator, &pcol, sizeof(duint))) {
		if (log) dprintf("Couldn't read ppCompleteObjectLocator: 0x%p.\n", (void*)ppCompleteObjectLocator);
		return false;
	}
	if (!(mCOL = CompleteObjectLocator::loadFromAddr(mModuleBase, pcol, log)))
		return false;
	mCOLp = pcol;
	mCompleteThis = mVFtablepp - mCOL->getOffset();
	mCOL->updateFromCompleteObject(mCompleteThis);
	return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool TypeInfo::initFinish(bool log)
{
	if (log) {
		dprintf("module base:  0x%p %s\n", (void*)mModuleBase, mModuleName.c_str());
		if (mCompleteThis) {
			dprintf("this:         0x%p\n", (void*)mThis);
			dprintf("completeThis: 0x%p\n", (void*)mCompleteThis);
			dprintf("pVFTable:     0x%p\n", (void*)mVFTablep);
		}
		mCOL->print();
	}
	if (mCompleteThis)
		mClassHierarchyStr = "[" + addrToStr(mThis) + "] " + mModuleName + " ";
	mClassHierarchyStr += mCOL->getClassHierarchyStr();
	return true;
}

};
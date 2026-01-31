#include "Rtti.h"

#define RTTI_LEA(A) ((m_completeObjectLocator.signature == 1 ? m_moduleBase : 0)+(A))

//// static ////////////////////////////////////////////////////////////////////////////////////////////////
void RTTI::ScanSection(const char *modName, duint modBase, duint secBase, size_t size)
{
	duint val = 0;
	duint pCOL = 0;
	int numVFuncs = 0;
	dprintf("=====================================================================================\n");
	dprintf("[%s:0x%p] size: 0x%X\n", modName, secBase, size);
	for (duint addr = secBase; addr < secBase + size; addr += sizeof(duint)) {
		if (!DbgMemRead(addr, &val, sizeof(duint))) {
			dprintf("ERROR trying to read 0x%p!\n", addr);
		}
		if (!val || !DbgMemIsValidReadPtr(val)) {
			if (pCOL) {
				// end of vftable
				dprintf("  numVFuncs: %d\n", numVFuncs);
				pCOL = 0;
			}
			continue;
		}
		RTTI rtti = RTTI::FromCompleteObjectLocatorAddr(modName, modBase, val, false);
		if (rtti.IsValid()) {
			if (pCOL) {
				// end of vftable
				dprintf("  numVFuncs: %d\n", numVFuncs);
			}
			pCOL = val;
			// vftable starts after pCOL
			addr += sizeof(duint);
			numVFuncs = 1;
			dprintf("%s\n", rtti.GetClassHierarchyString().c_str());
			dprintf("  pCOL: 0x%p\n", (void*)pCOL);
			dprintf("  pVFT: 0x%p\n", (void*)addr);
			// get xrefs to the pVFT, these are usually in constructors and destructors
			XREF_INFO xref{0};
			if (DbgXrefGet(addr, &xref)) {
				dprintf("  pVFT xrefs:");
				for (int i = 0; i < xref.refcount; ++i)
					if (xref.references[i].type == XREF_DATA)
						dprintf(" 0x%p", (void *) xref.references[i].addr);
				dprintf("\n");
			}
			continue;
		}
		if (pCOL)
			numVFuncs++;
	}
}
//// static ////////////////////////////////////////////////////////////////////////////////////////////////
RTTI RTTI::FromObjectThisAddr(duint addr, bool log)
{
	RTTI rtti;
	rtti.m_isValid = rtti.InitFromThisAddr(addr, log);
	return rtti;
}
//// static ////////////////////////////////////////////////////////////////////////////////////////////////
RTTI RTTI::FromCompleteObjectLocatorAddr(const char *modName, duint modBase, duint addr, bool log)
{
	RTTI rtti(modName, modBase);
	rtti.m_isValid = rtti.InitFromCOLAddr(addr, log);
	return rtti;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
RTTI::RTTI(const char *modName, duint modBase)
: m_moduleName(modName)
, m_moduleBase(modBase)
{
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool RTTI::InitFromThisAddr(duint addr, bool log)
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
bool RTTI::InitFromCOLAddr(duint addr, bool log)
{
	if (log) dprintf("=====================================================================================\n");
	if (!GetCOLFromAddr(addr, log))
		return false;
	return Init(log);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool RTTI::Init(bool log)
{
	// Parse TypeDescriptor
	if (!m_typeDescriptor.load(RTTI_LEA(m_completeObjectLocator.pTypeDescriptor))) {
		if (log) dprintf("Couldn't parse the TypeDescriptor.\n");
		return false;
	}
	// Parse ClassHierarchyDescriptor
	if (!GetCHDFromCOL(log))
		return false;

	if (log) {
		dprintf("module base:  0x%p %s\n", (void*)m_moduleBase, m_moduleName.c_str());
		dprintf("pCOL:         0x%p\n", (void*)m_pcol);
		if (m_this) {
			dprintf("this:         0x%p\n", (void *) m_this);
			dprintf("completeThis: 0x%p\n", (void *) m_completeThis);
			dprintf("pVFTable:     0x%p\n", (void *) m_pvftable);
		}		
		m_completeObjectLocator.print(m_moduleBase);
		m_typeDescriptor.print();
		m_classHierarchyDescriptor.print(m_moduleBase);
	}
	// Parse all BaseClassDescriptors
	if (!GetBaseClassesFromCHD(log))
		return false;

	InitClassHierarchyTree();
	return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool RTTI::GetVFTableFromThis(bool log)
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
bool RTTI::GetCOLFromVFTable(bool log)
{
	// pointer to a complete object locator is stored before start of vftable
	duint ppCompleteObjectLocator = (duint)SUBPTR(m_pvftable, sizeof(duint));
	duint pcol = 0;
	if (!DbgMemRead(ppCompleteObjectLocator, &pcol, sizeof(duint))) {
		if (log) dprintf("Couldn't read ppCompleteObjectLocator: 0x%p.\n", (void*)ppCompleteObjectLocator);
		return false;
	}
	if (!GetCOLFromAddr(pcol, log))
		return false;
	m_completeThis = m_ppvftable - m_completeObjectLocator.offset;
	return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool RTTI::GetCOLFromAddr(duint addr, bool log)
{
	// Read the RTTICompleteObjectLocator
	if (!m_completeObjectLocator.load(addr)) {
		if (log) dprintf("Couldn't read m_completeObjectLocator from: 0x%p.\n", (void*)addr);
		return false;
	}
	// Check the signature (should be 0 for 32bit, 1 for 64bit)
	if (m_completeObjectLocator.signature > 1) {
		if (log) dprintf("Unexpected RTTICompleteObjectLocator.signature: %d\n", m_completeObjectLocator.signature);
		return false;
	}
	// Verify that self is correct
	// (for 64bit modules, pSelf is an offset from the module base to the COL)
	if (m_completeObjectLocator.signature == 1 &&
		m_moduleBase != (duint)SUBPTR(addr, m_completeObjectLocator.pSelf)) {
		if (log) dprintf("Unexpected RTTICompleteObjectLocator.pSelf: 0x%X\n", m_completeObjectLocator.pSelf);
		return false;
	}
	m_pcol = addr;
	return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Get ClassHierarchyDescriptor from CompleteObjectLocator
bool RTTI::GetCHDFromCOL(bool log)
{
	if (!m_classHierarchyDescriptor.load(RTTI_LEA(m_completeObjectLocator.pClassHierarchyDescriptor))) {
		if (log) dprintf("Couldn't parse the ClassHierarchyDescriptor.\n");
		return false;
	}
	if (m_classHierarchyDescriptor.signature != 0) {
		if (log) dprintf("Unexpected RTTIClassHierarchyDescriptor.signature: %d\n", m_classHierarchyDescriptor.signature);
		return false;
	}
	if (m_classHierarchyDescriptor.numBaseClasses == 0) {
		if (log) dprintf("Unexpected RTTIClassHierarchyDescriptor.numBaseClasses: 0\n");
		return false;
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool RTTI::GetBaseClassesFromCHD(bool log)
{
	const unsigned int numBaseClasses = m_classHierarchyDescriptor.numBaseClasses;
	// Read the pointer/offset array
	std::vector<DWORD> pBaseClass(numBaseClasses, 0);
	const duint pBaseClassArray = RTTI_LEA(m_classHierarchyDescriptor.pBaseClassArray);
	if (!DbgMemRead(pBaseClassArray, (void*)pBaseClass.data(), sizeof(DWORD)*numBaseClasses)) {
		if (log) dprintf("Couldn't read pBaseClassArray: 0x%p.\n", (void*)pBaseClassArray);
		return false;
	}
	m_baseClassDescriptors.resize(numBaseClasses);
	m_baseClassTypeDescriptors.resize(numBaseClasses);
	m_baseClassOffsets.resize(numBaseClasses);
	m_typeNames.resize(numBaseClasses);
	// For each of the numBaseClasses populate the BaseClassDescriptors
	for (unsigned int i = 0; i < numBaseClasses; ++i)
	{
		// Copy in the BaseClassDescriptor
		duint addr = RTTI_LEA(pBaseClass[i]);
		if (!m_baseClassDescriptors[i].load(addr)) {
			if (log) dprintf("Couldn't load m_baseClassDescriptors[%d] from: 0x%p.\n", i, (void*)addr);
			return false;
		}
		auto &baseClass = m_baseClassDescriptors[i];
		// Copy in the BaseClassTypeDescriptor
		addr = RTTI_LEA(baseClass.pTypeDescriptor);
		if (!m_baseClassTypeDescriptors[i].load(addr)) {
			if (log) dprintf("Couldn't load m_baseClassTypeDescriptors[%d] from: 0x%p.\n", i, (void*)addr);
			return false;
		}
		m_typeNames[i] = Demangle(m_baseClassTypeDescriptors[i].sz_decorated_name, i > 0);
		if (log) baseClass.print(m_moduleBase, m_typeNames[i], i, numBaseClasses);
		const int mdisp = baseClass.where.mdisp;
		const int pdisp = baseClass.where.pdisp;
		const int vdisp = baseClass.where.vdisp;
		// Save each offset so we can display to the user where it is inside the class
		m_baseClassOffsets[i] = mdisp;
		if (!m_completeThis) {
			// If we're building the hierarchy without an object,
			// we can't find the vbtable, so can't compute vbtable based object offset
			if (pdisp != -1)
				// set offset to -1 so we can print a ? for it
				m_baseClassOffsets[i] = -1;
			continue;
		}
		// Calc vbtable based object offset
		if (pdisp != -1) {
			const duint pp_vbtable = (duint)ADDPTR(m_completeThis, pdisp);
			duint p_vbtable = 0;
			if (log) dprintf("        pp_vbtable: 0x%p\n", (void*)pp_vbtable);
			// read the vbtable addr
			if (!DbgMemRead(pp_vbtable, &p_vbtable, sizeof(duint))) {
				if (log) dprintf("        Problem reading p_vbtable.\n");
				continue;
			}
			const duint p_offset = (duint)ADDPTR(p_vbtable, vdisp);
			if (log) dprintf("        p_vbtable:  0x%p\n", (void *) p_vbtable);
			int offset = 0;
			if (!DbgMemRead(p_offset, &offset, sizeof(int))) {
				if (log) dprintf("        Problem reading offset.\n");
				continue;
			}
			m_baseClassOffsets[i] += pdisp + offset;
			if (log) dprintf("        offset:     +0x%X\n", m_baseClassOffsets[i]);
		}
		// If this is the index of the base class this points to, save it
		if (m_baseClassOffsets[i] > 0 &&
			m_completeThis + m_baseClassOffsets[i] == m_this)
			m_thisTypeIndex = i;
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void RTTI::InitClassHierarchyTree()
{
	// Create the class hierarchy tree
	m_classHierarchyTree = std::make_unique<CHTreeNode>(0);
	BuildClassHierarchyTree(1, m_classHierarchyDescriptor.numBaseClasses, m_classHierarchyTree);
	// Build full class hierarchy string
	std::ostringstream sstr;
	if (m_this)
		sstr << "[" << AddrToStr(m_this) << "] " << m_moduleName << " ";
	if (m_thisTypeIndex) {
		auto thisClass = Demangle(m_baseClassTypeDescriptors[m_thisTypeIndex].sz_decorated_name);
		sstr << "(" << thisClass << "*) ";
	} else if (m_completeObjectLocator.offset) {
		sstr << "(+";
		sstr << std::uppercase << std::hex << m_completeObjectLocator.offset;
		sstr << ") ";
	}
	sstr << m_typeNames[0];
	CHTreeToString(sstr, m_classHierarchyTree);
	m_classHierarchyStr = sstr.str();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void RTTI::BuildClassHierarchyTree(unsigned int startIdx, unsigned int endIdx, const CHTreeNodePtr &rootNode)
{
	for (unsigned int i = startIdx; i < endIdx; ++i) {
		auto baseClassNode = std::make_unique<CHTreeNode>(i);
		auto &baseClass = m_baseClassDescriptors[i];
		if (baseClass.numContainedBases > 0) {
			BuildClassHierarchyTree(i + 1, i + 1 + baseClass.numContainedBases, baseClassNode);
			i += baseClass.numContainedBases;
		}
		rootNode->baseClasses.push_back(std::move(baseClassNode));
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void RTTI::CHTreeToString(std::ostringstream &sstr, const CHTreeNodePtr &pnode) const
{
	const auto node = pnode.get();
	if (!node)
		return;
	if (!node->baseClasses.empty())
		sstr << ": ";
	for (auto &child : node->baseClasses) {
		if (child != *node->baseClasses.begin())
			sstr << ", ";
		auto idx = child->index;
		auto &classDesc = m_baseClassDescriptors[idx];
		if (classDesc.attributes & BCD_VBOFCONTOBJ)
			sstr << "virtual ";
		sstr << m_typeNames[idx];
		if (m_baseClassOffsets[idx]) {			
			// -1: unknown vbtable offsets
			if (m_baseClassOffsets[idx] == -1) {
				sstr << "(+?)";
			} else {
				sstr << "(+";
				sstr << std::uppercase << std::hex << m_baseClassOffsets[idx];
				sstr << ")";
			}
		}
		if (!child->baseClasses.empty()) {
			sstr << "[";
			CHTreeToString(sstr, child);
			sstr << "]";
		}
	}
}
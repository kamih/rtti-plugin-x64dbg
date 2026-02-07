#include "CompleteObjectLocator.h"
#include <map>

namespace RTTI {;

static std::map<duint, CompleteObjectLocator*> gCOLMap; // debugged process mem addr -> our COL

////////////////////////////////////////////////////////////////////////////////////////////////////////////
CompleteObjectLocator::CompleteObjectLocator(duint addr,
											 const _CompleteObjectLocator &col,
											 TypeDescriptor *pTD,
											 ClassHierarchyDescriptor *pCHD)
: mAddr(addr)
, mOffset(col.offset)
, mCDOffset(col.cdOffset)
, mCompleteType(pTD)
, mBaseType(nullptr)
, mClassHierarchyDesc(pCHD)
{
	updateBaseType();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CompleteObjectLocator::updateBaseType()
{
	mBaseType = mClassHierarchyDesc->findBaseTypeForOffset(mOffset);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CompleteObjectLocator::updateFromCompleteObject(duint completeObjAddr)
{
	// With the complete object address, we can find vbtables and compute all base class offsets
	mClassHierarchyDesc->updateOffsetsFromCompleteObject(completeObjAddr);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void CompleteObjectLocator::print() const
{
	dprintf("CompleteObjectLocator:\n");
	dprintf("    addr:                  0x%p\n", (void*)mAddr);
	dprintf("    offset:                +0x%X (from complete obj addr to ppvftable)\n", mOffset);
	dprintf("    cdOffset:              +0x%X\n", mCDOffset);
	mCompleteType->print();
	mClassHierarchyDesc->print();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::string CompleteObjectLocator::getClassHierarchyStr() const
{
	std::ostringstream sstr;
	if (mBaseType) {
		if (mBaseType != mCompleteType)
			sstr << "(" << mBaseType->getName() << "*) ";
	} else if (mOffset) {
		sstr << "(+";
		sstr << std::uppercase << std::hex << mOffset;
		sstr << ") ";
	}
	sstr << mCompleteType->getName();
	return sstr.str() + mClassHierarchyDesc->toString();
}
//// static ////////////////////////////////////////////////////////////////////////////////////////////////
CompleteObjectLocator *CompleteObjectLocator::loadFromAddr(duint modBase, duint addr, bool log)
{
	// First check if we have this in our map
	auto it = gCOLMap.find(addr);
	if (it != gCOLMap.end())
		return it->second;
	// Read the _CompleteObjectLocator from debugged process memory
	_CompleteObjectLocator col;
	if (!DbgMemRead(addr, (void*)&col, sizeof(_CompleteObjectLocator))) {
		if (log) dprintf("Couldn't read _CompleteObjectLocator from: 0x%p\n", (void*)addr);
		return nullptr;
	}
	// Check the signature (should be 0 for 32bit, 1 for 64bit)
	if (col.signature > 1) {
		if (log) dprintf("Unexpected _CompleteObjectLocator.signature: %d\n", col.signature);
		return nullptr;
	}
	// Verify that self is correct
	// (for 64bit modules, pSelf is an offset from the module base to the COL)
	if (col.signature == 1 &&
		modBase != (duint)SUBPTR(addr, col.pSelf)) {
		if (log) dprintf("Unexpected _CompleteObjectLocator.pSelf: 0x%X\n", col.pSelf);
		return nullptr;
	}
	const duint addrOffset = col.signature == 1 ? modBase : 0;
	// Load TypeDescriptor
	const duint pTD = col.pTypeDescriptor + addrOffset;
	TypeDescriptor *typeDesc = TypeDescriptor::loadFromAddr(pTD, log);
	if (!typeDesc)
		return nullptr;
	// Load CHD
	const duint pCHD = col.pClassHierarchyDescriptor + addrOffset;
	ClassHierarchyDescriptor *chd = ClassHierarchyDescriptor::loadFromAddr(addrOffset, pCHD, log);
	if (!chd)
		return nullptr;
	// Create COL and add it to our map
	auto *c = new CompleteObjectLocator(addr, col, typeDesc, chd);
	chd->addCOLRef(c);
	gCOLMap[addr] = c;
	return c;
}

};
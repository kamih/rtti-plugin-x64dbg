#include "BaseClassDescriptor.h"
#include "ClassHierarchyDescriptor.h"
#include <map>

namespace RTTI {;

static std::map<duint, BaseClassDescriptor*> gBCDMap; // debugged process mem addr -> our BCD

////////////////////////////////////////////////////////////////////////////////////////////////////////////
BaseClassDescriptor::BaseClassDescriptor(duint addr,
										 const _BaseClassDescriptor &bcd,
										 TypeDescriptor *td,
										 ClassHierarchyDescriptor *chd)
: mAddr(addr)
, mNumContainedBases(bcd.numContainedBases)
, mWhere(bcd.where)
, mAttributes(bcd.attributes)
, mTypeDesc(td)
, mClassHierarchyDesc(chd)
{
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
// returns -1 if offset can't be determined
int BaseClassDescriptor::calcOffsetFromCompleteObj(duint objAddr) const
{
	const int mdisp = mWhere.mdisp;
	const int pdisp = mWhere.pdisp;
	const int vdisp = mWhere.vdisp;
	if (pdisp == -1)
		return mdisp;
	// We need obj addr to find vbtable
	if (!objAddr)
		return -1;
	// Calc vbtable based object offset
	const duint pp_vbtable = objAddr + mdisp + pdisp;
	duint p_vbtable = 0;
	// read the vbtable addr
	if (!DbgMemRead(pp_vbtable, &p_vbtable, sizeof(duint))) {
		dprintf("        Problem reading p_vbtable from: 0x%p\n", (void*)pp_vbtable);
		return -1;
	}
	// read the vbtable at the vdisp offset
	const duint p_offset = (duint)ADDPTR(p_vbtable, vdisp);
	int offset = 0;
	if (!DbgMemRead(p_offset, &offset, sizeof(int))) {
		dprintf("        Problem reading vbtable[%d]: 0x%p\n", vdisp/sizeof(DWORD), (void*)p_offset);
		return -1;
	}
	return mdisp + pdisp + offset;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void BaseClassDescriptor::print(int index, int count, int offset) const
{
	std::string attribStr = "";
	if (mAttributes & BCD_PRIVORPROTBASE)
		attribStr += " Private";
	if (mAttributes & BCD_NOTVISIBLE)
		attribStr += " NotVisible";
	else
		attribStr += " Public";
	if (mAttributes & BCD_VBOFCONTOBJ)
		attribStr += " Virtual";
	if (mAttributes & BCD_HASPCHD)
		attribStr += " HasCHD";
	if (mAttributes & BCD_AMBIGUOUS)
		attribStr += " Ambiguous";
	if (mAttributes & BCD_NONPOLYMORPHIC)
		attribStr += " NonPoly";

	dprintf("    BaseClassDescriptor %d/%d:\n", index, count);
	dprintf("        addr:              0x%p\n", (void*)mAddr);
	dprintf("        typeName:          '%s'\n", mTypeDesc->getName().c_str());
	dprintf("        attributes:        0x%X%s\n", mAttributes,	attribStr.c_str());
	dprintf("        numContainedBases: %d\n", mNumContainedBases);
	dprintf("        pmd:               (%d, %d, %d)\n", mWhere.mdisp, mWhere.pdisp, mWhere.vdisp);
	if (offset == -1)
		dprintf("        offset:            unknown (vbtable based)\n");
	else
		dprintf("        offset:            +0x%X (from complete object to this pp_vftable)\n", offset);

}
//// static ////////////////////////////////////////////////////////////////////////////////////////////////
BaseClassDescriptor *BaseClassDescriptor::loadFromAddr(duint modBase, duint addr, bool log)
{
	// First check if we have this in our map
	auto it = gBCDMap.find(addr);
	if (it != gBCDMap.end())
		return it->second;
	_BaseClassDescriptor bcd;
	if (!DbgMemRead(addr, (void*)&bcd, sizeof(_BaseClassDescriptor))) {
		if (log) dprintf("Couldn't load _BaseClassDescriptor from: 0x%p\n", (void*)addr);
		return nullptr;
	}
	// Load TypeDescriptor
	const duint tdAddr = modBase + bcd.pTypeDescriptor;
	auto *td = TypeDescriptor::loadFromAddr(tdAddr, log);
	if (!td) {
		if (log) dprintf("Couldn't load TypeDescriptor from: 0x%p\n", (void*)tdAddr);
		return nullptr;
	}
	// Load ClassHierarchyDescriptor if present
	// FIXME: do we actually ever need this?
	ClassHierarchyDescriptor *chd = nullptr;
	/*if (bcd.attributes & BCD_HASPCHD) {
		const duint chdAddr = modBase + bcd.pClassHierarchyDescriptor;
		chd = ClassHierarchyDescriptor::loadFromAddr(modBase, chdAddr, log);
	}*/
	// Create new BaseClassDescriptor and add it to our map
	auto *b = new BaseClassDescriptor(addr, bcd, td, chd);
	gBCDMap[addr] = b;
	return b;
}

};
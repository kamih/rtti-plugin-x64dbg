#include "RTINFO.h"

#ifndef LongToPtr
#define LongToPtr(l)   ((void*)(LONG_PTR)((long)l))
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////
void RTTICompleteObjectLocator::print(duint base) const
{
	dprintf("CompleteObjectLocator:\n");
	dprintf("    signature:                 0x%X%s\n", signature, signature == 1 ? " (image base relative offsets)" : "");
	dprintf("    offset:                    +0x%X (from complete obj addr to ppvftable)\n", offset);
	dprintf("    cdOffset:                  +0x%X\n", cdOffset);
	if (signature == 1) {
		dprintf("    pTypeDescriptor:           +0x%X -> 0x%p\n", pTypeDescriptor, (void*)(base + pTypeDescriptor));
		dprintf("    pClassHierarchyDescriptor: +0x%X -> 0x%p\n", pClassHierarchyDescriptor, (void*)(base + pClassHierarchyDescriptor));
		dprintf("    pSelf:                     +0x%X -> 0x%p\n", pSelf, (void*)(base + pSelf));
	} else {
		dprintf("    pTypeDescriptor:           0x%p\n", LongToPtr(pTypeDescriptor));
		dprintf("    pClassHierarchyDescriptor: 0x%p\n", LongToPtr(pClassHierarchyDescriptor));
		dprintf("    pSelf:                     0x%p\n", LongToPtr(pSelf));
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool RTTICompleteObjectLocator::load(duint addr)
{
	return DbgMemRead(addr, (void*)this, sizeof(RTTICompleteObjectLocator));
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void PMD::print() const
{
	dprintf("        pmd:               (%d, %d, %d)\n", mdisp, pdisp, vdisp);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void TypeDescriptor::print() const
{
	dprintf("TypeDescriptor:\n");
	dprintf("    pVFTable:                  0x%p (&type_info::vftable)\n", (void*)pVFTable);
	dprintf("    decorated_name:            %s\n", sz_decorated_name);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool TypeDescriptor::load(duint addr)
{
	return DbgMemRead(addr, (void*)this, sizeof(TypeDescriptor));
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void RTTIBaseClassDescriptor::print(duint base, const std::string &name, int index, int count) const
{
	std::string attribStr = "";
	if (attributes & BCD_PRIVORPROTBASE)
		attribStr += " Private";
	if (attributes & BCD_NOTVISIBLE)
		attribStr += " NotVisible";
	else
		attribStr += " Public";
	if (attributes & BCD_VBOFCONTOBJ)
		attribStr += " Virtual";
	if (attributes & BCD_HASPCHD)
		attribStr += " HasCHD";
	if (attributes & BCD_AMBIGUOUS)
		attribStr += " Ambiguous";
	if (attributes & BCD_NONPOLYMORPHIC)
		attribStr += " NonPoly";

	dprintf("    BaseClassDescriptor %d/%d:\n", index, count);
	dprintf("        typeName:          '%s'\n", name.c_str());
	dprintf("        attributes:        0x%X%s\n", attributes,	attribStr.c_str());
	dprintf("        numContainedBases: %d\n", numContainedBases);
	where.print();
#ifdef _WIN64
	dprintf("        pTypeDescriptor:           +0x%X -> 0x%p\n", pTypeDescriptor, (void*)(base+pTypeDescriptor));
	dprintf("        pClassHierarchyDescriptor: +0x%X -> 0x%p\n", pClassHierarchyDescriptor, (void*)(base+pClassHierarchyDescriptor));
#else
	dprintf("        pTypeDescriptor:           0x%p\n", LongToPtr(pTypeDescriptor));
	dprintf("        pClassHierarchyDescriptor: 0x%p\n", LongToPtr(pClassHierarchyDescriptor));
#endif
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool RTTIBaseClassDescriptor::load(duint addr)
{
	return DbgMemRead(addr, (void*)this, sizeof(RTTIBaseClassDescriptor));
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void RTTIClassHierarchyDescriptor::print(duint base) const
{
	std::string attribStr = "";
	if (attributes & CHD_MULTINH)
		attribStr += " Multiple";
	if (attributes & CHD_VIRTINH)
		attribStr += " Virtual";
	if (attributes & CHD_AMBIGUOUS)
		attribStr += " Ambiguous";
	dprintf("ClassHierarchyDescriptor:\n");
	dprintf("    signature:                 0x%X\n", signature);
	dprintf("    attributes:                0x%X%s\n", attributes, attribStr.c_str());
	dprintf("    numBaseClasses:            %d\n", numBaseClasses);
#ifdef _WIN64
	dprintf("    pBaseClassArray:           +0x%X -> 0x%p\n", pBaseClassArray, (void*)(base+pBaseClassArray));
#else
	dprintf("    pBaseClassArray:           0x%p\n", LongToPtr(pBaseClassArray));
#endif
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool RTTIClassHierarchyDescriptor::load(duint addr)
{
	return DbgMemRead(addr, (void*)this, sizeof(RTTIClassHierarchyDescriptor));
}


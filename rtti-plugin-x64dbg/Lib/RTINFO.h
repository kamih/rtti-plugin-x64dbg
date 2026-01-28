#pragma once
#include "Utils.h"

// RTTIClassHierarchyDescriptor attrib flags
constexpr auto CHD_MULTINH =	0x00000001;
constexpr auto CHD_VIRTINH =	0x00000002;
constexpr auto CHD_AMBIGUOUS =	0x00000004;

// RTTIBaseClassDescriptor attrib flags
constexpr auto BCD_NOTVISIBLE =				0x00000001; // current base class is not inherited publicly
constexpr auto BCD_AMBIGUOUS =				0x00000002; // current base class is ambiguous in the class hierarchy
constexpr auto BCD_PRIVORPROTBASE =			0x00000004; // current base class is inherited privately
constexpr auto BCD_PRIVORPROTINCOMPOBJ =	0x00000008; // part of a privately inherited base class hierarchy
constexpr auto BCD_VBOFCONTOBJ =			0x00000010; // current base class is virtually inherited
constexpr auto BCD_NONPOLYMORPHIC =			0x00000020;
constexpr auto BCD_HASPCHD =				0x00000040;	// pClassHierarchyDescriptor field is present

////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct RTTICompleteObjectLocator
{
	DWORD	signature;					// 32bit: 0, 64bit: 1
	DWORD	offset;						// offset from the complete object to the current sub-object from which we’ve taken RTTICompleteObjectLocator
	DWORD	cdOffset;					// constructor displacement offset
	DWORD	pTypeDescriptor;			// sig0: pointer, sig1: Image relative offset of TypeDescriptor
	DWORD	pClassHierarchyDescriptor;	// sig0: pointer, sig1: Image relative offset of RTTIClassHierarchyDescriptor
	DWORD	pSelf;						// sig0: pointer, sig1: Image relative offset of this object

	void print(duint base) const;
	bool load(duint addr);
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct PMD
{
	// A vbtable (virtual base class table) is generated for multiple virtual inheritance.
	// Because it is sometimes necessary to upclass (casting to base classes), the exact location of
	// the base class needs to be determined.

	int mdisp;  // member displacement, vftable offset (if PMD.pdisp is -1)
	int pdisp;  // vbtable displacement, vbtable offset (-1: vftable is at displacement PMD.mdisp inside the class)
	int vdisp;  // displacement inside vbtable, displacement of the base class vftable pointer inside the vbtable

	void print() const;
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct TypeDescriptor
{
	duint	pVFTable;				// Always points to the type_info vftable
	duint	spare;					// Reserved for future use
	char	dot;					// Skip the period before the mangled name
	char	sz_decorated_name[256];	// The start of the decorated name of the type (?); 0 terminated

	void print() const;
	bool load(duint addr);
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct RTTIBaseClassDescriptor
{
	DWORD	pTypeDescriptor;			// sig0: pointer, sig1: Image relative offset of TypeDescriptor
	DWORD	numContainedBases;			// number of nested classes following in the Base Class Array
	PMD		where;						// pointer-to-member displacement info
	DWORD	attributes;					// bit flags
	DWORD	pClassHierarchyDescriptor;	// sig0: pointer, sig1: Image relative offset of RTTIClassHierarchyDescriptor

	void print(duint base, const std::string &name, int index, int count) const;
	bool load(duint addr);
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct RTTIClassHierarchyDescriptor
{
	DWORD	signature;			// currently always zero
	DWORD	attributes;			// bit flags
	DWORD	numBaseClasses;		// number of classes in pBaseClassArray
	DWORD	pBaseClassArray;	// sig0: pointer, sig1: Image relative offset of RTTIBaseClassArray
								// Index 0 of this array is always 'this' class first
	void print(duint base) const;
	bool load(duint addr);
};

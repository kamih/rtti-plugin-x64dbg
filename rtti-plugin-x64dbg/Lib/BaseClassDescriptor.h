#pragma once
#include "RTINFO.h"
#include "TypeDescriptor.h"

namespace RTTI {;

class ClassHierarchyDescriptor;

////////////////////////////////////////////////////////////////////////////////////////////////////////////
class BaseClassDescriptor
{	
public:
	static BaseClassDescriptor *loadFromAddr(duint modBase, duint addr, bool log);
	void print(int index, int count, int offset) const;
	int calcOffsetFromCompleteObj(duint objAddr) const;
	unsigned int getNumContainedBases() const { return mNumContainedBases; }
	TypeDescriptor *getTypeDesc() const { return mTypeDesc; }
	DWORD hasAttribute(DWORD a) const { return mAttributes & a; }

private:
	BaseClassDescriptor(duint addr, const _BaseClassDescriptor &bcd, TypeDescriptor *td, ClassHierarchyDescriptor *chd);

	duint			mAddr;					// virtual address in process mem of this _BaseClassDescriptor
	unsigned int	mNumContainedBases;		// number of nested classes following in the Base Class Array
	_PMD			mWhere;					// pointer-to-member displacement info
	DWORD			mAttributes;			// bit flags
	TypeDescriptor	*mTypeDesc;
	ClassHierarchyDescriptor *mClassHierarchyDesc;	
};

};
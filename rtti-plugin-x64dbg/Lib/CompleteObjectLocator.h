#pragma once
#include "RTINFO.h"
#include "ClassHierarchyDescriptor.h"

namespace RTTI {;

////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CompleteObjectLocator
{
public:
	static CompleteObjectLocator *loadFromAddr(duint modBase, duint addr, bool log);
	void updateFromCompleteObject(duint completeObjAddr);
	void updateBaseType();
	void print() const;
	std::string getClassHierarchyStr() const;
	TypeDescriptor *getCompleteType() const { return mCompleteType; }
	TypeDescriptor *getBaseType() const { return mBaseType; }
	DWORD getOffset() const { return mOffset; }

private:
	CompleteObjectLocator(duint addr, const _CompleteObjectLocator &col, TypeDescriptor *pTD, ClassHierarchyDescriptor *pCHD);

	duint	mAddr;					// virtual address in process mem of this _CompleteObjectLocator
	DWORD	mOffset;				// offset from the complete object to the current sub-object from which we've taken _CompleteObjectLocator
	DWORD	mCDOffset;				// constructor displacement offset
	TypeDescriptor *mCompleteType;	// type of complete object this COL is for (set from _CompleteObjectLocator)
	TypeDescriptor *mBaseType;		// type of base object this COL is for (determined at runtime)
	ClassHierarchyDescriptor *mClassHierarchyDesc;
};

};
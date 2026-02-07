#pragma once
#include "Utils.h"
#include <vector>

class TypeInfo;

////////////////////////////////////////////////////////////////////////////////////////////////////////////
class VFTableInfo
{
public:
	duint		mAddr = 0;			// virtual address of vftable
	DWORD		mOffset = 0;		// offset from complete object to &mAddr
	DWORD		mNumFuncs = 0;		// number of virtual functions in table
	TypeInfo	*mType = nullptr;	// base type (if known)
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TypeInfo
{
public:

	duint		mAddr = 0;					// virtual address of TypeDescriptor
	std::string	mDecoratedName;				// decorated type name
	std::string	mName;						// undecorated type name
	std::vector<VFTableInfo*> mVFTables;	// array of vftables for base types of this type
};
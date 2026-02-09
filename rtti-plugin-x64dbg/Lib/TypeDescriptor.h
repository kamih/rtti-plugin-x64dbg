#pragma once
#include "RTINFO.h"

namespace RTTI {;

////////////////////////////////////////////////////////////////////////////////////////////////////////////
class TypeDescriptor
{
public:
	static TypeDescriptor *loadFromAddr(duint addr, bool log);
	void print() const;
	const std::string &getName() const { return mName; }

private:
	TypeDescriptor(duint addr, const _TypeDescriptor &td);

	duint	mAddr;				// virtual address in process mem of this _TypeDescriptor
	duint	mVFTableAddr;		// virtual address in process mem of the type_info vftable
	std::string	mDecoratedName;	// decorated name of the type (starts with '.?')
	std::string mName;			// undecorated name of the type
};

};
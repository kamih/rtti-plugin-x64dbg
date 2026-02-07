#include "TypeDescriptor.h"
#include <map>

namespace RTTI {;

static std::map<duint, TypeDescriptor*> gTDMap; // debugged process mem addr -> our TD

////////////////////////////////////////////////////////////////////////////////////////////////////////////
TypeDescriptor::TypeDescriptor(duint addr, const _TypeDescriptor &td)
: mAddr(addr)
, mVFTableAddr(td.pVFTable)
, mDecoratedName(td.sz_decorated_name)
, mName(Demangle(td.sz_decorated_name, true))
{
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
void TypeDescriptor::print() const
{
	dprintf("TypeDescriptor:\n");
	dprintf("    addr:                  0x%p\n", (void*)mAddr);
	dprintf("    pVFTable:              0x%p (&type_info::vftable)\n", (void*)mVFTableAddr);
	dprintf("    decoratedName:         %s\n", mDecoratedName.c_str());
	dprintf("    name:                  %s\n", mName.c_str());
}
//// static ////////////////////////////////////////////////////////////////////////////////////////////////
TypeDescriptor *TypeDescriptor::loadFromAddr(duint addr, bool log)
{
	// First check if we have this in our map
	auto it = gTDMap.find(addr);
	if (it != gTDMap.end())
		return it->second;
	_TypeDescriptor td;
	if (!DbgMemRead(addr, (void*)&td, sizeof(_TypeDescriptor))) {
		if (log) dprintf("Couldn't load the _TypeDescriptor from: 0x%p\n", addr);
		return nullptr;
	}
	if (td.sz_decorated_name[0] != '.' || td.sz_decorated_name[1] != '?') {
		if (log) dprintf("Unexpected start of _TypeDescriptor decorated name: %X%X\n", td.sz_decorated_name[0], td.sz_decorated_name[1]);
		return nullptr;
	}
	if (!DbgMemIsValidReadPtr(td.pVFTable)) {
		if (log) dprintf("Unexpected _TypeDescriptor.pVFTable: 0x%X\n", td.pVFTable);
		return nullptr;
	}
	// create our TypeDescriptor and add it to the map
	auto *typeDesc = new TypeDescriptor(addr, td);
	gTDMap[addr] = typeDesc;
	return typeDesc;
}

};
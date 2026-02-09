#include "TypeDescriptor.h"
#include <map>

namespace RTTI {;

constexpr int gMaxDecoratedNameSz = 2048;
static std::map<duint, TypeDescriptor*> gTDMap; // debugged process mem addr -> our TD

////////////////////////////////////////////////////////////////////////////////////////////////////////////
TypeDescriptor::TypeDescriptor(duint addr, const _TypeDescriptor &td)
: mAddr(addr)
, mVFTableAddr(td.pVFTable)
{
	// read the null terminated decorated name string
	mDecoratedName = "?";
	const duint strAddr = addr + offsetof(_TypeDescriptor, sz_decorated_name) + 2;
	const duint endAddr = strAddr + gMaxDecoratedNameSz;
	char c;
	for (duint a = strAddr; (a < endAddr) && DbgMemRead(a, (void*)&c, 1) && c; ++a)
		mDecoratedName += c;
	mName = demangle(mDecoratedName.c_str(), true);
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
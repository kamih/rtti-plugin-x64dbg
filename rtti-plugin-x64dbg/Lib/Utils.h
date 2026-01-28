#pragma once
#include "../plugin.h"
#include <string>

template<typename T1, typename T2>
constexpr auto ADDPTR(T1 ptr, T2  add) { return (void*)(((char*)(ptr) + size_t(add))); }

template<typename T1, typename T2>
constexpr auto SUBPTR(T1 ptr, T2  sub) { return (void*)(((char*)(ptr) - size_t(sub))); }

template<typename T1, typename T2>
constexpr auto CHECK_BIT(T1 var, T2 pos) { return ((var) & (1<<(pos))); }

bool DbgDerefMemRead(duint addr, void *dest, duint size);
std::string Demangle(const char *sz_name, bool noECSU = false);
std::string AddrToStr(duint addr, bool pad = true);
#pragma once
#include "../plugin.h"
#include <string>
#include <memory>

namespace RTTI {;

template<typename T1, typename T2>
constexpr auto ADDPTR(T1 ptr, T2  add) { return (void*)(((char*)(ptr) + size_t(add))); }

template<typename T1, typename T2>
constexpr auto SUBPTR(T1 ptr, T2  sub) { return (void*)(((char*)(ptr) - size_t(sub))); }

template<typename T1, typename T2>
constexpr auto CHECK_BIT(T1 var, T2 pos) { return ((var) & (1<<(pos))); }

namespace {
	struct AlignedFreeDeleter {
		void operator()(BYTE *ptr) const { if (ptr) _aligned_free(ptr); }
	};
}
using AlignedBufferPtr = std::unique_ptr<BYTE[], AlignedFreeDeleter>;

bool dbgDerefMemRead(duint addr, void *dest, duint size);
std::string demangle(const char *sz_name, bool noECSU = false);
std::string addrToStr(duint addr, bool pad = true);
bool setLogFileName(const std::wstring &fname);
void closeLogFile();
void flushLogFile();
void log(const char *format, ...);
void logNoTime(const char *format, ...);
void guiStatusBarPrintf(const char *format, ...);

};
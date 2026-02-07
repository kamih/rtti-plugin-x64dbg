#include "Utils.h"
#include <iomanip>
#include <sstream>
#include <time.h>

namespace RTTI {;

static FILE *gFileOut = 0;
static DWORD gStartTickCount = 0;
constexpr int gLogBufferLen = 1024;

struct RelTime
{
	int hours;
	int minutes;
	int seconds;
	int milliseconds;
};

const UINT UNDNAME_NO_ECSU = 0x08000;  // Suppress enum/class/struct/union

extern "C" char *__cdecl __unDName(char *outputString, const char *name, int maxStringLength, void *pAlloc, void *pFree, unsigned short disableFlags);

////////////////////////////////////////////////////////////////////////////////////////////////////////////
namespace {
	struct FreeDeleter {
		void operator()(char *ptr) const {
			free(ptr);
		}
	};
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::string demangle(const char *sz_name, bool noECSU)
{
	auto flags = UNDNAME_32_BIT_DECODE | UNDNAME_NO_ARGUMENTS;
	if (noECSU)
		flags |= UNDNAME_NO_ECSU;
	// skip '.' at the start
	std::unique_ptr<char[], FreeDeleter> tmp(__unDName(nullptr, sz_name+1, 0, malloc, free, flags));
	if (!tmp)
		return "";
	return std::string(tmp.get());
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool dbgDerefMemRead(duint addr, void *dest, duint size)
{
	duint val = 0;
	if (!DbgMemRead(addr, &val, sizeof(val)))
		return false;
	if (!DbgMemRead(val, dest, size))
		return false;
	return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::string addrToStr(duint addr, bool pad)
{
    std::ostringstream ss;
    ss << "0x" << std::uppercase << std::hex;
	if (pad)
		ss << std::setfill('0') << std::setw(sizeof(addr)*2);
    ss << addr;
    return ss.str();
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void GetTimeSinceStart(RelTime &reltime)
{
	const int msPerHour = 1000 * 60 * 60;
	const int msPerMin = 1000 * 60;
	DWORD offset = GetTickCount() - gStartTickCount;
	reltime.hours = offset / msPerHour;
    offset %= msPerHour;
	reltime.minutes = offset / msPerMin;
    offset %= msPerMin;
	reltime.seconds = offset / 1000;
	reltime.milliseconds = offset % 1000;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool setLogFileName(const std::wstring &fname)
{
	if (gFileOut) {
		fclose(gFileOut);
		gFileOut = 0;
	}
	if (!fname.empty()) {
		_wfopen_s(&gFileOut, fname.c_str(), L"wt");
		if (!gFileOut)
			return false;
		time_t t = time(0);
		char buf[100];
		ctime_s(buf, 100, &t);
		fprintf(gFileOut, buf);
		gStartTickCount = GetTickCount();
	}
	return true;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void closeLogFile()
{
	if (gFileOut) {
		fclose(gFileOut);
		gFileOut = 0;
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void flushLogFile()
{
	if (gFileOut)
		fflush(gFileOut);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void log(const char *format, ...)
{
	char tmpDebugBuffer[gLogBufferLen];
	va_list args;
	va_start(args, format);
	(void)_vsnprintf_s(tmpDebugBuffer, gLogBufferLen, _TRUNCATE, format, args);
	va_end(args);
#ifdef TEST_BUILD
	OutputDebugStringA(tmpDebugBuffer);
#endif
	if (gFileOut) {
		RelTime reltime;
		GetTimeSinceStart(reltime);
		fprintf(gFileOut, "%02d:%02d:%02d:%03d %s",
			reltime.hours, reltime.minutes, reltime.seconds, reltime.milliseconds, tmpDebugBuffer);
	}
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void logNoTime(const char *format, ...)
{
	char tmpDebugBuffer[gLogBufferLen];
	va_list args;
	va_start(args, format);
	(void)_vsnprintf_s(tmpDebugBuffer, gLogBufferLen, _TRUNCATE, format, args);
	va_end(args);
#ifdef TEST_BUILD
	OutputDebugStringA(tmpDebugBuffer);
#endif
	if (gFileOut)
		fprintf(gFileOut, "%s", tmpDebugBuffer);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void guiStatusBarPrintf(const char *format, ...)
{
	char tmpDebugBuffer[gLogBufferLen];
	va_list args;
	va_start(args, format);
	(void)_vsnprintf_s(tmpDebugBuffer, gLogBufferLen, _TRUNCATE, format, args);
	va_end(args);
	GuiAddStatusBarMessage(tmpDebugBuffer);
}

};
#include "Log.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>

void logInfo(const char* fmt, ...)
{
	va_list vl;
	va_start(vl, fmt);
	char str[2048];
	vsprintf_s(str, sizeof(str) - 1, fmt, vl);
	va_end(vl);
	OutputDebugStringA(str);
}
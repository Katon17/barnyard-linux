// Narrow CRT compatibility and UTF-16 redirection for actual engine sources.
// Force-include before upstream PCH. This is NOT a Win32 API stub header.
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
#include <cwctype>
#include <cmath>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <iostream>
#include <filesystem>
#include <strings.h>
#include <alloca.h>
#include <unistd.h>
#include <sys/syscall.h>
#if defined(__i386__) || defined(__x86_64__)
#include <x86intrin.h>
#endif
#include "TStringHelper_Linux.h"
// Native Linux callbacks consistently use the System V ABI, including vendor
// headers compiled without ToshiPCH. Do not give them a Windows stdcall ABI.
#ifndef __stdcall
#define __stdcall
#endif
#ifndef __cdecl
#define __cdecl
#endif
#ifndef __assume
#define __assume(x) do { if (!(x)) __builtin_unreachable(); } while(false)
#endif
inline uint32_t GetCurrentThreadId() { return uint32_t(syscall(SYS_gettid)); }
void TDebug_OutputDebugString(const char*);
inline void OutputDebugStringA(const char* text) { TDebug_OutputDebugString(text); }
#define _stricmp strcasecmp
#define _strnicmp strncasecmp
#define strnicmp strncasecmp
#define _strdup strdup
#define _vsnprintf vsnprintf
#define _snprintf snprintf
#define _alloca alloca
#define _strupr TStringHelper_Linux::ToUpper
#define _strlwr TStringHelper_Linux::ToLower
#define _strtime TStringHelper_Linux::GetTimeString
#define _itoa TStringHelper_Linux::IntToString
// glibc's wide ABI remains 32-bit even with -fshort-wchar. Never pass engine
// wchar buffers to it. System/C++ wide declarations were included above first.
#define wcslen TStringHelper_Linux::WideLength
#define wcscmp TStringHelper_Linux::WideCompare
#define wcsncmp TStringHelper_Linux::WideCompareN
#define _wcsicmp TStringHelper_Linux::WideCompareNoCase
#define _wcsnicmp TStringHelper_Linux::WideCompareNoCaseN
#define wcscpy TStringHelper_Linux::WideCopy
#define wcsncpy TStringHelper_Linux::WideCopyN
#define wcscat TStringHelper_Linux::WideConcat
#define wcsncat TStringHelper_Linux::WideConcatN
#define wcschr TStringHelper_Linux::WideFindChar
#define wcsrchr TStringHelper_Linux::WideFindLastChar
#define wcsstr TStringHelper_Linux::WideFindString
#define _wcsupr TStringHelper_Linux::WideToUpper
#define _wcslwr TStringHelper_Linux::WideToLower
#define _wtoi TStringHelper_Linux::WideToInt
#define _wtof TStringHelper_Linux::WideToFloat
#define _itow TStringHelper_Linux::WideFromInt
#define _vsnwprintf TStringHelper_Linux::WideFormatN
#define _vswprintf TStringHelper_Linux::WideFormat

/**
 * TTypes_Linux.h — pengganti Platform/WASM/TTypes_WASM.h untuk Linux.
 *
 * Tujuan: menyediakan alias tipe Win32 + fungsi "MSVC C runtime" yang dipakai
 * kode bersama TOSHI/OpenBarnyard, tanpa Win32 dan tanpa emscripten.
 *
 * ⚠️  STATUS: kerangka hasil audit (lihat docs/01-AUDIT.md & docs/03-PETA-KODE.md).
 *     Setiap simbol diturunkan dari pemakaian nyata di sumber upstream @ b63d5e3,
 *     tetapi belum dikompilasi penuh terhadap seluruh engine. Saat error
 *     "not declared" muncul, tambahkan di sini — JANGAN menyentuh kode upstream.
 *
 * Dipakai sebagai: -include TTypes_Linux.h  (lihat premake/linux.lua)
 */
#pragma once

#ifndef TOSHI_SKU_LINUX
#  error "TTypes_Linux.h hanya untuk build Linux (TOSHI_SKU_LINUX)"
#endif

// ---------------------------------------------------------------------------
// 1. Header dasar (jarak dari versi WASM: tanpa <emscripten*>, <io.h>, <direct.h>)
// ---------------------------------------------------------------------------
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <math.h>
#include <wchar.h>
#include <time.h>
#include <pthread.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#undef PAGE_SIZE
#undef PAGE_SHIFT

// Helper 16-bit wchar (ditulis terpisah supaya bisa diuji unit)
#include "TStringHelper_Linux.h"

// ---------------------------------------------------------------------------
// 2. Tipe Win32 yang dipakai kode bersama untuk layout retail
//    (salinan dari TTypes_WASM.h — layout ini sengaja dipertahankan)
// ---------------------------------------------------------------------------
using DWORD   = uint32_t;        // Win32 tetap 32-bit pada host LP64
using WORD    = unsigned short;
using BYTE    = unsigned char;
using BOOL    = int;
using UINT    = unsigned int;
using LONG    = int32_t;
using ULONG   = uint32_t;
using INT     = int;
using FLOAT   = float;
using CHAR    = char;
using WCHAR   = wchar_t;         // 2 byte karena -fshort-wchar

#define TRUE 1
#define FALSE 0

#ifndef MAX_PATH
#  define MAX_PATH 260
#endif

using HANDLE    = void*;
using HMODULE   = void*;
using HDC       = void*;
using HWND      = void*;
using HICON     = void*;
using LPCSTR    = const char*;
using LPSTR     = char*;
using LPVOID    = void*;
using ULONG_PTR = uintptr_t;
using SIZE_T    = size_t;
using HGLRC     = void*;

union LARGE_INTEGER
{
    struct
    {
        DWORD LowPart;
        LONG  HighPart;
    };
    long long QuadPart;
};

struct FILETIME
{
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
};

using LONGLONG = long long;
using LPBYTE   = unsigned char*;
using HKEY     = void*;
using HGLOBAL  = void*;

union ULARGE_INTEGER
{
    struct
    {
        DWORD LowPart;
        DWORD HighPart;
    };
    unsigned long long QuadPart;
};

#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)
#define INVALID_HANDLE_VALUE ((HANDLE)(intptr_t)-1)

// ---------------------------------------------------------------------------
// 3. MSVC-isme di kode bersama → GCC
//    (302× __stdcall, 14× __declspec, 9× __cdecl, 2× __forceinline — audit §3.4)
// ---------------------------------------------------------------------------
#ifndef __stdcall
#  define __stdcall
#endif
#ifndef __cdecl
#  define __cdecl
#endif
#ifndef __fastcall
#  define __fastcall
#endif
#ifndef __forceinline
#  define __forceinline inline __attribute__( ( always_inline ) )
#endif
#ifndef __declspec
#  define __declspec( x ) /* empty_bases, noinline, dllexport… tidak berlaku */
#endif
#ifndef __unaligned
#  define __unaligned
#endif

// ---------------------------------------------------------------------------
// 4. Fungsi Win32 "seadanya" yang dipanggil kode bersama.
//    Sama dengan versi WASM: banyak yang sengaja stub (clipboard, dsb).
// ---------------------------------------------------------------------------
inline void* HeapAlloc( HANDLE, DWORD, size_t a_uiSize ) { return malloc( a_uiSize ); }

// Frame limiter Windows (AppBoot BARNYARD_COMMUNITY_PATCH).
// Di Linux: lihat FrameLimiter di src/Platform/Linux/TPlatform_Linux.cpp —
// simbol ini hanya untuk memuaskan kompilasi jalur bersama.
inline HANDLE CreateWaitableTimerExW( void*, const wchar_t*, DWORD, DWORD ) { return nullptr; }
inline BOOL   SetWaitableTimer( HANDLE, const LARGE_INTEGER*, LONG, void*, void*, BOOL ) { return FALSE; }
inline UINT   timeBeginPeriod( UINT ) { return 0; }
inline void   YieldProcessor() { __builtin_ia32_pause(); }

// Clipboard: tidak dipakai gameplay (versi WASM juga stub)
inline BOOL    OpenClipboard( HWND ) { return FALSE; }
inline BOOL    EmptyClipboard() { return FALSE; }
inline BOOL    CloseClipboard() { return FALSE; }
inline HANDLE  SetClipboardData( UINT, HANDLE ) { return nullptr; }
inline HGLOBAL GlobalAlloc( UINT, size_t ) { return nullptr; }
inline void*   GlobalLock( HGLOBAL ) { return nullptr; }
inline BOOL    GlobalUnlock( HGLOBAL ) { return FALSE; }
inline HGLOBAL GlobalFree( HGLOBAL ) { return nullptr; }

inline HANDLE GetCurrentProcess() { return nullptr; }
inline BOOL   GetProcessTimes( HANDLE, FILETIME* a_pCreation, FILETIME*, FILETIME*, FILETIME* )
{
    *a_pCreation = {};
    return TRUE;
}
void GetSystemTimeAsFileTime( FILETIME* a_pTime );  // TPlatform_Linux.cpp
unsigned long GetCurrentThreadId();                 // TPlatform_Linux.cpp

inline BOOL  CreateDirectoryA( const char*, void* ) { return TRUE; }
inline DWORD GetFileAttributesA( const char* ) { return INVALID_FILE_ATTRIBUTES; }
inline HDC   GetDC( HWND ) { return nullptr; }
inline int   ReleaseDC( HWND, HDC ) { return 1; }
inline int   GetDeviceCaps( HDC, int ) { return 96; }

// CRT MSVC yang tidak ada di glibc → helper sendiri
#define _strupr  TStringHelper_Linux::ToUpper
#define _strlwr  TStringHelper_Linux::ToLower
#define _strtime TStringHelper_Linux::GetTimeString
#define _itoa    TStringHelper_Linux::IntToString
#define __rdtsc  TStringHelper_Linux::GetTimestamp

// Wide-char: WAJIB karena -fshort-wchar membuat fungsi libc salah
#define wcslen       TStringHelper_Linux::WideLength
#define wcscmp       TStringHelper_Linux::WideCompare
#define wcsncmp      TStringHelper_Linux::WideCompareN
#define _wcsicmp     TStringHelper_Linux::WideCompareNoCase
#define _wcsnicmp    TStringHelper_Linux::WideCompareNoCaseN
#define wcscpy       TStringHelper_Linux::WideCopy
#define wcsncpy      TStringHelper_Linux::WideCopyN
#define wcscat       TStringHelper_Linux::WideConcat
#define wcsncat      TStringHelper_Linux::WideConcatN
#define wcschr       TStringHelper_Linux::WideFindChar
#define wcsrchr      TStringHelper_Linux::WideFindLastChar
#define wcsstr       TStringHelper_Linux::WideFindString
#define _wcsupr      TStringHelper_Linux::WideToUpper
#define _wcslwr      TStringHelper_Linux::WideToLower
#define _wtoi        TStringHelper_Linux::WideToInt
#define _wtof        TStringHelper_Linux::WideToFloat
#define _itow        TStringHelper_Linux::WideFromInt
#define _vsnwprintf  TStringHelper_Linux::WideFormatN
#define _vswprintf   TStringHelper_Linux::WideFormat

static_assert(sizeof(DWORD) == 4 && sizeof(LONG) == 4);
static_assert(sizeof(FILETIME) == 8 && sizeof(LARGE_INTEGER) == 8);

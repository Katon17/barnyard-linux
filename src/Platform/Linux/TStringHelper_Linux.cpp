/**
 * TStringHelper_Linux.cpp — implementasi pengganti TStringHelper_WASM.cpp
 *
 * Semua fungsi ditulis ulang dalam bentuk murni (tidak memanggil libc wide)
 * supaya benar untuk wchar_t 2 byte (-fshort-wchar) DAN 4 byte (host). Ini
 * membuat file ini bisa diuji unit di laptop tanpa mengompilasi engine.
 */
#include "TStringHelper_Linux.h"

#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <wchar.h>

namespace
{
    inline int WideCharClassLower( uint32_t a_uiCode )
    {
        // ASCII saja: cukup untuk format & nama file engine (sama dengan WASM)
        if ( a_uiCode >= 'A' && a_uiCode <= 'Z' ) return a_uiCode + 32;
        return (int)a_uiCode;
    }

    inline int WideCharClassUpper( uint32_t a_uiCode )
    {
        if ( a_uiCode >= 'a' && a_uiCode <= 'z' ) return a_uiCode - 32;
        return (int)a_uiCode;
    }

    // Formatter UTF-16 is isolated in TWideFormat_Linux.cpp; no libc wide ABI.
} // namespace

// ---------------------------------------------------------------------------
// String 8-bit
// ---------------------------------------------------------------------------
char* TStringHelper_Linux::ToUpper( char* a_szString )
{
    if ( !a_szString ) return nullptr;
    for ( char* p = a_szString; *p; ++p ) *p = (char)toupper( (unsigned char)*p );
    return a_szString;
}

char* TStringHelper_Linux::ToLower( char* a_szString )
{
    if ( !a_szString ) return nullptr;
    for ( char* p = a_szString; *p; ++p ) *p = (char)tolower( (unsigned char)*p );
    return a_szString;
}

char* TStringHelper_Linux::GetTimeString( char* a_szBuffer )
{
    // Setara _strtime: "HH:MM:SS"
    if ( !a_szBuffer ) return nullptr;
    time_t    t = time( nullptr );
    struct tm tmv;
    localtime_r( &t, &tmv );
    snprintf( a_szBuffer, 9, "%02d:%02d:%02d", tmv.tm_hour, tmv.tm_min, tmv.tm_sec );
    return a_szBuffer;
}

char* TStringHelper_Linux::IntToString( int a_iValue, char* a_szBuffer, int a_iRadix )
{
    // Setara _itoa
    if ( !a_szBuffer ) return nullptr;
    if ( a_iRadix == 10 )
        snprintf( a_szBuffer, 12, "%d", a_iValue );
    else if ( a_iRadix == 16 )
        snprintf( a_szBuffer, 12, "%x", a_iValue );
    else
        snprintf( a_szBuffer, 34, "%d", a_iValue );
    return a_szBuffer;
}

unsigned long long TStringHelper_Linux::GetTimestamp()
{
    // Setara __rdtsc: cukup penghitung monotonik (dipakai untuk profiling kasar)
    struct timespec ts;
    clock_gettime( CLOCK_MONOTONIC, &ts );
    return (unsigned long long)ts.tv_sec * 1000000000ull + (unsigned long long)ts.tv_nsec;
}

// ---------------------------------------------------------------------------
// Wide char (2 byte di target, 4 byte di host uji)
// ---------------------------------------------------------------------------
size_t TStringHelper_Linux::WideLength( const wchar_t* a_wszString )
{
    if ( !a_wszString ) return 0;
    const wchar_t* p = a_wszString;
    while ( *p ) p++;
    return (size_t)( p - a_wszString );
}

int TStringHelper_Linux::WideCompare( const wchar_t* a_wszA, const wchar_t* a_wszB )
{
    if ( !a_wszA || !a_wszB ) return a_wszA ? 1 : ( a_wszB ? -1 : 0 );
    for ( ;; ++a_wszA, ++a_wszB )
    {
        if ( *a_wszA != *a_wszB ) return *a_wszA < *a_wszB ? -1 : 1;
        if ( !*a_wszA ) return 0;
    }
}

int TStringHelper_Linux::WideCompareN( const wchar_t* a_wszA, const wchar_t* a_wszB, size_t a_uiSize )
{
    if ( a_uiSize == 0 ) return 0;
    if ( !a_wszA || !a_wszB ) return a_wszA ? 1 : ( a_wszB ? -1 : 0 );
    for ( size_t i = 0; i < a_uiSize; ++i )
    {
        if ( a_wszA[ i ] != a_wszB[ i ] ) return a_wszA[ i ] < a_wszB[ i ] ? -1 : 1;
        if ( !a_wszA[ i ] ) return 0;
    }
    return 0;
}

int TStringHelper_Linux::WideCompareNoCase( const wchar_t* a_wszA, const wchar_t* a_wszB )
{
    if ( !a_wszA || !a_wszB ) return a_wszA ? 1 : ( a_wszB ? -1 : 0 );
    for ( ;; ++a_wszA, ++a_wszB )
    {
        int a = WideCharClassLower( (uint32_t)*a_wszA );
        int b = WideCharClassLower( (uint32_t)*a_wszB );
        if ( a != b ) return a < b ? -1 : 1;
        if ( !*a_wszA ) return 0;
    }
}

int TStringHelper_Linux::WideCompareNoCaseN( const wchar_t* a_wszA, const wchar_t* a_wszB, size_t a_uiSize )
{
    if ( a_uiSize == 0 ) return 0;
    if ( !a_wszA || !a_wszB ) return a_wszA ? 1 : ( a_wszB ? -1 : 0 );
    for ( size_t i = 0; i < a_uiSize; ++i )
    {
        int a = WideCharClassLower( (uint32_t)a_wszA[ i ] );
        int b = WideCharClassLower( (uint32_t)a_wszB[ i ] );
        if ( a != b ) return a < b ? -1 : 1;
        if ( !a_wszA[ i ] ) return 0;
    }
    return 0;
}

wchar_t* TStringHelper_Linux::WideCopy( wchar_t* a_wszDst, const wchar_t* a_wszSrc )
{
    if ( !a_wszDst || !a_wszSrc ) return a_wszDst;
    wchar_t* d = a_wszDst;
    while ( ( *d++ = *a_wszSrc++ ) != 0 ) {}
    return a_wszDst;
}

wchar_t* TStringHelper_Linux::WideCopyN( wchar_t* a_wszDst, const wchar_t* a_wszSrc, size_t a_uiSize )
{
    if ( !a_wszDst || !a_wszSrc || a_uiSize == 0 ) return a_wszDst;
    size_t i = 0;
    for ( ; i < a_uiSize && a_wszSrc[ i ]; ++i ) a_wszDst[ i ] = a_wszSrc[ i ];
    for ( ; i < a_uiSize; ++i ) a_wszDst[ i ] = 0; // padding seperti wcsncpy
    return a_wszDst;
}

wchar_t* TStringHelper_Linux::WideConcat( wchar_t* a_wszDst, const wchar_t* a_wszSrc )
{
    if ( !a_wszDst || !a_wszSrc ) return a_wszDst;
    WideCopy( a_wszDst + WideLength( a_wszDst ), a_wszSrc );
    return a_wszDst;
}

wchar_t* TStringHelper_Linux::WideConcatN( wchar_t* a_wszDst, const wchar_t* a_wszSrc, size_t a_uiSize )
{
    if ( !a_wszDst || !a_wszSrc || a_uiSize == 0 ) return a_wszDst;

    // Setara wcsncat: menambah paling banyak a_uiSize karakter lalu menutup string.
    wchar_t* pEnd    = a_wszDst + WideLength( a_wszDst );
    size_t   uiAdded = 0;

    while ( uiAdded < a_uiSize && a_wszSrc[ uiAdded ] )
    {
        pEnd[ uiAdded ] = a_wszSrc[ uiAdded ];
        uiAdded++;
    }

    pEnd[ uiAdded ] = 0;
    return a_wszDst;
}

wchar_t* TStringHelper_Linux::WideFindChar( const wchar_t* a_wszString, wchar_t a_cChar )
{
    if ( !a_wszString ) return nullptr;
    for ( ; *a_wszString; ++a_wszString )
        if ( *a_wszString == a_cChar ) return const_cast<wchar_t*>( a_wszString );
    return a_cChar == 0 ? const_cast<wchar_t*>( a_wszString ) : nullptr;
}

wchar_t* TStringHelper_Linux::WideFindLastChar( const wchar_t* a_wszString, wchar_t a_cChar )
{
    if ( !a_wszString ) return nullptr;
    const wchar_t* pLast = nullptr;
    const wchar_t* p = a_wszString;
    do { if ( *p == a_cChar ) pLast = p; } while ( *p++ );
    return const_cast<wchar_t*>( pLast );
}

wchar_t* TStringHelper_Linux::WideFindString( const wchar_t* a_wszString, const wchar_t* a_wszFind )
{
    if ( !a_wszString || !a_wszFind ) return nullptr;
    const size_t n = WideLength( a_wszFind );
    if ( n == 0 ) return const_cast<wchar_t*>( a_wszString );
    for ( const wchar_t* p = a_wszString; *p; ++p )
    {
        size_t i = 0;
        while ( i < n && p[ i ] && p[ i ] == a_wszFind[ i ] ) i++;
        if ( i == n ) return const_cast<wchar_t*>( p );
    }
    return nullptr;
}

wchar_t* TStringHelper_Linux::WideToUpper( wchar_t* a_wszString )
{
    if ( !a_wszString ) return nullptr;
    for ( wchar_t* p = a_wszString; *p; ++p ) *p = (wchar_t)WideCharClassUpper( (uint32_t)*p );
    return a_wszString;
}

wchar_t* TStringHelper_Linux::WideToLower( wchar_t* a_wszString )
{
    if ( !a_wszString ) return nullptr;
    for ( wchar_t* p = a_wszString; *p; ++p ) *p = (wchar_t)WideCharClassLower( (uint32_t)*p );
    return a_wszString;
}

int TStringHelper_Linux::WideToInt( const wchar_t* a_wszString )
{
    if ( !a_wszString ) return 0;
    int  iResult = 0;
    bool bNeg   = false;
    while ( *a_wszString == ' ' || *a_wszString == '\t' ) a_wszString++;
    if ( *a_wszString == '-' ) { bNeg = true; a_wszString++; }
    else if ( *a_wszString == '+' ) a_wszString++;
    while ( *a_wszString >= '0' && *a_wszString <= '9' )
        iResult = iResult * 10 + (int)( *a_wszString++ - '0' );
    return bNeg ? -iResult : iResult;
}

double TStringHelper_Linux::WideToFloat( const wchar_t* a_wszString )
{
    if ( !a_wszString ) return 0.0;
    char  szBuf[ 64 ];
    int   i = 0;
    bool  bNeg = false;
    if ( *a_wszString == '-' ) { bNeg = true; a_wszString++; }
    else if ( *a_wszString == '+' ) a_wszString++;
    while ( ( ( *a_wszString >= '0' && *a_wszString <= '9' ) || *a_wszString == '.' || *a_wszString == 'e' || *a_wszString == 'E' ||
             *a_wszString == '+' || *a_wszString == '-' ) && i < 63 )
        szBuf[ i++ ] = (char)*a_wszString++;
    szBuf[ i ] = 0;
    double d = atof( szBuf );
    return bNeg ? -d : d;
}

wchar_t* TStringHelper_Linux::WideFromInt( int a_iValue, wchar_t* a_wszBuffer, int a_iRadix )
{
    if ( !a_wszBuffer ) return nullptr;
    wchar_t* start = a_wszBuffer;
    char szBuf[ 40 ];
    IntToString( a_iValue, szBuf, a_iRadix );
    for ( char* p = szBuf; *p; ++p ) *a_wszBuffer++ = (wchar_t)(unsigned char)*p;
    *a_wszBuffer = 0;
    return start;
}

// WideFormatN/WideFormat are implemented in TWideFormat_Linux.cpp.

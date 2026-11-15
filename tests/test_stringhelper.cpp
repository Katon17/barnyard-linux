// Uji unit TStringHelper_Linux (host, wchar 4 byte). Tidak butuh engine.
#include "../src/Platform/Linux/TStringHelper_Linux.h"
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <cstdarg>

static int format(wchar_t* out, size_t n, const wchar_t* fmt, ...) {
    va_list args; va_start(args, fmt);
    int result = TStringHelper_Linux::WideFormatN(out, n, fmt, args);
    va_end(args); return result;
}

static int g_fail = 0;
#define CHECK(cond) do { if(!(cond)) { printf("  FAIL: %s (line %d)\n", #cond, __LINE__); g_fail++; } } while(0)

int main()
{
    // 8-bit
    char buf[64] = "Hello World 123";
    TStringHelper_Linux::ToUpper(buf); CHECK(strcmp(buf,"HELLO WORLD 123")==0);
    TStringHelper_Linux::ToLower(buf); CHECK(strcmp(buf,"hello world 123")==0);
    TStringHelper_Linux::IntToString(255, buf, 10); CHECK(strcmp(buf,"255")==0);
    TStringHelper_Linux::IntToString(255, buf, 16); CHECK(strcmp(buf,"ff")==0);
    TStringHelper_Linux::GetTimeString(buf); CHECK(strlen(buf)==8 && buf[2]==':' && buf[5]==':');
    CHECK(TStringHelper_Linux::GetTimestamp() > 0);

    // Wide
    const wchar_t* a = L"Barnyard";
    CHECK(TStringHelper_Linux::WideLength(a)==8);
    CHECK(TStringHelper_Linux::WideCompare(a, L"Barnyard")==0);
    CHECK(TStringHelper_Linux::WideCompare(a, L"Barnyarz")<0);
    CHECK(TStringHelper_Linux::WideCompareNoCase(L"BARNYARD", a)==0);
    CHECK(TStringHelper_Linux::WideCompareNoCaseN(L"barX", L"BARNY", 3)==0);

    wchar_t dst[32];
    TStringHelper_Linux::WideCopy(dst, a);           CHECK(TStringHelper_Linux::WideCompare(dst,a)==0);
    TStringHelper_Linux::WideConcat(dst, L" 2006");  CHECK(TStringHelper_Linux::WideLength(dst)==13);
    TStringHelper_Linux::WideCopyN(dst, a, 4);       CHECK(dst[0]==L'B' && dst[3]==L'n'); // wcsncpy tidak menambah NUL jika sumber lebih panjang
    CHECK(TStringHelper_Linux::WideFindChar(a, L'y') != nullptr);
    CHECK(TStringHelper_Linux::WideFindChar(a, L'Z') == nullptr);
    CHECK(TStringHelper_Linux::WideFindLastChar(a, L'a') == a+5); // "Barnyard": a terakhir di indeks 5
    CHECK(TStringHelper_Linux::WideFindString(a, L"nyar") == a+3);
    CHECK(TStringHelper_Linux::WideFindString(a, L"zzz") == nullptr);

    TStringHelper_Linux::WideCopy(dst, L"MixedCase");
    TStringHelper_Linux::WideToUpper(dst); CHECK(TStringHelper_Linux::WideCompare(dst,L"MIXEDCASE")==0);
    TStringHelper_Linux::WideToLower(dst); CHECK(TStringHelper_Linux::WideCompare(dst,L"mixedcase")==0);

    CHECK(TStringHelper_Linux::WideToInt(L"-1234") == -1234);
    CHECK(TStringHelper_Linux::WideToInt(L"  +42") == 42);
    CHECK(TStringHelper_Linux::WideToFloat(L"3.5") == 3.5);
    CHECK(TStringHelper_Linux::WideToFloat(L"-0.25") == -0.25);
    TStringHelper_Linux::WideFromInt(77, dst, 10); CHECK(TStringHelper_Linux::WideCompare(dst,L"77")==0);

    // WideConcatN: perilaku batas
    TStringHelper_Linux::WideCopy(dst, L"ab"); TStringHelper_Linux::WideConcatN(dst, L"cdef", 2);
    CHECK(TStringHelper_Linux::WideCompare(dst, L"abcd")==0);

    CHECK(TStringHelper_Linux::WideFindLastChar(a, 0) == a+8);
    CHECK(TStringHelper_Linux::WideFromInt(77, dst, 10) == dst);
    CHECK(format(dst, 32, L"%d", 42) == 2);
    CHECK(TStringHelper_Linux::WideCompare(dst, L"42") == 0);
    CHECK(format(dst, 32, L"%s / %S", L"wide", "byte") == 11);
    CHECK(TStringHelper_Linux::WideCompare(dst, L"wide / byte") == 0);
    CHECK(format(dst, 32, L"%-6.3s!", L"abcdef") == 7);
    CHECK(TStringHelper_Linux::WideCompare(dst, L"abc   !") == 0);
    CHECK(format(dst, 32, L"%hs:%ls", "narrow", L"wide") == 11);
    CHECK(TStringHelper_Linux::WideCompare(dst, L"narrow:wide") == 0);
    CHECK(format(dst, 32, L"%08x %.2f", 255u, 3.5) == 13);
    CHECK(TStringHelper_Linux::WideCompare(dst, L"000000ff 3.50") == 0);
    CHECK(format(dst, 32, L"%I64d", -1234567890123ll) == 14);
    CHECK(TStringHelper_Linux::WideCompare(dst, L"-1234567890123") == 0);
    CHECK(format(dst, 32, L"%*.*s", -6, 3, L"abcdef") == 6);
    CHECK(TStringHelper_Linux::WideCompare(dst, L"abc   ") == 0);
    dst[4] = L'X';
    CHECK(format(dst, 4, L"abcdef") == 6);
    CHECK(TStringHelper_Linux::WideCompare(dst, L"abc") == 0 && dst[4] == L'X');
    int untouched = 17;
    CHECK(format(dst, 32, L"%n", &untouched) == -1 && untouched == 17);
    CHECK(format(dst, 32, L"broken%") == -1);
    CHECK(format(nullptr, 0, L"%s", L"count") == 5);


    printf(g_fail ? "RESULT: %d FAILURE(S)\n" : "RESULT: ALL PASS\n", g_fail);
    return g_fail ? 1 : 0;
}

// MSVC wide printf subset for engine UTF-16 buffers. Never calls glibc wide APIs.
// %s/%ls are wide, %S/%hs narrow (byte widening), matching the upstream WASM
// convention. This is not a general Unicode/locale printf implementation.
#include "TStringHelper_Linux.h"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <climits>
#include <cstring>
#include <string>
#include <type_traits>
#include <vector>
#include <sys/types.h>

int TStringHelper_Linux::WideFormatN(wchar_t* out, size_t capacity, const wchar_t* format, va_list input) {
    if (!format || (!out && capacity)) return -1;
    va_list ap;
    va_copy(ap, input);
    size_t count = 0;
    bool valid = true;
    constexpr int limit = 1048576;
    auto put = [&](wchar_t c) { if (capacity && count < capacity-1) out[count] = c; ++count; };
    auto narrow = [&](const char* text) { while (*text) put((unsigned char)*text++); };
    for (const wchar_t* p = format; valid && *p;) {
        if (*p != L'%') { put(*p++); continue; }
        ++p;
        if (*p == L'%') { put(L'%'); ++p; continue; }
        std::string flags;
        while (*p && *p < 128 && strchr("-+ #0", char(*p))) flags += char(*p++);
        bool left = flags.find('-') != std::string::npos;
        int width = 0, precision = -1;
        auto number = [&](int& value) {
            value = 0;
            while (*p >= L'0' && *p <= L'9') {
                if (value > limit/10) { valid = false; break; }
                value = value*10 + int(*p++-L'0');
                if (value > limit) { valid = false; break; }
            }
        };
        if (*p == L'*') {
            width = va_arg(ap, int); ++p;
            if (width < 0 && width != INT_MIN) { width = -width; left = true; flags += '-'; }
        } else number(width);
        if (*p == L'.') {
            ++p;
            if (*p == L'*') { precision = va_arg(ap, int); ++p; }
            else number(precision);
        }
        if (!valid || width < 0 || width > limit || precision > limit) { valid = false; break; }
        std::string length;
        if (*p == L'h' || *p == L'l') {
            auto c = *p++; length += char(c);
            if (*p == c) { length += char(c); ++p; }
        } else if (*p == L'z' || *p == L't' || *p == L'j' || *p == L'L') length += char(*p++);
        else if (*p == L'I') {
            ++p;
            if (p[0] == L'6' && p[1] == L'4') { length = "ll"; p += 2; }
            else if (p[0] == L'3' && p[1] == L'2') p += 2;
            else { valid = false; break; }
        }
        if (!*p || *p > 127) { valid = false; break; }
        const char kind = char(*p++);
        if (kind == 's' || kind == 'S') {
            if (!length.empty() && length != "h" && length != "l") { valid = false; break; }
            const bool bytes = length == "h" || (kind == 'S' && length != "l");
            size_t n = 0;
            const char* text = nullptr;
            const wchar_t* wide = nullptr;
            if (bytes) {
                text = va_arg(ap, const char*); if (!text) text = "(null)";
                while (text[n] && (precision < 0 || n < size_t(precision))) ++n;
            } else {
                wide = va_arg(ap, const wchar_t*); if (!wide) wide = L"(null)";
                while (wide[n] && (precision < 0 || n < size_t(precision))) ++n;
            }
            const size_t pad = size_t(width) > n ? size_t(width)-n : 0;
            if (!left) for (size_t i=0; i<pad; ++i) put(L' ');
            for (size_t i=0; i<n; ++i) put(bytes ? wchar_t((unsigned char)text[i]) : wide[i]);
            if (left) for (size_t i=0; i<pad; ++i) put(L' ');
            continue;
        }
        if (kind == 'c' || kind == 'C') {
            auto c = wchar_t(va_arg(ap, int));
            if (!left) for (int i=1; i<width; ++i) put(L' ');
            put(c);
            if (left) for (int i=1; i<width; ++i) put(L' ');
            continue;
        }
        std::string spec = "%" + flags;
        if (width) spec += std::to_string(width);
        if (precision >= 0) spec += "." + std::to_string(precision);
        spec += length;
        spec += kind;
        auto numeric = [&](auto value) {
            char local[256];
            int n = snprintf(local, sizeof(local), spec.c_str(), value);
            if (n < 0 || n > limit) { valid = false; return; }
            if (n < int(sizeof(local))) narrow(local);
            else {
                std::vector<char> buffer(size_t(n)+1);
                if (snprintf(buffer.data(), buffer.size(), spec.c_str(), value) != n) { valid = false; return; }
                narrow(buffer.data());
            }
        };
        if (kind == 'd' || kind == 'i') {
            if (length == "ll") numeric(va_arg(ap, long long));
            else if (length == "l") numeric(va_arg(ap, long));
            else if (length == "z") numeric(va_arg(ap, ssize_t));
            else if (length == "t") numeric(va_arg(ap, ptrdiff_t));
            else if (length == "j") numeric(va_arg(ap, intmax_t));
            else if (length.empty() || length == "h" || length == "hh") numeric(va_arg(ap, int));
            else valid = false;
        } else if (kind == 'u' || kind == 'x' || kind == 'X' || kind == 'o') {
            if (length == "ll") numeric(va_arg(ap, unsigned long long));
            else if (length == "l") numeric(va_arg(ap, unsigned long));
            else if (length == "z") numeric(va_arg(ap, size_t));
            else if (length == "t") numeric(va_arg(ap, std::make_unsigned_t<ptrdiff_t>));
            else if (length == "j") numeric(va_arg(ap, uintmax_t));
            else if (length == "h" || length == "hh") numeric(va_arg(ap, int));
            else if (length.empty()) numeric(va_arg(ap, unsigned int));
            else valid = false;
        } else if (strchr("fFeEgGaA", kind)) {
            if (length == "L") numeric(va_arg(ap, long double));
            else if (length.empty() || length == "l") numeric(va_arg(ap, double));
            else valid = false;
        } else if (kind == 'p' && length.empty()) numeric(va_arg(ap, void*));
        else valid = false; // reject %n and unknown directives, rather than corrupt arguments.
    }
    va_end(ap);
    if (capacity) out[std::min(count, capacity-1)] = 0;
    return valid && count <= size_t(INT_MAX) ? int(count) : -1;
}
int TStringHelper_Linux::WideFormat(wchar_t* out, const wchar_t* format, va_list args) {
    return WideFormatN(out, size_t(-1)/4, format, args);
}

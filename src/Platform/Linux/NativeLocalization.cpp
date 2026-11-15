#include "NativePreferences.h"
#include <algorithm>
#include <iterator>
namespace BarnyardLinux {
namespace {
struct Entry { uint64_t hash; const char16_t* translated; };
const Entry translations[]={
#include "IndonesianData.inc"
};
uint64_t textHash(const char16_t* s) {
    uint64_t h=14695981039346656037ULL;
    for(size_t i=0;s[i] && i<65536;++i) {
        h=(h^uint8_t(s[i]))*1099511628211ULL;
        h=(h^uint8_t(s[i]>>8))*1099511628211ULL;
    }
    return h;
}
}
const char16_t* LocalizedText(const char16_t* original) {
    if(!original || !NativePreferences().indonesian)return original;
    auto hash=textHash(original);
    auto* found=std::lower_bound(std::begin(translations),std::end(translations),hash,
        [](const Entry& e,uint64_t value){return e.hash<value;});
    return found!=std::end(translations) && found->hash==hash ? found->translated : original;
}
}

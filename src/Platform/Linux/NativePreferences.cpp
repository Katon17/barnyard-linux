#include "NativePreferences.h"
#include "NativeSave.h"
#include <cstring>
#include <cstdlib>
#include <cstdio>
namespace BarnyardLinux {
namespace {
struct Disk { char magic[8]; uint32_t version,flags,width,height,reserved,checksum; };
static_assert(sizeof(Disk)==32);
Preferences state;
bool loaded=false;
std::string error;
uint32_t checksum(const Disk& d) {
    uint32_t h=2166136261u;
    for(size_t i=0;i<28;++i)h=(h^reinterpret_cast<const unsigned char*>(&d)[i])*16777619u;
    return h;
}
bool valid(const Preferences& p) {return p.windowWidth>=640 && p.windowWidth<=3840 && p.windowHeight>=480 && p.windowHeight<=2160;}
constexpr const char* path="Settings/native-preferences.bin";
}
const Preferences& NativePreferences() {
    if(loaded)return state;
    const char* user=getenv("BARNYARD_USER_DIR"),*game=getenv("BARNYARD_GAME_DIR");
    if(!user || !game)return state;
    loaded=true;state=Preferences{};
    SaveStore store(user,game);
    if(!store.valid()){error=store.error();return state;}
    auto status=store.status(path);
    if(status==SaveStore::Status::Missing)return state;
    Disk d{};
    if(status!=SaveStore::Status::Regular || !store.readExact(path,&d,sizeof(d)) ||
       memcmp(d.magic,"BYLPREF1",8) || d.version!=1 || (d.flags&~15u) || d.reserved || d.checksum!=checksum(d)) {
        error="Invalid native preferences; using defaults";fprintf(stderr,"%s\n",error.c_str());return state;
    }
    Preferences candidate{bool(d.flags&1),bool(d.flags&2),bool(d.flags&4),d.width,d.height};
    candidate.showFPS=bool(d.flags&8);
    if(valid(candidate))state=candidate;
    else error="Invalid preference dimensions; using defaults";
    return state;
}
bool StoreNativePreferences(const Preferences& value) {
    if(!valid(value)){error="Invalid preference dimensions";return false;}
    const char* user=getenv("BARNYARD_USER_DIR"),*game=getenv("BARNYARD_GAME_DIR");
    if(!user || !game){error="Missing native roots";return false;}
    SaveStore store(user,game);
    if(!store.valid()){error=store.error();return false;}
    Disk d{};memcpy(d.magic,"BYLPREF1",8);d.version=1;
    d.flags=(value.fullscreen?1u:0u)|(value.captureMouse?2u:0u)|(value.indonesian?4u:0u)|(value.showFPS?8u:0u);
    d.width=value.windowWidth;d.height=value.windowHeight;d.checksum=checksum(d);
    if(!store.writeAtomic(path,&d,sizeof(d))){error=store.error();return false;}
    state=value;loaded=true;error.clear();return true;
}
const std::string& PreferencesError(){return error;}
void ReloadNativePreferences(){loaded=false;state=Preferences{};error.clear();}
}

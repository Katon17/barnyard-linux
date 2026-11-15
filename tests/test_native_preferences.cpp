#include "Platform/Linux/NativePreferences.h"
#include <cassert>
#include <array>
#include <cstring>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unistd.h>
using namespace BarnyardLinux;
namespace fs=std::filesystem;
int main(){
 char tmp[]="/tmp/barnyard-prefs-XXXXXX";assert(mkdtemp(tmp));
 fs::path base(tmp),user=base/"user",game=base/"game";
 fs::create_directory(user);fs::create_directory(game);
 setenv("BARNYARD_USER_DIR",user.c_str(),1);setenv("BARNYARD_GAME_DIR",game.c_str(),1);
 ReloadNativePreferences();
 auto p=NativePreferences();assert(p.fullscreen && p.captureMouse && !p.indonesian && !p.showFPS);
 assert(LocalizedText(nullptr)==nullptr);
 assert(std::u16string(LocalizedText(u"New Game"))==u"New Game");
 p.fullscreen=false;p.indonesian=true;p.windowWidth=1024;p.windowHeight=768;
 assert(StoreNativePreferences(p));assert(fs::file_size(user/"Settings/native-preferences.bin")==32);
 auto perm=fs::status(user/"Settings/native-preferences.bin").permissions();
 assert((perm&(fs::perms::group_all|fs::perms::others_all))==fs::perms::none);
 ReloadNativePreferences();auto q=NativePreferences();
 assert(!q.fullscreen && q.indonesian && q.windowWidth==1024 && q.windowHeight==768);
 // Existing test4/test5 files contain no FPS flag. They remain valid/default off.
 assert(!q.showFPS && q.captureMouse);
 p.showFPS=true;assert(StoreNativePreferences(p));
 ReloadNativePreferences();q=NativePreferences();
 assert(q.showFPS && !q.fullscreen && q.indonesian && q.captureMouse && q.windowWidth==1024 && q.windowHeight==768);
 q.showFPS=false;assert(StoreNativePreferences(q));ReloadNativePreferences();
 assert(!NativePreferences().showFPS && NativePreferences().indonesian);
 p.showFPS=true;assert(StoreNativePreferences(p));
 assert(std::u16string(LocalizedText(u"New Game"))==u"Permainan Baru");
 assert(std::u16string(LocalizedText(u"Restart"))==u"Mulai Ulang");
 assert(std::u16string(LocalizedText(u"a string not in the catalog"))==u"a string not in the catalog");
 const char16_t* stable=LocalizedText(u"New Game");p.indonesian=false;assert(StoreNativePreferences(p));
 assert(std::u16string(stable)==u"Permainan Baru");
 assert(std::u16string(LocalizedText(u"New Game"))==u"New Game");
 // The new bit 8 is valid, but unrelated future/reserved flag bits remain invalid.
 std::array<unsigned char,32> original{};
 {std::ifstream f(user/"Settings/native-preferences.bin",std::ios::binary);f.read(reinterpret_cast<char*>(original.data()),32);assert(f);}
 auto unknown=original;unknown[12]|=16;
 uint32_t h=2166136261u;for(int i=0;i<28;++i)h=(h^unknown[i])*16777619u;
 std::memcpy(unknown.data()+28,&h,4);
 {std::ofstream f(user/"Settings/native-preferences.bin",std::ios::binary|std::ios::trunc);f.write(reinterpret_cast<const char*>(unknown.data()),32);}
 ReloadNativePreferences();assert(NativePreferences().fullscreen && !NativePreferences().showFPS && !NativePreferences().indonesian);
 {std::ofstream f(user/"Settings/native-preferences.bin",std::ios::binary|std::ios::trunc);f.write(reinterpret_cast<const char*>(original.data()),32);}
 p.windowWidth=0;assert(!StoreNativePreferences(p));
 {std::fstream f(user/"Settings/native-preferences.bin",std::ios::binary|std::ios::in|std::ios::out);f.seekp(16);f.put('x');}
 ReloadNativePreferences();assert(NativePreferences().fullscreen && !NativePreferences().indonesian);
 assert(!PreferencesError().empty());
 {std::ofstream f(user/"Settings/native-preferences.bin",std::ios::binary|std::ios::trunc);f<<"bad";}
 ReloadNativePreferences();assert(NativePreferences().fullscreen);
 fs::remove(user/"Settings/native-preferences.bin");fs::create_symlink(game/"outside",user/"Settings/native-preferences.bin");
 ReloadNativePreferences();assert(NativePreferences().fullscreen);assert(!StoreNativePreferences(Preferences{}));
 assert(!fs::exists(game/"outside"));fs::remove_all(base);
 std::cout<<"PASS: preference defaults, atomic 0600 persistence, display/language/FPS roundtrip and legacy FPS-off compatibility, stable translation pointers, English fallback, dimensions/unknown-flag/checksum/truncation/symlink rejection\n";
}

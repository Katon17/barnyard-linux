// Exercise the compiled lookup on a PRIVATE length-prefixed UTF-16LE catalog.
// Test fixtures contain original game text: never add them to a source/package.
#include "Platform/Linux/NativePreferences.h"
#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>
using namespace BarnyardLinux;
uint32_t number(std::ifstream& f){uint32_t n=0;f.read(reinterpret_cast<char*>(&n),4);assert(f);return n;}
std::u16string text(std::ifstream& f){auto n=number(f);assert(n<65536);std::u16string s(n,u'\0');f.read(reinterpret_cast<char*>(s.data()),n*2);assert(f);return s;}
int main(int argc,char** argv){
 assert(argc==2);std::ifstream input(argv[1],std::ios::binary);assert(input);
 char tmp[]="/tmp/barnyard-catalog-XXXXXX";assert(mkdtemp(tmp));
 namespace fs=std::filesystem;fs::path base(tmp),user=base/"user",game=base/"game";
 fs::create_directory(user);fs::create_directory(game);
 setenv("BARNYARD_USER_DIR",user.c_str(),1);setenv("BARNYARD_GAME_DIR",game.c_str(),1);
 auto count=number(input);assert(count>4000 && count<10000);Preferences p;p.indonesian=true;
 assert(StoreNativePreferences(p));
 for(uint32_t i=0;i<count;++i){auto src=text(input),dst=text(input);assert(std::u16string(LocalizedText(src.c_str()))==dst);}
 input.clear();input.seekg(4);p.indonesian=false;assert(StoreNativePreferences(p));
 for(uint32_t i=0;i<count;++i){auto src=text(input);text(input);assert(LocalizedText(src.c_str())==src.c_str());}
 fs::remove_all(base);
 std::cout<<"PASS: compiled i386 lookup matched "<<count<<" private source/translation pairs; English bypass matched all; no source text emitted\n";
}

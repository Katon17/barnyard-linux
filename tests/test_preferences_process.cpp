// Run against a disposable legacy fixture, never a user's real save root.
#include "Platform/Linux/NativePreferences.h"
#include <cassert>
#include <iostream>
int main(int argc,char** argv){
 assert(argc==2 || argc==3);
 auto p=BarnyardLinux::NativePreferences();
 assert(p.fullscreen && p.captureMouse && p.indonesian && p.windowWidth==800 && p.windowHeight==600);
 assert(p.showFPS==(argv[1][0]=='1'));
 if(argc==3){p.showFPS=argv[2][0]=='1';assert(BarnyardLinux::StoreNativePreferences(p));}
 std::cout<<"PASS: fresh-process FPS preference "<<(argv[1][0]=='1'?"on":"off")<<"; all legacy fields preserved\n";
}

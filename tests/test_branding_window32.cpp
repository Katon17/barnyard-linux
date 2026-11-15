// Actual X11 window properties, no game data or screenshot required.
#include "Platform/Linux/NativeWindow.h"
#include <SDL2/SDL_syswm.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <cassert>
#include <cstring>
#include <cstdio>
int main(){
 static_assert(sizeof(void*)==4);
 BarnyardLinux::NativeWindow window;
 assert(window.create("Barnyard Linux",640,480,true));
 assert(std::strcmp(SDL_GetWindowTitle(window.handle()),"Barnyard Linux")==0);
 SDL_SysWMinfo info{};SDL_VERSION(&info.version);assert(SDL_GetWindowWMInfo(window.handle(),&info));
 assert(info.subsystem==SDL_SYSWM_X11);
 auto* display=info.info.x11.display;auto handle=info.info.x11.window;XSync(display,False);
 XClassHint klass{};assert(XGetClassHint(display,handle,&klass));
 assert(klass.res_class && std::strcmp(klass.res_class,"barnyard-linux")==0);
 XFree(klass.res_name);XFree(klass.res_class);
 Atom type;int format;unsigned long count,remaining;unsigned char* bytes=nullptr;
 assert(XGetWindowProperty(display,handle,XInternAtom(display,"_NET_WM_ICON",False),0,256*256+2,False,XA_CARDINAL,&type,&format,&count,&remaining,&bytes)==Success);
 assert(type==XA_CARDINAL && format==32 && count==256*256+2 && remaining==0 && bytes);
 auto* pixels=reinterpret_cast<unsigned long*>(bytes);assert(pixels[0]==256 && pixels[1]==256);
 assert((pixels[2]>>24)==0); // Transparent exterior corner from BMPv4 alpha.
 assert((pixels[2+128*256+128]>>24)==255); // Opaque sign center, not an invisible icon.
 XFree(bytes);window.destroy();
 puts("PASS: real X11 SDL window title, barnyard-linux WM_CLASS, matching 256px _NET_WM_ICON with transparent corners and opaque center. Not a full desktop/gameplay test.");
}

// White-box headless test of the actual compiled AGUI2 FPS methods.
// Compile this test only with -fno-access-control (no runtime/header changes).
// No font assets/OnCreate/render scene are invoked; NOT a visual/gameplay test.
#include "pch.h"
#include "Platform/Linux/NativeFPSCounter.h"
#include "GUI/AGUI2.h"
#include <SDL2/SDL.h>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <string>
int main(){
 static_assert(sizeof(void*)==4 && sizeof(TWCHAR)==2);
 AGUI2 gui;
 gui.SetNativeFPSVisible(TFALSE);
 assert(!gui.m_bShowFPSInfo && !gui.m_oFPS.IsVisible());
 gui.UpdateNativeFPS();assert(!gui.m_nativeFPSCounter.hasSample());
 gui.SetNativeFPSVisible(TTRUE);
 assert(gui.m_bShowFPSInfo && gui.m_oFPS.IsVisible());
 assert(std::u16string(reinterpret_cast<const char16_t*>(gui.m_oFPS.GetText()))==u"FPS: --");
 gui.UpdateNativeFPS();SDL_Delay(550);gui.UpdateNativeFPS();
 assert(gui.m_nativeFPSCounter.hasSample());
 assert(std::isfinite(gui.m_nativeFPSCounter.fps()) && gui.m_nativeFPSCounter.fps()>0);
 assert(gui.m_oFPS.GetText()==gui.m_wszFPS);
 std::u16string numeric(reinterpret_cast<const char16_t*>(gui.m_wszFPS));
 assert(numeric.rfind(u"FPS: ",0)==0 && numeric[5]>=u'0' && numeric[5]<=u'9');
 gui.SetNativeFPSVisible(TFALSE);
 assert(!gui.m_bShowFPSInfo && !gui.m_oFPS.IsVisible() && !gui.m_nativeFPSCounter.hasSample());
 gui.SetNativeFPSVisible(TTRUE);assert(gui.m_oFPS.IsVisible() && !gui.m_nativeFPSCounter.hasSample());
 gui.SetNativeFPSVisible(TFALSE);
 puts("PASS: actual compiled AGUI2 FPS show/hide, real SDL clock sampling, UTF-16 numeric formatter, warm-up and re-enable reset. Headless component test only, no assets/visual/gameplay claim.");
}

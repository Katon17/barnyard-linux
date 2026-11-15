// Native backend test only. No game renderer and no copyrighted assets.
#include <GL/glew.h>
#include "Platform/Linux/NativeWindow.h"
#include <cassert>
#include <cstdio>
#include <cstring>

int main() {
    static_assert(sizeof(void*) == 4);
    BarnyardLinux::NativeWindow window;
    if (!window.create("Barnyard Linux backend test (not gameplay)", 640, 480, true) || !window.createContext()) {
        fprintf(stderr, "%s\n", window.error().c_str()); return 1;
    }
    glewExperimental = GL_TRUE;
    assert(glewInit() == GLEW_OK);
    while (glGetError() != GL_NO_ERROR) {}
    assert(GLEW_VERSION_3_3);
    printf("GL_VERSION=%s\nGL_RENDERER=%s\n", glGetString(GL_VERSION), glGetString(GL_RENDERER));
    glViewport(0, 0, 640, 480);
    glClearColor(0.25f, 0.5f, 0.75f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    unsigned char pixel[4]{};
    glReadPixels(10, 10, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    assert(pixel[0] >= 62 && pixel[0] <= 65);
    assert(pixel[1] >= 126 && pixel[1] <= 129);
    assert(pixel[2] >= 190 && pixel[2] <= 193);
    assert(glGetError() == GL_NO_ERROR);
    int w, h; window.size(w, h); assert(w == 640 && h == 480);
    assert(window.displayMode(800, 600, true, false));
    window.size(w, h); assert(w == 800 && h == 600);
    SDL_ShowWindow(window.handle());
    window.pump();
    SDL_DisplayMode desktop{};assert(SDL_GetDesktopDisplayMode(0,&desktop)==0);
    assert(window.displayMode(desktop.w,desktop.h,false,true));
    assert(!window.windowed());
    assert((SDL_GetWindowFlags(window.handle())&SDL_WINDOW_FULLSCREEN_DESKTOP)==SDL_WINDOW_FULLSCREEN_DESKTOP);
    window.size(w,h);assert(w==desktop.w && h==desktop.h);
    assert(window.displayMode(800,600,true,false));assert(window.windowed());
    SDL_Event hotkey{};hotkey.type=SDL_KEYDOWN;hotkey.key.windowID=SDL_GetWindowID(window.handle());
    hotkey.key.keysym.scancode=SDL_SCANCODE_RETURN;hotkey.key.keysym.mod=KMOD_ALT;
    assert(SDL_PushEvent(&hotkey)==1);
    hotkey.key.repeat=1;assert(SDL_PushEvent(&hotkey)==1);
    hotkey.type=SDL_KEYUP;hotkey.key.repeat=0;hotkey.key.keysym.mod=0;assert(SDL_PushEvent(&hotkey)==1);
    window.pump();assert(window.takeFullscreenToggleRequest());assert(!window.takeFullscreenToggleRequest());
    for(const auto& event:window.events())assert(event.type!=SDL_KEYDOWN && event.type!=SDL_KEYUP);
    SDL_Event e{};
    e.type = SDL_KEYDOWN; e.key.windowID = SDL_GetWindowID(window.handle());
    e.key.keysym.scancode = SDL_SCANCODE_W;
    assert(SDL_PushEvent(&e) == 1);
    e = {}; e.type = SDL_WINDOWEVENT; e.window.windowID = SDL_GetWindowID(window.handle());
    e.window.event = SDL_WINDOWEVENT_FOCUS_LOST;
    assert(SDL_PushEvent(&e) == 1);
    window.pump(); assert(!window.focused());
    bool found = false;
    for (const auto& event : window.events()) if (event.type == SDL_KEYDOWN && event.key.keysym.scancode == SDL_SCANCODE_W) found = true;
    assert(found);
    window.present();
    e = {}; e.type = SDL_WINDOWEVENT; e.window.windowID = SDL_GetWindowID(window.handle());
    e.window.event = SDL_WINDOWEVENT_CLOSE;
    assert(SDL_PushEvent(&e) == 1);
    window.pump(); assert(window.closed());
    window.destroy(); window.destroy();
    assert(window.create("Recreate context", 320, 240, true));
    assert(window.createContext());
    assert(glGetString(GL_VERSION));
    window.destroy();
    puts("PASS: i386 SDL window + real OpenGL pixels + desktop fullscreen/return + consumed nonrepeating Alt+Enter + resize + key/focus/close + context recreation (not gameplay)");
}

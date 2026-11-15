#include "NativeWindow.h"
#include <algorithm>

namespace BarnyardLinux {
NativeWindow::~NativeWindow() { destroy(); }
bool NativeWindow::fail(const char* action) {
    error_ = std::string(action) + ": " + SDL_GetError();
    return false;
}
bool NativeWindow::create(const char* title, int width, int height, bool hidden) {
    if (window_ || width <= 0 || height <= 0) { error_ = "invalid window creation"; return false; }
    SDL_setenv("SDL_VIDEO_X11_WMCLASS","barnyard-linux",1);
    SDL_SetHint(SDL_HINT_APP_NAME,"Barnyard Linux");
    // Balanced subsystem reference even if another subsystem already uses SDL video.
    if (SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) return fail("SDL_InitSubSystem");
    ownsVideo_ = true;
    SDL_SetHint(SDL_HINT_GRAB_KEYBOARD,"0"); // Alt+Tab must remain available.
    SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS,"0");
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    window_ = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height, SDL_WINDOW_OPENGL | (hidden ? SDL_WINDOW_HIDDEN : SDL_WINDOW_SHOWN));
    if (!window_) { fail("SDL_CreateWindow"); destroy(); return false; }
    // Application icon only; never alters retail in-game artwork. No new SDL_image dependency.
    if(SDL_Surface* icon=SDL_LoadBMP("/usr/share/barnyard-linux/icon.bmp")) {
        SDL_SetWindowIcon(window_,icon);SDL_FreeSurface(icon);
    } else SDL_ClearError(); // Optional when running an unpackaged development binary.
    fullscreenToggle_ = suppressReturn_ = false;
    closed_ = false;
    focused_ = (SDL_GetWindowFlags(window_) & SDL_WINDOW_INPUT_FOCUS) != 0;
    minimized_ = false;
    windowed_ = true;
    error_.clear();
    return true;
}
bool NativeWindow::createContext() {
    if (!window_) { error_ = "no window for GL context"; return false; }
    if (context_) return true;
    context_ = SDL_GL_CreateContext(window_);
    if (!context_) return fail("SDL_GL_CreateContext");
    if (SDL_GL_MakeCurrent(window_, context_) != 0) {
        fail("SDL_GL_MakeCurrent"); destroyContext(); return false;
    }
    swapInterval_ = SDL_GL_SetSwapInterval(1) == 0;
    deadline_ = 0;
    return true;
}
void NativeWindow::destroyContext() {
    if (context_) SDL_GL_DeleteContext(context_);
    context_ = nullptr;
}
void NativeWindow::destroy() {
    destroyContext();
    if (window_) {
        SDL_SetRelativeMouseMode(SDL_FALSE);
        SDL_SetWindowGrab(window_,SDL_FALSE);
        SDL_ShowCursor(SDL_ENABLE);
        SDL_DestroyWindow(window_);
    }
    window_ = nullptr;
    closed_ = true; focused_ = false;
    events_.clear();
    if (ownsVideo_) SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    ownsVideo_ = false;
}
void NativeWindow::pump() {
    events_.clear();
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) closed_ = true;
        if (window_ && event.type == SDL_WINDOWEVENT && event.window.windowID == SDL_GetWindowID(window_)) {
            switch (event.window.event) {
                case SDL_WINDOWEVENT_CLOSE: closed_ = true; break;
                case SDL_WINDOWEVENT_FOCUS_GAINED: focused_ = true; break;
                case SDL_WINDOWEVENT_FOCUS_LOST: focused_ = false; break;
                case SDL_WINDOWEVENT_MINIMIZED: minimized_ = true; break;
                case SDL_WINDOWEVENT_RESTORED: minimized_ = false; break;
            }
        }
        // Consume the fullscreen shortcut before the game can treat Return as
        // menu confirmation. Execute the actual display change at game level,
        // where all world/GUI viewports can be updated together.
        if(window_ && (event.type==SDL_KEYDOWN || event.type==SDL_KEYUP) &&
                event.key.windowID==SDL_GetWindowID(window_) &&
                event.key.keysym.scancode==SDL_SCANCODE_RETURN) {
            if(event.type==SDL_KEYDOWN && (event.key.keysym.mod & KMOD_ALT)) {
                if(!event.key.repeat) fullscreenToggle_=!fullscreenToggle_;
                suppressReturn_=true;
                continue;
            }
            if(suppressReturn_) {
                if(event.type==SDL_KEYUP) suppressReturn_=false;
                continue;
            }
        }
        // Retain only input/window/controller events; never retain pointer-owning
        // drop or user events, which would have unclear ownership here.
        if (event.type == SDL_TEXTEDITING_EXT)
            SDL_free(event.editExt.text);
        else if (event.type >= SDL_KEYDOWN && event.type <= SDL_CONTROLLERDEVICEREMAPPED)
            events_.push_back(event);
        else if (event.type == SDL_WINDOWEVENT || event.type == SDL_QUIT)
            events_.push_back(event);
        else if (event.type == SDL_DROPFILE || event.type == SDL_DROPTEXT)
            SDL_free(event.drop.file);
    }
}
bool NativeWindow::displayMode(int width, int height, bool windowed, bool borderless) {
    if (!window_ || width <= 0 || height <= 0) { error_ = "invalid display mode"; return false; }
    if (windowed) {
        if (SDL_SetWindowFullscreen(window_, 0) != 0) return fail("leave fullscreen");
        SDL_SetWindowBordered(window_, borderless ? SDL_FALSE : SDL_TRUE);
        SDL_SetWindowSize(window_, width, height);
        SDL_SetWindowPosition(window_, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    } else if (borderless) {
        if (SDL_SetWindowFullscreen(window_, SDL_WINDOW_FULLSCREEN_DESKTOP) != 0) return fail("desktop fullscreen");
    } else {
        SDL_DisplayMode requested{}, selected{};
        requested.w = width; requested.h = height;
        int display = SDL_GetWindowDisplayIndex(window_);
        if (!SDL_GetClosestDisplayMode(display, &requested, &selected)) return fail("find fullscreen mode");
        if (SDL_SetWindowDisplayMode(window_, &selected) != 0) return fail("select fullscreen mode");
        if (SDL_SetWindowFullscreen(window_, SDL_WINDOW_FULLSCREEN) != 0) return fail("enter fullscreen");
    }
    windowed_ = windowed;
    return true;
}
void NativeWindow::present() {
    if (!window_ || !context_ || closed_) return;
    SDL_GL_SwapWindow(window_);
    // Some X11 drivers lack swap control. Avoid pegging an X270 CPU core there.
    if (!swapInterval_) {
        const Uint64 now = SDL_GetPerformanceCounter(), freq = SDL_GetPerformanceFrequency();
        if (deadline_ > now) SDL_Delay(static_cast<Uint32>((deadline_ - now) * 1000 / freq));
        deadline_ = SDL_GetPerformanceCounter() + freq / 60;
    }
}
bool NativeWindow::extension(const char* name) const {
    return context_ && name && SDL_GL_ExtensionSupported(name) == SDL_TRUE;
}
void NativeWindow::centerCursor() {
    int w, h; size(w, h);
    if (window_ && SDL_GetRelativeMouseMode()==SDL_FALSE) SDL_WarpMouseInWindow(window_, w / 2, h / 2);
}
void NativeWindow::size(int& w, int& h) const {
    w = h = 0;
    if (window_) SDL_GetWindowSize(window_, &w, &h);
}
}

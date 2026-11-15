#pragma once
#include <SDL2/SDL.h>
#include <string>
#include <vector>

namespace BarnyardLinux {
// Native SDL2/OpenGL backend shared by the engine adapter and platform tests.
// Owns one window/context on its creating thread. Not a Barnyard renderer.
class NativeWindow {
public:
    NativeWindow() = default;
    NativeWindow(const NativeWindow&) = delete;
    NativeWindow& operator=(const NativeWindow&) = delete;
    ~NativeWindow();
    bool create(const char* title, int width, int height, bool hidden = false);
    bool createContext();
    void destroyContext();
    void destroy();
    void pump();
    bool displayMode(int width, int height, bool windowed, bool borderless);
    bool takeFullscreenToggleRequest() { bool value=fullscreenToggle_; fullscreenToggle_=false; return value; }
    void present();
    bool extension(const char* name) const;
    void centerCursor();
    SDL_Window* handle() const { return window_; }
    bool closed() const { return closed_; }
    bool focused() const { return focused_; }
    bool minimized() const { return minimized_; }
    bool windowed() const { return windowed_; }
    const std::vector<SDL_Event>& events() const { return events_; }
    const std::string& error() const { return error_; }
    void size(int& w, int& h) const;
private:
    bool fail(const char* action);
    SDL_Window* window_ = nullptr;
    SDL_GLContext context_ = nullptr;
    bool ownsVideo_ = false, closed_ = true, focused_ = false, minimized_ = false, windowed_ = true;
    bool fullscreenToggle_ = false, suppressReturn_ = false;
    bool swapInterval_ = false;
    Uint64 deadline_ = 0;
    std::string error_;
    std::vector<SDL_Event> events_;
};
}

#pragma once
#include "Toshi/TObject.h"
#include "NativeWindow.h"

TOSHI_NAMESPACE_START
class TRenderInterface;
class TWindow_Linux : public TObject {
public:
    TDECLARE_CLASS(TWindow_Linux, TObject);
    TWindow_Linux() = default;
    ~TWindow_Linux() override;
    TBOOL Create(TRenderInterface* renderer, const TCHAR* title);
    TBOOL CreateContext();
    void DestroyContext() { m_native.destroyContext(); }
    void UnregisterWindowClass() { m_native.destroy(); }
    void Update(TFLOAT dt);
    void SwapBuffers() { m_native.present(); }
    TBOOL IsExtensionSupported(const TCHAR* name) const { return m_native.extension(name); }
    TBOOL SetDisplayMode(TUINT w, TUINT h, TBOOL windowed, TBOOL borderless);
    void RestoreDisplayMode();
    void CenterCursor(TUINT, TUINT) { m_native.centerCursor(); }
    TBOOL IsDestroyed() const { return m_native.closed(); }
    TBOOL IsWindowed() const { return m_native.windowed(); }
    TBOOL TakeFullscreenToggleRequest() { return m_native.takeFullscreenToggleRequest(); }
    SDL_Window* GetSDLWindow() const { return m_native.handle(); }
    const std::vector<SDL_Event>& GetEvents() const { return m_native.events(); }
    static TBOOL IsBackgroundMode() { return TFALSE; }
    static TBOOL IsNoPauseMode() { return TFALSE; }
    static void SetFocused(TBOOL focused) { ms_focused = focused; }
    static void Dev_SetKeepRunning(TBOOL keepRunning);
private:
    void SyncPause();
    BarnyardLinux::NativeWindow m_native;
    TRenderInterface* m_renderer = nullptr;
    bool m_pausedByWindow = false;
    inline static bool ms_focused = true, ms_keepRunning = false;
    inline static TWindow_Linux* ms_active = nullptr;
};
TOSHI_NAMESPACE_END

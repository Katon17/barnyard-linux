#pragma once
#include "Input/TInputInterface.h"
#include <SDL2/SDL.h>

TOSHI_NAMESPACE_START
class TInputLinuxDeviceKeyboard;
class TInputLinuxDeviceMouse;
// Dispatch/ProcessEvents run on the same main thread as the SDL window.
class TInputLinuxInterface : public TInputInterface {
public:
    TDECLARE_CLASS(TInputLinuxInterface, TInputInterface);
    ~TInputLinuxInterface() override;
    TBOOL Initialise() override;
    TBOOL Deinitialise() override;
    TBOOL AcquireAll() override;
    TBOOL UnacquireAll() override;
    void SetExclusiveMode(TBOOL exclusive) override;
    static void DispatchEvent(const SDL_Event& event);
private:
    void HandleEvent(const SDL_Event& event);
    TInputLinuxDeviceKeyboard* m_keyboard = nullptr;
    TInputLinuxDeviceMouse* m_mouse = nullptr;
    bool m_focused = true;
    bool m_captureRequested = true, m_mouseReleased = false, m_captureShortcutDown = false;
    void UpdateMouseCapture();
};
TOSHI_NAMESPACE_END

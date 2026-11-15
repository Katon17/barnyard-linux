#include "ToshiPCH.h"
#include "TInputInterface_Linux.h"
#include "TInputDeviceKeyboard_Linux.h"
#include "TInputDeviceMouse_Linux.h"

TOSHI_NAMESPACE_START
TDEFINE_CLASS(TInputLinuxInterface);
TInputLinuxInterface::~TInputLinuxInterface() { Deinitialise(); }
TBOOL TInputLinuxInterface::Initialise() {
    if (m_keyboard || m_mouse) return TFALSE;
    const char* capture=getenv("BARNYARD_CAPTURE_MOUSE");
    m_captureRequested=!capture || strcmp(capture,"0")!=0;
    m_mouseReleased=false;
    m_keyboard = new TInputLinuxDeviceKeyboard;
    m_mouse = new TInputLinuxDeviceMouse;
    AddDevice(m_keyboard); AddDevice(m_mouse);
    return AcquireAll();
}
TBOOL TInputLinuxInterface::Deinitialise() {
    if (m_keyboard || m_mouse) {
        UnacquireAll();
        ReleaseAllDevices();
        m_keyboard = nullptr; m_mouse = nullptr;
    }
    return TTRUE;
}
void TInputLinuxInterface::UpdateMouseCapture() {
    bool capture = m_mouse && m_mouse->IsAcquired() && m_focused &&
        (GetExclusiveMode() || m_captureRequested) && !m_mouseReleased;
    if(!SDL_WasInit(SDL_INIT_VIDEO)) return;
    auto* window=SDL_GetKeyboardFocus();
    if(!window) capture=false;
    if(SDL_GetRelativeMouseMode()!=(capture ? SDL_TRUE : SDL_FALSE) &&
            SDL_SetRelativeMouseMode(capture ? SDL_TRUE : SDL_FALSE)!=0 && capture) {
        fprintf(stderr,"SDL relative mouse unavailable: %s\n",SDL_GetError());
        capture=false;
    }
    if(window) SDL_SetWindowGrab(window,capture ? SDL_TRUE : SDL_FALSE);
    else if(auto* grabbed=SDL_GetGrabbedWindow()) SDL_SetWindowGrab(grabbed,SDL_FALSE);
    SDL_ShowCursor(capture ? SDL_DISABLE : SDL_ENABLE);
}
TBOOL TInputLinuxInterface::AcquireAll() {
    TBOOL result = TInputInterface::AcquireAll();
    UpdateMouseCapture();
    return result;
}
TBOOL TInputLinuxInterface::UnacquireAll() {
    TBOOL result = TInputInterface::UnacquireAll();
    UpdateMouseCapture();
    return result;
}
void TInputLinuxInterface::SetExclusiveMode(TBOOL exclusive) {
    TInputInterface::SetExclusiveMode(exclusive);
    UpdateMouseCapture();
}
void TInputLinuxInterface::DispatchEvent(const SDL_Event& event) {
    if (!TInputInterface::IsSingletonCreated()) return;
    auto* input = TInputInterface::GetSingletonSafe();
    if (input->IsA(&TGetClass(TInputLinuxInterface)))
        static_cast<TInputLinuxInterface*>(input)->HandleEvent(event);
}
void TInputLinuxInterface::HandleEvent(const SDL_Event& event) {
    if (!m_keyboard || !m_mouse) return;
    if (event.type == SDL_QUIT || (event.type == SDL_WINDOWEVENT &&
        (event.window.event == SDL_WINDOWEVENT_FOCUS_LOST || event.window.event == SDL_WINDOWEVENT_MINIMIZED ||
         event.window.event == SDL_WINDOWEVENT_CLOSE))) {
        m_keyboard->ReleaseKeys(); m_mouse->ReleaseButtons();
        m_focused = false;
        UpdateMouseCapture();
    } else if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
        m_focused = true;
        UpdateMouseCapture();
    } else if (m_focused) {
        if((event.type==SDL_KEYDOWN || event.type==SDL_KEYUP) &&
                event.key.keysym.scancode==SDL_SCANCODE_F1) {
            if(event.type==SDL_KEYDOWN && (event.key.keysym.mod & KMOD_SHIFT)) {
                if(!event.key.repeat) {
                    bool enabled=(GetExclusiveMode() || m_captureRequested) && !m_mouseReleased;
                    m_mouseReleased=enabled;
                    if(!enabled) m_captureRequested=true;
                    m_mouse->ReleaseButtons();
                    UpdateMouseCapture();
                    fprintf(stderr,"Mouse capture %s (Shift+F1 toggles; click game to recapture)\n",enabled?"released":"requested");
                }
                m_captureShortcutDown=true;
                return;
            }
            if(m_captureShortcutDown) {
                if(event.type==SDL_KEYUP) m_captureShortcutDown=false;
                return;
            }
        }
        if(m_mouseReleased && event.type>=SDL_MOUSEMOTION && event.type<=SDL_MOUSEWHEEL) {
            if(event.type==SDL_MOUSEBUTTONDOWN && m_captureRequested) {
                m_mouseReleased=false;
                UpdateMouseCapture();
            }
            return; // release/capture click must not activate an in-game action.
        }
        m_keyboard->HandleEvent(event); m_mouse->HandleEvent(event);
    }
}
TOSHI_NAMESPACE_END

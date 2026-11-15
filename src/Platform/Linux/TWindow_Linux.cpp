#include "ToshiPCH.h"
#include "Platform/GL/TGLHeaders.h"
#include "TWindow_Linux.h"
#include "Platform/GL/TRenderInterface_GL.h"
#include "Toshi/TApplication.h"
#include "Toshi/TSystem.h"
#include "Input/TInputInterface_Linux.h"

TOSHI_NAMESPACE_START
TDEFINE_CLASS(TWindow_Linux);
TWindow_Linux::~TWindow_Linux() {
    if (ms_active == this) ms_active = nullptr;
}
TBOOL TWindow_Linux::Create(TRenderInterface* renderer, const TCHAR* title) {
    m_renderer = renderer;
    if (!m_native.create(title, 800, 600)) {
        fprintf(stderr, "%s\n", m_native.error().c_str());
        return TFALSE;
    }
    ms_active = this;
    return TTRUE;
}
TBOOL TWindow_Linux::CreateContext() {
    if (!m_native.createContext()) {
        fprintf(stderr, "%s\n", m_native.error().c_str());
        return TFALSE;
    }
    glewExperimental = GL_TRUE;
    GLenum result = glewInit();
    // GLEW can probe a legacy extension string in core contexts.
    while (glGetError() != GL_NO_ERROR) {}
    if (result != GLEW_OK || !GLEW_VERSION_3_3) {
        fprintf(stderr, "OpenGL 3.3 loader unavailable: %s\n", glewGetErrorString(result));
        m_native.destroyContext();
        return TFALSE;
    }
    return TTRUE;
}
void TWindow_Linux::SyncPause() {
    bool pause = (!ms_focused || m_native.minimized()) && !ms_keepRunning;
    if (pause && !m_pausedByWindow && !g_oSystemManager.IsPaused()) {
        g_oSystemManager.Pause(TTRUE);
        m_pausedByWindow = true;
    } else if (!pause && m_pausedByWindow) {
        g_oSystemManager.Pause(TFALSE);
        m_pausedByWindow = false;
    }
}
void TWindow_Linux::Dev_SetKeepRunning(TBOOL keepRunning) {
    ms_keepRunning = keepRunning;
    if (ms_active) ms_active->SyncPause();
}
void TWindow_Linux::Update(TFLOAT) {
    m_native.pump();
    for (const auto& event : m_native.events()) TInputLinuxInterface::DispatchEvent(event);
    SetFocused(m_native.focused());
    if (m_native.closed()) {
        static_cast<TRenderGLInterface*>(m_renderer)->Exit();
        TGlobalEmitter<TApplicationExitEvent>::Throw(TApplicationExitEvent{TFALSE});
        return;
    }
    SyncPause();
    if (m_native.minimized()) SDL_Delay(20);
}
TBOOL TWindow_Linux::SetDisplayMode(TUINT w, TUINT h, TBOOL windowed, TBOOL borderless) {
    if (!m_native.displayMode(w, h, windowed, borderless)) {
        fprintf(stderr, "%s\n", m_native.error().c_str());
        return TFALSE;
    }
    return TTRUE;
}
void TWindow_Linux::RestoreDisplayMode() {
    int w, h; m_native.size(w, h);
    if (w > 0 && h > 0) m_native.displayMode(w, h, true, false);
}
TOSHI_NAMESPACE_END

#pragma once
#include "Toshi/TUtil.h"
#include "Toshi/TEvent.h"

TOSHI_NAMESPACE_START
// Terminal/log sink, not a Win32 console window and not an interactive command UI.
class TDebugConsole {
public:
    TDebugConsole();
    ~TDebugConsole() = default;
    TBOOL Show(TBOOL state) { TBOOL old = m_visible; m_visible = state; return old; }
    TBOOL Toggle() { Show(!m_visible); return m_visible; }
    TBOOL IsVisible() const { return m_visible; }
    TBOOL IsCreated() const { return m_created; }
private:
    static TBOOL __stdcall OnLog(TDebugConsole* caller, TUtil*, TUtil::LogEvent* event);
    TBOOL m_visible = TFALSE, m_created = TFALSE;
    TListener<TUtil, TUtil::LogEvent, TDebugConsole> m_listener;
};
TOSHI_NAMESPACE_END

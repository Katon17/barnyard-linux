#include "ToshiPCH.h"
#include "TDebugConsole_Linux.h"
#include <cstdio>

TOSHI_NAMESPACE_START
TDebugConsole::TDebugConsole() {
    if (TUtil::IsSingletonCreated()) {
        m_listener.Connect(TUtil::GetLogEmitter(), this, OnLog, 1);
        m_created = TTRUE;
    }
}
TBOOL __stdcall TDebugConsole::OnLog(TDebugConsole* caller, TUtil*, TUtil::LogEvent* event) {
    if (caller->m_visible && event && event->m_szString) {
        // The message is data, never a second printf format string.
        fprintf(stderr, "[%s] %s", TUtil::LogTypeToString(event->m_eType), event->m_szString);
    }
    return TTRUE;
}
TOSHI_NAMESPACE_END

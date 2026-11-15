#include <ToshiPCH.h>
#include <pthread.h>
#include <cstdio>
#include <cstdarg>
#include <cstdint>
namespace {
pthread_mutex_t mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
thread_local char scratch[4096];
}
void TDebug_AcquireMutex() { pthread_mutex_lock(&mutex); }
void TDebug_ReleaseMutex() { pthread_mutex_unlock(&mutex); }
TBOOL TDebug_IsValidAddress(const void* ptr) {
    // Same purpose as upstream: cheap debug heuristic, NOT a memory-safety check.
    auto value = uintptr_t(ptr);
    return value >= 4096 && value != 0xcdcdcdcdu && value != 0xdeaddeadu && value != 0xbeefbeefu;
}
void TDebug_FinalVSPrintf(const TCHAR* format, va_list args) { vsnprintf(scratch, sizeof(scratch), format, args); }
void TDebug_FinalSPrintf(const TCHAR* format, ...) {
    va_list args; va_start(args, format); TDebug_FinalVSPrintf(format, args); va_end(args);
}
void TDebug_OutputDebugString(const TCHAR* text) {
    TDebug_AcquireMutex(); fputs(text ? text : "(null)", stderr); TDebug_ReleaseMutex();
}
void TDebug_FinalVPrintf(const TCHAR* format, va_list args) {
    TDebug_FinalVSPrintf(format, args); TDebug_OutputDebugString(scratch);
}
void TDebug_FinalPrintf(const TCHAR* format, ...) {
    va_list args; va_start(args, format); TDebug_FinalVPrintf(format, args); va_end(args);
}
void TDebug_BroadcastDebugString(const TCHAR* text) { TDebug_OutputDebugString(text); }
TINT TDebug_AssertHandler(const TCHAR* expression, const TCHAR* file, TUINT line, const TCHAR* description) {
    TDebug_FinalPrintf("ASSERT: %s (%s:%u) %s\n", expression, file, line, description ? description : "");
    return 1; // TASSERT traps, never silently pretends a failed assertion passed.
}

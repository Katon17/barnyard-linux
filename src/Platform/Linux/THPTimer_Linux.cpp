#include "THPTimer_Linux.h"
#include <cstdlib>
#include <time.h>
namespace {
TUINT64 microseconds() {
    timespec ts{};
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) std::abort();
    return TUINT64(ts.tv_sec) * 1000000ull + TUINT64(ts.tv_nsec) / 1000ull;
}
}
TOSHI_NAMESPACE_START
THPTimer::THPTimer() { Reset(); }
THPTimer::~THPTimer() = default;
TUINT32 THPTimer::GetRaw32() { return TUINT32(microseconds()); }
int64_t THPTimer::GetRaw64() { return int64_t(microseconds()); }
void THPTimer::Reset() {
    m_iFrequency = 1000000;
    m_iOldTime = m_iCurrentTime = microseconds();
    m_fCurrentSeconds = TFLOAT(double(m_iCurrentTime) / double(m_iFrequency));
    m_Unused = 0;
    m_fDelta = 0;
}
void THPTimer::Update() {
    m_iOldTime = m_iCurrentTime;
    m_iCurrentTime = microseconds();
    m_fDelta = TFLOAT(double(m_iCurrentTime - m_iOldTime) / double(m_iFrequency));
    m_fCurrentSeconds = TFLOAT(double(m_iCurrentTime) / double(m_iFrequency));
}
TOSHI_NAMESPACE_END

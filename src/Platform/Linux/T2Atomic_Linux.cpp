// Linux implementation of the upstream T2Atomic32 API (see header attribution).
#include "T2Atomic_Linux.h"
#include <cerrno>
#include <cstdlib>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <unistd.h>

TOSHI_NAMESPACE_START
TINT32 T2Atomic32::Load(MemoryOrders order) const {
    return __atomic_load_n(&m_iValue, order == MEMORY_ORDER_STRONG ? __ATOMIC_SEQ_CST : __ATOMIC_RELAXED);
}
TINT32 T2Atomic32::Exchange(TINT32 value) {
    return __atomic_exchange_n(&m_iValue, value, __ATOMIC_SEQ_CST);
}
TINT32 T2Atomic32::CompareExchange(TINT32 value, TINT32 compare) {
    __atomic_compare_exchange_n(&m_iValue, &compare, value, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
    return compare; // Upstream API returns OLD value, not a bool.
}
TINT32 T2Atomic32::Add(TINT32 value) { return __atomic_fetch_add(&m_iValue, value, __ATOMIC_SEQ_CST); }
TINT32 T2Atomic32::Or(TINT32 mask) { return __atomic_fetch_or(&m_iValue, mask, __ATOMIC_SEQ_CST); }
TINT32 T2Atomic32::And(TINT32 mask) { return __atomic_fetch_and(&m_iValue, mask, __ATOMIC_SEQ_CST); }
void T2Atomic32::WaitForChange() { WaitForChange(Load(MEMORY_ORDER_WEAK)); }
void T2Atomic32::WaitForChange(TINT32 value) {
    while (Load() == value) {
        // FUTEX_WAIT atomically checks the value before sleeping: no lost wakeup
        // between the load above and the kernel wait. Caller must Signal after
        // modifying the value, as required by the upstream contract.
        const long result = syscall(SYS_futex, &m_iValue, FUTEX_WAIT_PRIVATE, value, nullptr, nullptr, 0);
        if (result < 0 && errno != EINTR && errno != EAGAIN) std::abort();
    }
}
void T2Atomic32::Signal() {
    if (syscall(SYS_futex, &m_iValue, FUTEX_WAKE_PRIVATE, 1, nullptr, nullptr, 0) < 0) std::abort();
}
TOSHI_NAMESPACE_END

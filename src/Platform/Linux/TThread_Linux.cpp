#include "ToshiPCH.h"
#include "TThread_Linux.h"
#include <sys/syscall.h>
#include <unistd.h>
#include <sys/resource.h>
#include <cerrno>

TOSHI_NAMESPACE_START
static TThreadManager manager;
TThread::~TThread() {
    // A base destructor cannot safely join a worker accessing already-destroyed
    // derived members. Require explicit Stop/Destroy in the derived destructor.
    TASSERT(m_hThreadHnd == nullptr, "Linux thread must be joined before derived state is destroyed");
}
void* TThread::Entry(void* context) {
    auto* self = static_cast<TThread*>(context);
    {
        std::lock_guard lock(self->m_gate);
        self->m_iThreadID = static_cast<unsigned long>(syscall(SYS_gettid));
        self->m_started = true;
    }
    self->m_ready.notify_one();
    self->Main();
    return nullptr;
}
TBOOL TThread::Create(size_t stackSize, PRIORITY priority, FLAGS flags) {
    std::lock_guard lifecycle(m_lifecycle);
    // No implicit ignored flags or privileged realtime scheduling. Current game
    // asset-streamer uses NORMAL/NONE; unsupported requests fail explicitly.
    if (m_hThreadHnd || m_joining || flags != FLAGS_NONE || priority != THREAD_PRIORITY_NORMAL) return TFALSE;
    pthread_attr_t attr;
    if (pthread_attr_init(&attr) != 0) return TFALSE;
    if (stackSize && pthread_attr_setstacksize(&attr, stackSize) != 0) {
        pthread_attr_destroy(&attr); return TFALSE;
    }
    m_started = false;
    m_hThreadHnd = this;
    int result = pthread_create(&m_thread, &attr, Entry, this);
    pthread_attr_destroy(&attr);
    if (result != 0) { m_hThreadHnd = nullptr; return TFALSE; }
    {
        std::unique_lock lock(m_gate);
        m_ready.wait(lock, [&] { return m_started; });
    }
    manager.InsertThread(this);
    return TTRUE;
}
TBOOL TThread::Destroy() {
    std::unique_lock lifecycle(m_lifecycle);
    if (!m_hThreadHnd || m_joining || pthread_equal(pthread_self(), m_thread)) return TFALSE;
    m_joining = true;
    pthread_t joining = m_thread;
    lifecycle.unlock();
    int result = pthread_join(joining, nullptr);
    lifecycle.lock();
    m_joining = false;
    if (result != 0) return TFALSE;
    manager.RemoveThread(this);
    m_hThreadHnd = nullptr;
    m_iThreadID = 0;
    return TTRUE;
}
TBOOL TThread::GetPriority(void* handle, PRIORITY& priority) {
    if (!handle) return TFALSE;
    auto* thread = static_cast<TThread*>(handle);
    std::lock_guard lifecycle(thread->m_lifecycle);
    if (!thread->m_hThreadHnd) return TFALSE;
    errno = 0;
    int value = getpriority(PRIO_PROCESS, thread->m_iThreadID);
    if (errno || value != 0) return TFALSE;
    priority = THREAD_PRIORITY_NORMAL;
    return TTRUE;
}
TBOOL TThread::SetPriority(void* handle, PRIORITY priority) {
    // Verify rather than claim to have changed a scheduling policy we do not implement.
    PRIORITY actual;
    return priority == THREAD_PRIORITY_NORMAL && GetPriority(handle, actual);
}
void TThread::Exit(TThread* thread) {
    TASSERT(thread && pthread_equal(pthread_self(), thread->m_thread));
    pthread_exit(nullptr);
}
void TThreadManager::InsertThread(TThread* thread) {
    std::lock_guard lock(m_mutex);
    m_threads.PushFront(thread);
}
void TThreadManager::RemoveThread(TThread* thread) {
    std::lock_guard lock(m_mutex);
    thread->Remove();
}
void TThreadManager::Delete() {
    std::lock_guard lock(m_mutex);
    TASSERT(m_threads.IsEmpty(), "All native workers must be joined before thread manager shutdown");
}
TThreadManager::~TThreadManager() { Delete(); }
TOSHI_NAMESPACE_END

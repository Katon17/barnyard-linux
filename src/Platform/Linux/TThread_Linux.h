#pragma once
#include "Toshi/T2DList.h"
#include <pthread.h>
#include <mutex>
#include <condition_variable>

TOSHI_NAMESPACE_START
class TThread : public T2DList<TThread>::Node {
public:
    using PRIORITY = TINT;
    enum PRIORITY_ : PRIORITY {
        THREAD_PRIORITY_ABOVE_NORMAL=1, THREAD_PRIORITY_BELOW_NORMAL=-1,
        THREAD_PRIORITY_HIGHEST=2, THREAD_PRIORITY_IDLE=-15, THREAD_PRIORITY_LOWEST=-2,
        THREAD_PRIORITY_NORMAL=0, THREAD_PRIORITY_TIME_CRITICAL=15
    };
    using FLAGS = TUINT8;
    enum FLAGS_ : FLAGS { FLAGS_NONE=0, FLAGS_SUSPENDED=BITFLAG(0) };
    virtual void Main() = 0;
    virtual ~TThread();
    TBOOL Create(size_t stackSize, PRIORITY priority, FLAGS flags);
    // Cooperative join, never asynchronous termination. Subclass must request stop
    // BEFORE destroying its state. TFileStream::Stop implements that contract.
    TBOOL Destroy();
    static TBOOL GetPriority(void* handle, PRIORITY& priority);
    static TBOOL SetPriority(void* handle, PRIORITY priority);
    static void Exit(TThread* thread);
    void* m_hThreadHnd = nullptr;
    unsigned long m_iThreadID = 0;
private:
    static void* Entry(void* context);
    pthread_t m_thread{};
    std::mutex m_lifecycle;
    std::mutex m_gate;
    std::condition_variable m_ready;
    bool m_started = false, m_joining = false;
};
class TThreadManager : public TSingleton<TThreadManager> {
public:
    TThreadManager() = default;
    ~TThreadManager();
    void Create() {}
    void Delete();
    void RemoveThread(TThread* thread);
    void InsertThread(TThread* thread);
private:
    std::mutex m_mutex;
    T2DList<TThread> m_threads;
};
TOSHI_NAMESPACE_END

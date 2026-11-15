#include <ToshiPCH.h>
#include <Toshi/Platform.h>
#include <time.h>
#include <sched.h>
#include <cerrno>
TOSHI_NAMESPACE_START
void ThreadSleep(TUINT milliseconds) {
    if(!milliseconds) {sched_yield();return;}
    timespec request{time_t(milliseconds/1000),long(milliseconds%1000)*1000000};
    while(nanosleep(&request,&request) && errno==EINTR) {}
}
TOSHI_NAMESPACE_END

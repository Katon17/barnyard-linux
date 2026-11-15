#include "NativeStartup.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cerrno>
#include <thread>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <unistd.h>

namespace BarnyardLinux {
std::string commandLine(int argc, char* const* argv) {
    std::string result;
    for (int i = 0; i < argc; ++i) {
        if (i) result += ' ';
        result += '"';
        for (const char* p = argv[i]; p && *p; ++p) {
            if (*p == '"' || *p == '\\') result += '\\';
            result += *p;
        }
        result += '"';
    }
    return result;
}
bool hasArgument(int argc, char* const* argv, const char* flag) {
    for (int i = 1; i < argc; ++i) if (argv[i] && strcmp(argv[i], flag) == 0) return true;
    return false;
}
InstanceLock::~InstanceLock() { if (fd_ >= 0) close(fd_); }
InstanceLock::Result InstanceLock::acquire(const char* userDirectory) {
    if (fd_ >= 0 || !userDirectory || userDirectory[0] != '/') {
        error_ = "instance lock requires an absolute, existing BARNYARD_USER_DIR";
        return Result::Error;
    }
    // Do not create directories here: the native mount validates and creates
    // the user root before acquiring this lock. The lock is never in game data.
    int root = open(userDirectory, O_DIRECTORY | O_RDONLY | O_CLOEXEC | O_NOFOLLOW);
    if (root < 0) { error_ = strerror(errno); return Result::Error; }
    int fd = openat(root, ".barnyard.instance.lock", O_CREAT | O_RDWR | O_CLOEXEC | O_NOFOLLOW, 0600);
    int saved = errno;
    close(root);
    if (fd < 0) { error_ = strerror(saved); return Result::Error; }
    struct stat info{};
    if (fstat(fd, &info) != 0 || !S_ISREG(info.st_mode) || info.st_nlink != 1 || info.st_uid != getuid()) {
        close(fd); error_ = "unsafe instance lock file"; return Result::Error;
    }
    if (flock(fd, LOCK_EX | LOCK_NB) != 0) {
        saved = errno; close(fd);
        if (saved == EWOULDBLOCK || saved == EAGAIN) return Result::AlreadyRunning;
        error_ = strerror(saved); return Result::Error;
    }
    fd_ = fd;
    error_.clear();
    return Result::Acquired;
}
void FramePacer::wait(float fps) {
    if (!std::isfinite(fps) || fps <= 0) { limit_ = 0; next_ = {}; return; }
    // Bound pathological console input before duration conversion.
    fps = std::clamp(fps, 1.0f, 1000.0f);
    auto period = std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(1.0 / fps));
    auto now = Clock::now();
    if (fps != limit_ || next_ == Clock::time_point{} || now > next_ + period) {
        limit_ = fps; next_ = now + period; return;
    }
    if (now < next_) std::this_thread::sleep_until(next_);
    next_ += period;
}
}

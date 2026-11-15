#pragma once
#include <chrono>
#include <string>

namespace BarnyardLinux {
// A diagnostic representation only. Linux main passes the original argc/argv
// to the game; it never round-trips argv through the Windows command-line parser.
std::string commandLine(int argc, char* const* argv);
bool hasArgument(int argc, char* const* argv, const char* flag);

class InstanceLock {
public:
    enum class Result { Acquired, AlreadyRunning, Error };
    InstanceLock() = default;
    InstanceLock(const InstanceLock&) = delete;
    InstanceLock& operator=(const InstanceLock&) = delete;
    ~InstanceLock();
    Result acquire(const char* userDirectory);
    const std::string& error() const { return error_; }
private:
    int fd_ = -1;
    std::string error_;
};

// Sleep-based steady-clock pacing, with no busy spin. An invalid or zero FPS
// disables this optional limiter. SDL swap interval can provide the default cap.
class FramePacer {
public:
    using Clock = std::chrono::steady_clock;
    void wait(float framesPerSecond);
private:
    float limit_ = 0;
    Clock::time_point next_{};
};
}

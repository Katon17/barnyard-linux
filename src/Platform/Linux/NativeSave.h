#pragma once
#include <cstddef>
#include <filesystem>
#include <string>

namespace BarnyardLinux {
// Descriptor-relative user-only save storage. Absolute paths are accepted only
// inside the configured user root; no component may be a symlink or '..'.
class SaveStore {
public:
    enum class Status { Missing, Regular, Error };
    SaveStore(const char* userRoot, const char* gameRoot);
    ~SaveStore();
    SaveStore(const SaveStore&) = delete;
    SaveStore& operator=(const SaveStore&) = delete;
    bool valid() const { return rootFd_ >= 0; }
    const std::string& error() const { return error_; }
    std::string directory() const;
    Status status(const char* path);
    bool readExact(const char* path, void* data, size_t size);
    // Same-directory temporary + fsync + rename + directory fsync. A failure
    // after rename is conservatively reported as failure (commit may exist).
    bool writeAtomic(const char* path, const void* data, size_t size);
private:
    int parent(const char* path, bool create, std::string& leaf);
    bool fail(const char* operation);
    int rootFd_ = -1;
    std::filesystem::path root_;
    std::string error_;
};
}

#pragma once
#include <filesystem>
#include <stdexcept>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace BarnyardLinux {
class AssetMissing : public std::runtime_error { public: using std::runtime_error::runtime_error; };
// Read-only resolver for game data, not for saves. Directory contents are assumed
// immutable for the session; clear() invalidates cached listings after changes.
// ASCII case folding intentionally matches the observed English asset paths.
class AssetPaths {
public:
    explicit AssetPaths(const std::filesystem::path& root);
    std::filesystem::path resolve(std::string relative);
    void clear();
private:
    using Listing = std::unordered_map<std::string, std::vector<std::string>>;
    static std::string fold(std::string value);
    bool inside(const std::filesystem::path& value) const;
    std::filesystem::path root_;
    std::mutex mutex_;
    std::unordered_map<std::string, Listing> directories_;
};
}

#include "AssetPaths.h"
#include <algorithm>
#include <stdexcept>

namespace fs = std::filesystem;
namespace BarnyardLinux {
std::string AssetPaths::fold(std::string value) {
    for (auto& c : value) if (c >= 'A' && c <= 'Z') c = char(c + ('a' - 'A'));
    return value;
}
AssetPaths::AssetPaths(const fs::path& root) : root_(fs::canonical(root)) {
    if (!fs::is_directory(root_)) throw std::runtime_error("Asset root is not a directory");
}
bool AssetPaths::inside(const fs::path& value) const {
    auto r = root_.begin();
    auto v = value.begin();
    for (; r != root_.end(); ++r, ++v) if (v == value.end() || *r != *v) return false;
    return true;
}
void AssetPaths::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    directories_.clear();
}
fs::path AssetPaths::resolve(std::string relative) {
    if (relative.find('\0') != std::string::npos) throw std::runtime_error("NUL in asset path");
    std::replace(relative.begin(), relative.end(), '\\', '/');
    fs::path input(relative);
    if (relative.empty() || input.is_absolute() || relative.find(':') != std::string::npos)
        throw std::runtime_error("Expected a non-empty relative asset path");
    // Reject parent traversal even if lexical normalization would cancel it.
    for (const auto& part : input) if (part == "..") throw std::runtime_error("Parent traversal rejected");
    std::lock_guard<std::mutex> lock(mutex_);
    auto current = root_;
    for (const auto& part : input) {
        if (part.empty() || part == ".") continue;
        const auto name = part.string();
        auto [it, inserted] = directories_.try_emplace(current.string());
        if (inserted) {
            try {
                for (const auto& entry : fs::directory_iterator(current)) {
                    const auto filename = entry.path().filename().string();
                    it->second[fold(filename)].push_back(filename);
                }
            } catch (...) { directories_.erase(it); throw; }
        }
        const auto found = it->second.find(fold(name));
        if (found == it->second.end()) throw AssetMissing("Missing asset component: " + name);
        // Refuse case collisions, even if one exact match exists. Otherwise the
        // same retail path could silently load a different asset on another host.
        if (found->second.size() != 1) throw std::runtime_error("Ambiguous case collision: " + name);
        current = fs::canonical(current / found->second.front());
        if (!inside(current)) throw std::runtime_error("Asset symlink escapes root");
    }
    return current;
}
}

#include "ToshiPCH.h"
#include "Toshi/TUtil.h"
#include <ctime>
#include <vector>
#include <algorithm>
#include <string>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fnmatch.h>

TOSHI_NAMESPACE_START
const TCHAR* TUtil::GetTime() {
    thread_local char buffer[64];
    time_t now = time(nullptr);
    tm local{};
    if (!localtime_r(&now, &local) || !strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &local))
        snprintf(buffer, sizeof(buffer), "time unavailable");
    return buffer;
}
void TUtil::TrimLog(const TCHAR* pattern, TSIZE retain) {
    // Linux logs live only in the user overlay, never enumerate/delete game files.
    const char* user = getenv("BARNYARD_USER_DIR");
    if (!user || !pattern || strchr(pattern, '/') || strchr(pattern, '\\')) return;
    int root = open(user, O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    if (root < 0) return;
    int logs = openat(root, "Logs", O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
    close(root);
    if (logs < 0) return; // no previous logs yet
    DIR* directory = fdopendir(logs);
    if (!directory) { close(logs); return; }
    struct Entry { std::string name; timespec modified; };
    std::vector<Entry> entries;
    while (auto* entry = readdir(directory)) {
        if (fnmatch(pattern, entry->d_name, FNM_PERIOD) != 0) continue;
        struct stat info{};
        if (fstatat(logs, entry->d_name, &info, AT_SYMLINK_NOFOLLOW) == 0 &&
            S_ISREG(info.st_mode) && info.st_nlink == 1 && info.st_uid == getuid())
            entries.push_back({entry->d_name, info.st_mtim});
    }
    std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) {
        if (a.modified.tv_sec != b.modified.tv_sec) return a.modified.tv_sec > b.modified.tv_sec;
        if (a.modified.tv_nsec != b.modified.tv_nsec) return a.modified.tv_nsec > b.modified.tv_nsec;
        return a.name > b.name;
    });
    for (size_t i = retain; i < entries.size(); ++i) unlinkat(logs, entries[i].name.c_str(), 0);
    closedir(directory);
}
TOSHI_NAMESPACE_END

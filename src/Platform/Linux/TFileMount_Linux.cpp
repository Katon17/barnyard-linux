#include <ToshiPCH.h>
#include "TNativeFile_Linux.h"
#include <File/TNullFile.h>
#include <File/TMemFile.h>
#include <cstdlib>
#include <filesystem>

TOSHI_NAMESPACE_START
TBOOL TFileManager::Create() {
    const char* game = getenv("BARNYARD_GAME_DIR");
    const char* user = getenv("BARNYARD_USER_DIR");
    if (!game || !*game || !user || !*user) {
        TDebug_OutputDebugString("Linux mounts require BARNYARD_GAME_DIR and BARNYARD_USER_DIR\n");
        return false;
    }
    try {
        const auto gamePath = std::filesystem::canonical(game);
        if (!std::filesystem::is_directory(gamePath)) return false;
        const auto userPath = std::filesystem::weakly_canonical(user);
        // Refuse writing back into the original asset tree (even a subfolder).
        auto relation = userPath.lexically_relative(gamePath);
        auto inverse = gamePath.lexically_relative(userPath);
        if (userPath == gamePath || (!relation.empty() && *relation.begin() != "..") ||
            (!inverse.empty() && *inverse.begin() != "..")) {
            TDebug_OutputDebugString("Game and user directories must be disjoint\n");
            return false;
        }
        std::filesystem::create_directories(userPath);
        CreateCommon();
        new TNativeFileSystem("local", gamePath.c_str(), userPath.c_str());
        new TNativeFileSystem("abs", gamePath.c_str(), userPath.c_str());
        new TNullFileSystem("null");
        new TMemFileSystem("mem");
        GetSingletonSafe()->SetSystemPath("local");
        return true;
    } catch (const std::exception& error) {
        TDebug_FinalPrintf("Linux mounts failed: %s\n", error.what());
        if (IsSingletonCreated()) Destroy();
        return false;
    }
}
void FixPathSlashes(TString8& path) {
    for (int i=0; i<path.Length(); ++i) if (path[i] == '/') path[i] = '\\';
}
void FixPathSlashesBack(TString8& path) {
    for (int i=0; i<path.Length(); ++i) if (path[i] == '\\') path[i] = '/';
}
TOSHI_NAMESPACE_END

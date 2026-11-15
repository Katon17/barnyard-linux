#include <ToshiPCH.h>
#include "TNativeFile_Linux.h"
#include "TStringHelper_Linux.h"
#include <algorithm>
#include <cerrno>
#include <climits>
#include <fcntl.h>
#include <map>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>

static_assert(sizeof(TWCHAR) == 2, "Linux engine file I/O requires UTF-16 wchar");
namespace fs = std::filesystem;
namespace {
std::string logicalPath(const Toshi::TString8& value) {
    std::string result(value.GetString() ? value.GetString() : "");
    std::replace(result.begin(), result.end(), '\\', '/');
    // TFileManager's leading slash denotes a virtual root, NOT host filesystem.
    while (!result.empty() && result.front() == '/') result.erase(0, 1);
    if (result.find(':') != std::string::npos) throw std::runtime_error("Invalid volume in local asset path");
    for (const auto& part : fs::path(result)) if (part == "..") throw std::runtime_error("Parent traversal rejected");
    return result.empty() ? "." : result;
}
std::string fold(std::string value) {
    for (auto& c : value) if (c >= 'A' && c <= 'Z') c += 'a'-'A';
    return value;
}
}

TOSHI_NAMESPACE_START
TNativeFileSystem::TNativeFileSystem(const TCHAR* name, const char* gameRoot, const char* userRoot)
    : TFileSystem(name), m_game(fs::canonical(gameRoot)), m_user(fs::canonical(userRoot)),
      m_gamePaths(m_game), m_userPaths(m_user) {
    TFileManager::GetSingletonSafe()->MountFileSystem(this);
}
TNativeFileSystem::~TNativeFileSystem() { UnmountFileSystem(); }
fs::path TNativeFileSystem::Resolve(const TString8& input, bool writing, bool directory) {
    const auto relative = logicalPath(input);
    if (!writing) {
        try { return m_userPaths.resolve(relative); }
        catch (const BarnyardLinux::AssetMissing&) { return m_gamePaths.resolve(relative); }
    }
    // Save/config writes go ONLY to the user root. Match existing case, create
    // missing directories one component at a time, then re-resolve each path.
    auto parts = fs::path(relative);
    fs::path prefix;
    auto current = m_user;
    for (auto it = parts.begin(); it != parts.end(); ++it) {
        if (*it == "." || it->empty()) continue;
        prefix /= *it;
        auto next = it;
        ++next;
        try { current = m_userPaths.resolve(prefix.string()); }
        catch (const BarnyardLinux::AssetMissing&) {
            current /= *it;
            if (next != parts.end() || directory) {
                fs::create_directory(current);
                m_userPaths.clear();
                current = m_userPaths.resolve(prefix.string());
            }
        }
    }
    m_userPaths.clear();
    return current;
}
TFile* TNativeFileSystem::CreateFile(const TString8& name, TFILEMODE mode) {
    auto* file = new TNativeFile(this);
    if (!file->Open(name, mode)) { delete file; return nullptr; }
    return file;
}
void TNativeFileSystem::DestroyFile(TFile* file) { delete file; }
TBOOL TNativeFileSystem::RemoveFile(const TString8& name) {
    try {
        // Do not resolve missing paths for deletion or create any directories.
        auto path = m_userPaths.resolve(logicalPath(name));
        if (!fs::is_regular_file(path)) return false;
        bool removed = fs::remove(path);
        m_userPaths.clear();
        return removed;
    } catch (const std::exception&) { return false; }
}
TString8 TNativeFileSystem::MakeInternalPath(const TString8& name) {
    try { return TString8(Resolve(name, false).string().c_str()); }
    catch (const std::exception&) { return TString8(""); }
}
TBOOL TNativeFileSystem::MakeDirectory(const TString8& name) {
    try { return fs::is_directory(Resolve(name, true, true)); }
    catch (const std::exception&) { return false; }
}
TBOOL TNativeFileSystem::GetFirstFile(const TString8& name, TString8& out, TFINDFILE flags) {
    m_entries.clear(); m_next = 0;
    try {
        std::map<std::string, Entry> merged;
        auto relative = logicalPath(name);
        for (auto* resolver : { &m_gamePaths, &m_userPaths }) {
            fs::path path;
            try { path = resolver->resolve(relative); }
            catch (const BarnyardLinux::AssetMissing&) { continue; }
            for (const auto& entry : fs::directory_iterator(path)) {
                auto filename = entry.path().filename().string();
                // Ensure enumeration also rejects symlinks escaping the root.
                resolver->resolve((fs::path(relative)/filename).string());
                merged[fold(filename)] = {filename, entry.is_directory()};
            }
        }
        for (const auto& [key, entry] : merged) m_entries.push_back(entry);
        return GetNextFile(out, flags);
    } catch (const std::exception&) { return false; }
}
TBOOL TNativeFileSystem::GetNextFile(TString8& out, TFINDFILE flags) {
    while (m_next < m_entries.size()) {
        const auto& entry = m_entries[m_next++];
        if (flags & (entry.directory ? TFINDFILE_DIRECTORY : TFINDFILE_FILE)) {
            out = entry.name.c_str(); return true;
        }
    }
    return false;
}

TNativeFile::TNativeFile(TNativeFileSystem* fs) : TFile(fs) {}
TNativeFile::~TNativeFile() { Close(); }
TBOOL TNativeFile::Open(const TString8& name, TFILEMODE mode) {
    Close();
    bool write = mode & (TFILEMODE_WRITE | TFILEMODE_READWRITE | TFILEMODE_CREATENEW);
    bool read = mode & (TFILEMODE_READ | TFILEMODE_READWRITE);
    if (!write && !read) return false;
    try {
        auto* native = static_cast<TNativeFileSystem*>(GetFileSystem());
        auto path = native->Resolve(name, write);
        int flags = (write ? (read ? O_RDWR : O_WRONLY) : O_RDONLY) | O_CLOEXEC;
        if (mode & TFILEMODE_CREATENEW) flags |= O_CREAT;
        if (write) flags |= O_NOFOLLOW;
        int fd = ::open(path.c_str(), flags, 0600);
        if (fd < 0) return false;
        // fdopen does not truncate an already open descriptor, unlike fopen.
        m_file = fdopen(fd, write ? (read ? "r+b" : "wb") : "rb");
        if (!m_file) { ::close(fd); return false; }
        struct stat info{};
        if (fstat(fileno(m_file), &info) || !S_ISREG(info.st_mode) || uint64_t(info.st_size) > UINT32_MAX) {
            Close(); return false;
        }
        if (write && (info.st_nlink != 1 || ((mode & TFILEMODE_CREATENEW) && ftruncate(fileno(m_file), 0)))) {
            Close(); return false;
        }
        if (mode & TFILEMODE_NOBUFFER) setvbuf(m_file, nullptr, _IONBF, 0);
        return true;
    } catch (const std::exception& e) {
        TDebug_FinalPrintf("Linux file open failed (%s): %s\n", name.GetString(), e.what());
        return false;
    }
}
void TNativeFile::Close() { if (m_file) fclose(m_file); m_file = nullptr; m_operation = 0; }
TSIZE TNativeFile::Read(void* out, TSIZE n) {
    if (!m_file) return 0;
    if (m_operation == 2 && fseeko(m_file, 0, SEEK_CUR)) return 0;
    m_operation = 1;
    return fread(out, 1, n, m_file);
}
TSIZE TNativeFile::Write(const void* in, TSIZE n) {
    if (!m_file) return 0;
    if (m_operation == 1 && fseeko(m_file, 0, SEEK_CUR)) return 0;
    m_operation = 2;
    return fwrite(in, 1, n, m_file);
}
TBOOL TNativeFile::Seek(TINT offset, TSEEK origin) {
    if (!m_file || origin < TSEEK_SET || origin > TSEEK_END) return false;
    const int native[] = { SEEK_SET, SEEK_CUR, SEEK_END };
    if (fseeko(m_file, offset, native[origin])) return false;
    m_operation = 0; return true;
}
TSIZE TNativeFile::Tell() {
    auto value = m_file ? ftello(m_file) : off_t(-1);
    return value >= 0 && uint64_t(value) <= UINT32_MAX ? TSIZE(value) : TSIZE(-1);
}
void TNativeFile::FlushBuffers() { if (m_file) fflush(m_file); }
TSIZE TNativeFile::GetSize() {
    if (!m_file) return TSIZE(-1);
    fflush(m_file);
    struct stat info{};
    return fstat(fileno(m_file), &info) == 0 && uint64_t(info.st_size) <= UINT32_MAX ? TSIZE(info.st_size) : TSIZE(-1);
}
TUINT64 TNativeFile::GetDate() {
    struct stat info{};
    if (!m_file || fstat(fileno(m_file), &info)) return 0;
    return (TUINT64(info.st_mtime) + 11644473600ull) * 10000000ull + TUINT64(info.st_mtim.tv_nsec)/100;
}
TCHAR TNativeFile::GetCChar() { TCHAR c; return Read(&c, 1) == 1 ? c : TCHAR(-1); }
TWCHAR TNativeFile::GetWChar() {
    unsigned char bytes[2];
    return Read(bytes, 2) == 2 ? TWCHAR(bytes[0] | (unsigned(bytes[1]) << 8)) : TWCHAR(0xFFFF);
}
TINT TNativeFile::PutCChar(TCHAR value) { return Write(&value, 1) == 1 ? (unsigned char)value : -1; }
TINT TNativeFile::PutWChar(TWCHAR value) {
    unsigned char bytes[2] = { (unsigned char)value, (unsigned char)(unsigned(value) >> 8) };
    return Write(bytes, 2) == 2 ? (unsigned(value) & 0xFFFF) : -1;
}
TINT TNativeFile::VCPrintf(const TCHAR* format, va_list args) {
    if (!m_file) return -1;
    if (m_operation == 1 && fseeko(m_file, 0, SEEK_CUR)) return -1;
    m_operation = 2;
    return vfprintf(m_file, format, args);
}
TINT TNativeFile::VWPrintf(const TWCHAR* format, va_list args) {
    TWCHAR buffer[4096];
    int result = TStringHelper_Linux::WideFormatN(buffer, 4096, format, args);
    if (result < 0 || result >= 4096) return -1;
    for (int i=0; i<result; ++i) if (PutWChar(buffer[i]) < 0) return -1;
    return result;
}
TOSHI_NAMESPACE_END

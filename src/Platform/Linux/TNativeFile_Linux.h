// Linux implementation of the OpenBarnyard TFile platform API.
// See third_party/OpenBarnyard-License.md for upstream API/decompilation terms.
#pragma once
#include <Toshi/Defines.h>
#include <File/TFile.h>
#include "AssetPaths.h"
#include <cstdio>
#include <filesystem>
#include <vector>

TOSHI_NAMESPACE_START
class TNativeFileSystem : public TFileSystem {
public:
    TNativeFileSystem(const TCHAR* name, const char* gameRoot, const char* userRoot);
    ~TNativeFileSystem() override;
    TFile* CreateFile(const TString8&, TFILEMODE) override;
    void DestroyFile(TFile*) override;
    TBOOL RemoveFile(const TString8&) override;
    TString8 MakeInternalPath(const TString8&) override;
    TBOOL GetFirstFile(const TString8&, TString8&, TFINDFILE) override;
    TBOOL GetNextFile(TString8&, TFINDFILE) override;
    TBOOL MakeDirectory(const TString8&);
    std::filesystem::path Resolve(const TString8&, bool writing, bool directory = false);
private:
    std::filesystem::path m_game, m_user;
    BarnyardLinux::AssetPaths m_gamePaths, m_userPaths;
    struct Entry { std::string name; bool directory; };
    std::vector<Entry> m_entries;
    size_t m_next = 0;
};

class TNativeFile : public TFile {
public:
    explicit TNativeFile(TNativeFileSystem*);
    ~TNativeFile() override;
    TSIZE Read(void*, TSIZE) override;
    TSIZE Write(const void*, TSIZE) override;
    TBOOL Seek(TINT, TSEEK = TSEEK_CUR) override;
    TSIZE Tell() override;
    void FlushBuffers() override;
    TSIZE GetSize() override;
    TUINT64 GetDate() override;
    TCHAR GetCChar() override;
    TWCHAR GetWChar() override;
    TINT PutCChar(TCHAR) override;
    TINT PutWChar(TWCHAR) override;
    TINT VCPrintf(const TCHAR*, va_list) override;
    TINT VWPrintf(const TWCHAR*, va_list) override;
    TBOOL Open(const TString8&, TFILEMODE);
    void Close();
private:
    FILE* m_file = nullptr;
    int m_operation = 0; // 0=positioned, 1=read, 2=write (stdio direction changes)
};
TOSHI_NAMESPACE_END

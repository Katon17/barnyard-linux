// Integration test: REAL upstream TMemory/TString/TFile/TTSF/TTRB, no mock engine.
// Fixture data is synthesized below and contains no Barnyard game assets.
#include <ToshiPCH.h>
#include <File/TTRB.h>
#include "../src/Platform/Linux/TNativeFile_Linux.h"
#include "../src/Platform/Linux/TSystemDLL_Linux.h"
#include <cassert>
#include <fstream>
#include <filesystem>
#include <cstring>
#include <vector>
#include <thread>
#include <unistd.h>

static_assert(sizeof(void*) == 4, "TRB relocation integration must use Linux x86");
static_assert(sizeof(wchar_t) == 2, "Retail wide strings are UTF-16");
// Upstream overrides global new. Initialize its allocator before C++ globals.
__attribute__((constructor(101))) static void memory_init() {
    if (!Toshi::TMemory::Initialise(64*1024*1024, 0, 0)) _exit(90);
}
namespace fs = std::filesystem;
using Bytes = std::vector<unsigned char>;
void u32(Bytes& out, uint32_t value) { for(int i=0; i<4; ++i) out.push_back((value>>(i*8))&255); }
void u16(Bytes& out, uint16_t value) { out.push_back(value&255); out.push_back(value>>8); }
void text(Bytes& out, const char* value) { out.insert(out.end(), value, value+strlen(value)); }
void hunk(Bytes& out, const char* name, const Bytes& payload) {
    text(out, name); u32(out, payload.size()); out.insert(out.end(), payload.begin(), payload.end());
    while(out.size()%4) out.push_back(0);
}
void fixture(const fs::path& path) {
    Bytes header; u32(header, 0x10000); u32(header, 1);
    u16(header, 0); u16(header, 16); u32(header, 16); u32(header, 0); u32(header, 0);
    Bytes data; u32(data, 8); u32(data, 0xdecafbad); text(data, "native"); data.push_back(0); data.push_back(0);
    Bytes relc; u32(relc, 1); u16(relc, 0); u16(relc, 0); u32(relc, 0);
    Bytes symbols; u32(symbols, 2);
    auto symbol = [&](const char* name, uint16_t nameOffset, uint32_t dataOffset) {
        u16(symbols, 0); u16(symbols, nameOffset); u16(symbols, 0);
        u16(symbols, uint16_t(Toshi::TTRB::HashString(name))); u32(symbols, dataOffset);
    };
    symbol("root", 0, 0); symbol("marker", 5, 4);
    text(symbols, "root"); symbols.push_back(0); text(symbols, "marker"); symbols.push_back(0);
    Bytes body; text(body, "TRBF"); hunk(body, "HDRX", header); hunk(body, "SECT", data);
    hunk(body, "RELC", relc); hunk(body, "SYMB", symbols);
    Bytes file; text(file, "TSFL"); u32(file, body.size()); file.insert(file.end(), body.begin(), body.end());
    std::ofstream output(path, std::ios::binary); output.write(reinterpret_cast<char*>(file.data()), file.size());
}
int main() {
    char pattern[] = "/tmp/barnyard-engine32-XXXXXX";
    auto* directory = mkdtemp(pattern); assert(directory);
    fs::path root(directory), game = root/"game", user = root/"user";
    fs::create_directories(game/"Data"); fixture(game/"Data/Test.trb");
    setenv("BARNYARD_GAME_DIR", game.c_str(), 1); setenv("BARNYARD_USER_DIR", user.c_str(), 1);
    // Bad root configurations must be rejected without creating directories in assets.
    setenv("BARNYARD_USER_DIR", (game/"forbidden-save").c_str(), 1);
    assert(!Toshi::TFileManager::Create()); assert(!fs::exists(game/"forbidden-save"));
    setenv("BARNYARD_USER_DIR", root.c_str(), 1);
    assert(!Toshi::TFileManager::Create());
    setenv("BARNYARD_USER_DIR", user.c_str(), 1);
    assert(Toshi::TFileManager::Create());
    {
        Toshi::TTRB trb;
        assert(trb.Load("DATA\\test.TRB") == Toshi::TTRB::ERROR_OK);
        assert(trb.GetNumSymbols() == 2);
        auto** pointer = static_cast<char**>(trb.GetSymbolAddress("root"));
        assert(pointer && !strcmp(*pointer, "native"));
        auto* marker = static_cast<uint32_t*>(trb.GetSymbolAddress("marker"));
        assert(marker && *marker == 0xdecafbad);
        assert(!trb.GetSymbolAddress("missing"));
        trb.Close();
        assert(trb.Load("data/test.trb") == Toshi::TTRB::ERROR_OK);
    }
    {
        auto* saved = Toshi::TFile::Create("Saves/slot01.sav", Toshi::TFILEMODE_CREATENEW | Toshi::TFILEMODE_READWRITE);
        assert(saved);
        assert(saved->CPrintf("slot=%d", 7) == 6);
        assert(saved->Seek(0, Toshi::TSEEK_SET));
        char buffer[16]{}; assert(saved->Read(buffer, 6) == 6); assert(!strcmp(buffer,"slot=7"));
        assert(saved->GetSize() == 6); saved->Destroy();
        assert(fs::exists(user/"Saves/slot01.sav")); assert(!fs::exists(game/"Saves"));
        saved = Toshi::TFile::Create("saves/SLOT01.SAV"); assert(saved);
        saved->Destroy();
        assert(!Toshi::TFile::Create("../outside", Toshi::TFILEMODE_CREATENEW));
        auto* fsys = Toshi::TFileManager::GetSingletonSafe()->FindFileSystem("local");
        assert(!fsys->RemoveFile("data/test.trb")); // Cannot delete original assets.
        assert(fsys->RemoveFile("saves/slot01.sav"));
        // A hardlinked user file must not permit truncating an original asset.
        fs::create_hard_link(game/"Data/Test.trb", user/"hardlink.trb");
        assert(!Toshi::TFile::Create("hardlink.trb", Toshi::TFILEMODE_CREATENEW));
        assert(fs::file_size(game/"Data/Test.trb") > 16);
    }
    {
        auto* wide = Toshi::TFile::Create("saves/wide.txt", Toshi::TFILEMODE_CREATENEW | Toshi::TFILEMODE_READWRITE);
        assert(wide && wide->WPrintf(L"%s %d", L"cow", 12) == 6);
        assert(wide->GetSize() == 12 && wide->Seek(0, Toshi::TSEEK_SET));
        for (auto* p = L"cow 12"; *p; ++p) assert(wide->GetWChar() == *p);
        assert(wide->GetWChar() == wchar_t(0xFFFF)); wide->Destroy();
        Toshi::TSystemDLLLinux module;
        assert(module.Load("libc.so.6"));
        auto length = reinterpret_cast<size_t(*)(const char*)>(module.GetAddress("strlen"));
        assert(length && length("native") == 6);
        module.Unload(); assert(!module.GetDLL());
    }
    {
        // Exercise the REAL recursive engine mutex and global allocator under
        // concurrent allocations, not only the standalone atomic primitive.
        Toshi::TMutex lock; lock.Create();
        int increments = 0;
        std::vector<std::thread> workers;
        for (int t=0; t<4; ++t) workers.emplace_back([&] {
            for (int i=0; i<3000; ++i) {
                lock.Lock(); lock.Lock();
                ++increments;
                lock.Unlock(); lock.Unlock();
                Toshi::TString8 value("thread allocation");
                assert(value.Length() == 17);
            }
        });
        for (auto& worker : workers) worker.join();
        assert(increments == 12000);
    }
    Toshi::TFileManager::Destroy();
    assert(Toshi::g_pMemory->TestMemIntegrity(Toshi::g_pMemory->GetGlobalBlock()));
    fs::remove_all(root);
    puts("PASS: real Toshi allocator + filesystem + TSF/TRB symbols/32-bit relocation + user saves");
    // Leave original allocator alive through C++ static destructors; OS reclaims
    // it at process exit. Full application shutdown is a separate engine task.
}

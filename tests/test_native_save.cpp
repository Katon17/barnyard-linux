#include "Platform/Linux/NativeSave.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <signal.h>
namespace fs=std::filesystem;
int main(){
    char pattern[]="/tmp/barnyard-save-XXXXXX";auto* temp=mkdtemp(pattern);assert(temp);
    fs::path root(temp),game=root/"game",user=root/"user";
    fs::create_directories(game);fs::create_directories(user);
    using BarnyardLinux::SaveStore;
    SaveStore store(user.c_str(),game.c_str());assert(store.valid());
    assert(store.status("Save/savedata.by")==SaveStore::Status::Missing);
    const char first[]="first save",next[]="replacement";
    assert(store.writeAtomic("Save/savedata.by",first,sizeof(first)));
    char actual[sizeof(first)]{};assert(store.readExact("Save/savedata.by",actual,sizeof(actual)) && !memcmp(actual,first,sizeof(first)));
    char untouched[3]={'a','b','c'};assert(!store.readExact("Save/savedata.by",untouched,3));assert(!memcmp(untouched,"abc",3));
    assert(store.writeAtomic((user/"Save/savedata.by").c_str(),next,sizeof(next)));
    char replacement[sizeof(next)]{};assert(store.readExact("Save/savedata.by",replacement,sizeof(replacement)) && !memcmp(replacement,next,sizeof(next)));
    assert(!store.writeAtomic("../game/escape",first,sizeof(first)));
    assert(!store.writeAtomic((game/"outside").c_str(),first,sizeof(first)));
    fs::create_directory_symlink(game,user/"linked");assert(!store.writeAtomic("linked/escape",first,sizeof(first)));
    fs::create_symlink(game/"target",user/"Save/symlink.by");assert(!store.writeAtomic("Save/symlink.by",first,sizeof(first)));
    fs::create_hard_link(user/"Save/savedata.by",user/"Save/hard.by");assert(!store.writeAtomic("Save/hard.by",first,sizeof(first)));assert(!store.readExact("Save/hard.by",replacement,sizeof(replacement)));
    fs::remove(user/"Save/hard.by");
    assert(mkfifo((user/"Save/fifo").c_str(),0600)==0);assert(!store.readExact("Save/fifo",actual,sizeof(actual)));
    // Deterministic ENOSPC-like short write: RLIMIT_FSIZE denies replacement,
    // previous committed file must survive and temporary must be removed.
    struct rlimit old{},small{};assert(!getrlimit(RLIMIT_FSIZE,&old));small=old;small.rlim_cur=4;
    auto oldSignal=signal(SIGXFSZ,SIG_IGN);assert(!setrlimit(RLIMIT_FSIZE,&small));
    assert(!store.writeAtomic("Save/savedata.by",first,sizeof(first)));
    assert(!setrlimit(RLIMIT_FSIZE,&old));signal(SIGXFSZ,oldSignal);
    assert(store.readExact("Save/savedata.by",replacement,sizeof(replacement)) && !memcmp(replacement,next,sizeof(next)));
    for(const auto& f:fs::directory_iterator(user/"Save")) assert(f.path().extension()!=".tmp");
    SaveStore overlap(game.c_str(),game.c_str());assert(!overlap.valid());
    SaveStore missing(nullptr,game.c_str());assert(!missing.valid());
    assert(fs::is_empty(game));fs::remove_all(root);
    puts("PASS: native user-only atomic save/read/replace, exact-size transaction, traversal/symlink/hardlink/FIFO denial, failed write preserves previous commit");
}

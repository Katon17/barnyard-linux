#include "NativeSave.h"
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <sys/random.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
#include <cstdio>
#include <cstdint>

namespace fs = std::filesystem;
namespace BarnyardLinux {
namespace {
struct Fd { int fd; ~Fd(){if(fd>=0) close(fd);} };
bool beneath(const fs::path& child, const fs::path& parent) {
    auto rel=child.lexically_relative(parent);
    return !rel.empty() && !rel.is_absolute() && *rel.begin()!="..";
}
bool regular(int dir, const char* name, bool missing) {
    struct stat st{};
    if(fstatat(dir,name,&st,AT_SYMLINK_NOFOLLOW)) return missing && errno==ENOENT;
    if(!S_ISREG(st.st_mode) || st.st_nlink!=1 || st.st_uid!=geteuid()) { errno=EPERM; return false; }
    return true;
}
}
bool SaveStore::fail(const char* op) { const int code=errno;error_=std::string(op)+": "+strerror(code);errno=code;return false; }
SaveStore::SaveStore(const char* userRoot,const char* gameRoot) {
    try {
        if(!userRoot || !*userRoot || !gameRoot || !*gameRoot) {error_="Missing user/game root"; return;}
        root_=fs::canonical(userRoot);
        auto game=fs::canonical(gameRoot);
        if(!fs::is_directory(root_) || !fs::is_directory(game) || beneath(root_,game) || beneath(game,root_)) {
            error_="Save root and game root must be disjoint directories"; return;
        }
        rootFd_=open(root_.c_str(),O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW);
        if(rootFd_<0) fail("open save root");
    } catch(const std::exception& e) {error_=e.what();}
}
SaveStore::~SaveStore(){if(rootFd_>=0) close(rootFd_);}
std::string SaveStore::directory() const {return valid() ? (root_/"Save").string() : std::string();}
int SaveStore::parent(const char* path,bool create,std::string& leaf) {
    if(!valid() || !path || !*path) {errno=EINVAL;fail("save path");return -1;}
    fs::path relative(path);
    // Reject traversal before any lexical normalization.
    for(const auto& p:relative) if(p=="..") {errno=EPERM;fail("save traversal");return -1;}
    if(relative.is_absolute()) {
        if(!beneath(relative,root_)) {errno=EPERM;fail("save outside user root");return -1;}
        relative=relative.lexically_relative(root_);
    }
    if(relative.empty() || relative.filename().empty() || relative.filename()==".") {errno=EINVAL;fail("save leaf");return -1;}
    leaf=relative.filename().string();
    Fd current{fcntl(rootFd_,F_DUPFD_CLOEXEC,0)};
    if(current.fd<0) {fail("dup save root");return -1;}
    for(const auto& part:relative.parent_path()) {
        if(part=="." || part.empty()) continue;
        const auto name=part.string();
        if(create) {
            const int result=mkdirat(current.fd,name.c_str(),0700);
            if(result && errno!=EEXIST) {fail("mkdir save");return -1;}
            if(!result && fsync(current.fd)) {fail("sync created save directory entry");return -1;}
        }
        int next=openat(current.fd,name.c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW|O_CLOEXEC);
        if(next<0) {fail("open save directory");return -1;}
        close(current.fd);current.fd=next;
    }
    int result=current.fd;current.fd=-1;return result;
}
SaveStore::Status SaveStore::status(const char* path) {
    std::string leaf; Fd dir{parent(path,false,leaf)};
    if(dir.fd<0) return errno==ENOENT ? Status::Missing : Status::Error;
    struct stat st{};
    if(fstatat(dir.fd,leaf.c_str(),&st,AT_SYMLINK_NOFOLLOW)) {
        if(errno==ENOENT) return Status::Missing;
        fail("stat save");return Status::Error;
    }
    if(!regular(dir.fd,leaf.c_str(),false)) {fail("unsafe save");return Status::Error;}
    return Status::Regular;
}
bool SaveStore::readExact(const char* path,void* data,size_t size) {
    if((!data && size) || size>64*1024*1024) {errno=EINVAL;return fail("save read size");}
    std::string leaf;Fd dir{parent(path,false,leaf)};if(dir.fd<0) return false;
    Fd file{openat(dir.fd,leaf.c_str(),O_RDONLY|O_NOFOLLOW|O_CLOEXEC|O_NONBLOCK)};
    if(file.fd<0) return fail("open save read");
    struct stat st{};
    if(fstat(file.fd,&st)) return fail("stat open save");
    if(!S_ISREG(st.st_mode) || st.st_nlink!=1 || st.st_uid!=geteuid() || st.st_size!=off_t(size)) {errno=EINVAL;return fail("save format/size");}
    std::vector<unsigned char> bytes(size);
    size_t done=0;
    while(done<size) {
        ssize_t n=read(file.fd,bytes.data()+done,size-done);
        if(n<0 && errno==EINTR) continue;
        if(n<=0) {if(!n) errno=EIO;return fail("read save");}
        done+=size_t(n);
    }
    unsigned char extra;ssize_t n;
    do { n=read(file.fd,&extra,1); } while(n<0 && errno==EINTR);
    if(n!=0) {errno=EIO;return fail("save changed size");}
    if(size) memcpy(data,bytes.data(),size);
    return true;
}
bool SaveStore::writeAtomic(const char* path,const void* data,size_t size) {
    if((!data && size) || size>64*1024*1024) {errno=EINVAL;return fail("save write size");}
    std::string leaf;Fd dir{parent(path,true,leaf)};if(dir.fd<0) return false;
    if(!regular(dir.fd,leaf.c_str(),true)) return fail("unsafe save target");
    uint64_t random;
    ssize_t got;
    do {got=getrandom(&random,sizeof(random),0);} while(got<0 && errno==EINTR);
    if(got!=sizeof(random)) {errno=EIO;return fail("save random name");}
    char temp[64];snprintf(temp,sizeof(temp),".save-%ld-%016llx.tmp",long(getpid()),(unsigned long long)random);
    Fd file{openat(dir.fd,temp,O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC|O_NOFOLLOW,0600)};
    if(file.fd<0) return fail("create save temporary");
    auto cleanup=[&](const char* op){int saved=errno;unlinkat(dir.fd,temp,0);errno=saved;return fail(op);};
    const auto* bytes=static_cast<const unsigned char*>(data);size_t done=0;
    while(done<size) {
        ssize_t n=write(file.fd,bytes+done,size-done);
        if(n<0 && errno==EINTR) continue;
        if(n<=0) {if(!n) errno=EIO;return cleanup("write save temporary");}
        done+=size_t(n);
    }
    if(fsync(file.fd)) return cleanup("sync save temporary");
    int fd=file.fd;file.fd=-1;
    if(close(fd)) return cleanup("close save temporary");
    if(!regular(dir.fd,leaf.c_str(),true)) return cleanup("save target changed");
    if(renameat(dir.fd,temp,dir.fd,leaf.c_str())) return cleanup("commit save");
    if(fsync(dir.fd)) return fail("sync save directory after commit");
    return true;
}
}

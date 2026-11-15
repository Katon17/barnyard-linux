// Real Toshi application/scheduler/log/filesystem lifecycle; NOT Barnyard gameplay.
#include <ToshiPCH.h>
#include <Toshi/TApplication.h>
#include <Toshi/TSystem.h>
#include <Toshi/TUtil.h>
#include <Toshi/TDateTime.h>
#include <Util/PString.h>
#include <ctime>
#include "Platform/Linux/NativeStartup.h"
#include "Platform/Linux/NativeOptions.h"
#include <File/TFile.h>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <cmath>
#include <limits>
#include <sys/wait.h>
#include <unistd.h>

__attribute__((constructor(101))) static void allocator_init() {
    if (!Toshi::TMemory::Initialise(64*1024*1024, 0, 0)) _exit(90);
}
namespace fs = std::filesystem;
static void platform_test(const fs::path& user) {
    using namespace BarnyardLinux;
    char one[]="barnyard", two[]="folder with spaces", three[]="", four[]="a\"b\\c", flag[]="-multiinstance";
    char* argv[]{one,two,three,four,flag};
    assert(commandLine(5,argv)=="\"barnyard\" \"folder with spaces\" \"\" \"a\\\"b\\\\c\" \"-multiinstance\"");
    assert(hasArgument(5,argv,"-multiinstance")); assert(!hasArgument(4,argv,"-multiinstance"));
    assert(!hasArgument(5,argv,"-multi"));
    {
        InstanceLock first;
        assert(first.acquire(user.c_str())==InstanceLock::Result::Acquired);
        InstanceLock second;
        assert(second.acquire(user.c_str())==InstanceLock::Result::AlreadyRunning);
        pid_t child=fork(); assert(child>=0);
        if (!child) {
            InstanceLock other;
            _exit(other.acquire(user.c_str())==InstanceLock::Result::AlreadyRunning ? 0 : 1);
        }
        int status; assert(waitpid(child,&status,0)==child); assert(WIFEXITED(status)&&WEXITSTATUS(status)==0);
    }
    {
        InstanceLock again; assert(again.acquire(user.c_str())==InstanceLock::Result::Acquired);
    }
    fs::remove(user/".barnyard.instance.lock");
    fs::create_symlink(user/"target",user/".barnyard.instance.lock");
    { InstanceLock unsafe; assert(unsafe.acquire(user.c_str())==InstanceLock::Result::Error); }
    assert(!fs::exists(user/"target")); fs::remove(user/".barnyard.instance.lock");
    FramePacer pacer;
    auto begin=FramePacer::Clock::now();
    for(int i=0;i<4;++i) pacer.wait(50);
    auto elapsed=FramePacer::Clock::now()-begin;
    assert(elapsed>=std::chrono::milliseconds(50));
    pacer.wait(0); pacer.wait(-1); pacer.wait(std::numeric_limits<float>::quiet_NaN());
    puts("PASS: Linux argv preservation, process instance lock/release/symlink denial, steady-clock frame pacing");
}
static void portability_test() {
    char buffer[5] = {};
    PString::Format(buffer, sizeof(buffer), "%s", "abcdefgh");
    assert(strcmp(buffer, "abcd") == 0);
    char sentinel='x';
    PString::Format(&sentinel, 0, "%s", "do not write");
    assert(sentinel=='x');
    PString::Format(&sentinel, 1, "%s", "empty");
    assert(sentinel=='\0');
    char tokens[]=" , alpha,beta";
    assert(strcmp(PString::Tokenize(tokens," ,"),"alpha")==0);
    assert(PString::Tokenize(nullptr," ,")==nullptr);
    using Toshi::TDateTime;
    const time_t before=time(nullptr);
    TDateTime date{};
    assert(TDateTime::GetLocalTime(date));
    const time_t after=time(nullptr);
    tm local{};
    local.tm_year=date.uiYear-1900; local.tm_mon=date.uiMonth;
    local.tm_mday=date.uiDay; local.tm_hour=date.uiHour;
    local.tm_min=date.uiMinute; local.tm_sec=date.uiSecond;
    local.tm_isdst=-1;
    const time_t converted=mktime(&local);
    assert(converted>=before && converted<=after);
    TUINT32 packedDate=0, packedTime=0;
    date.Pack(packedDate,packedTime);
    TDateTime decoded{}; decoded.UnPack(packedDate,packedTime);
    assert(decoded.uiYear==date.uiYear && decoded.uiMonth==date.uiMonth && decoded.uiDay==date.uiDay);
    assert(decoded.uiHour==date.uiHour && decoded.uiMinute==date.uiMinute && decoded.uiSecond==date.uiSecond);
    puts("PASS: native debug string truncation/zero capacity/tokenization + localtime_r date/pack roundtrip");
}
int main() {
    using namespace Toshi;
    static_assert(sizeof(void*)==4 && sizeof(TWCHAR)==2);
    char pattern[]="/tmp/barnyard-startup32-XXXXXX";
    char* temp=mkdtemp(pattern); assert(temp);
    fs::path root(temp), game=root/"game", user=root/"user";
    fs::create_directories(game);
    fs::create_directories(user/"Logs");
    setenv("BARNYARD_GAME_DIR",game.c_str(),1);
    setenv("BARNYARD_USER_DIR",user.c_str(),1);
    platform_test(user);
    portability_test();
    BarnyardLinux::NativeOptions options;
    std::string optionError;
    assert(options.parse("width=1280\nheight=720\ngamma=0.6\nsensitivity=0.7\nwindowed=0\nborderless=1\nhigh_detail_grass=1\ncontroller=0\n",optionError));
    assert(options.width==1280 && options.height==720 && !options.windowed && options.borderless);
    assert(!options.parse("width=800\ngamma=nan",optionError) && options.width==1280);
    assert(!options.parse("width=12",optionError));
    assert(!options.parse("windowed=2",optionError));
    assert(!options.parse("typo=1",optionError));
    assert(!options.parse(std::string(9000,'a'),optionError));
    TUtil::TOSHIParams params;
    params.szLogFileName="native-test";
    // A mount failure must not report a successful core startup.
    setenv("BARNYARD_USER_DIR",game.c_str(),1);
    assert(!TUtil::ToshiCreate(params));
    assert(!TUtil::IsSingletonCreated());
    setenv("BARNYARD_USER_DIR",user.c_str(),1);
    assert(TFileManager::Create()); // Linux main validates/mounts before instance lock and log startup
    assert(TUtil::ToshiCreate(params));
    assert(options.load(optionError)==BarnyardLinux::NativeOptions::LoadResult::Missing);
    auto* config=TFile::Create("Settings/Linux.cfg", TFILEMODE_CREATENEW | TFILEMODE_WRITE);
    assert(config);
    const char* settings="width=1024\nheight=768\nwindowed=1\nhigh_detail_grass=0\n";
    assert(config->Write(settings,strlen(settings))==strlen(settings)); config->Destroy();
    assert(options.load(optionError)==BarnyardLinux::NativeOptions::LoadResult::Loaded);
    assert(options.width==1024 && options.height==768 && options.windowed && !options.highDetailGrass);
    puts("PASS: validated transactional native options + real Toshi user-overlay config IO");
    {
        TDebugConsole console;
        assert(console.IsCreated()); assert(!console.IsVisible());
        assert(!console.Show(TTRUE)); assert(console.Toggle()==TFALSE);
    }
    struct App : TApplication {
        int updates=0, destroys=0;
        TBOOL OnCreate(int argc, char** argv) override {
            assert(argc==2 && strcmp(argv[1],"argument with spaces")==0);
            return TApplication::OnCreate(argc,argv);
        }
        TBOOL OnUpdate(float dt) override {
            assert(std::isfinite(dt)&&dt>=0);
            if (++updates==3) Destroy();
            return TApplication::OnUpdate(dt);
        }
        TBOOL OnDestroy() override { ++destroys; return TApplication::OnDestroy(); }
    };
    {
        App app;
        char a[]="barnyard", b[]="argument with spaces"; char* argv[]{a,b};
        assert(app.Create("Native lifecycle integration",2,argv));
        TUtil::Log("literal payload: %s\n", "100% %n %s");
        assert(app.Execute());
        assert(app.updates==3 && app.destroys==1 && app.IsDestroyed());
        assert(g_oSystemManager.GetFrameCount()==3);
    }
    TUtil::ToshiDestroy();
    assert(!TUtil::IsSingletonCreated());
    bool found=false;
    for (const auto& entry:fs::directory_iterator(user/"Logs")) {
        std::ifstream in(entry.path());
        std::string text((std::istreambuf_iterator<char>(in)),{});
        if (text.find("100% %n %s")!=std::string::npos) found=true;
    }
    assert(found); assert(fs::is_empty(game));
    // Retention preserves game data, non-logs, symlinks and hardlinks.
    std::ofstream(game/"protected.log")<<"asset";
    fs::create_symlink(game/"protected.log",user/"Logs/link.log");
    fs::create_hard_link(game/"protected.log",user/"Logs/hard.log");
    std::ofstream(user/"Logs/keep.txt")<<"keep";
    for(int i=0;i<12;++i) std::ofstream(user/"Logs"/("old"+std::to_string(i)+".log"))<<"old";
    TUtil::TrimLog("*.log",2);
    int retained=0;
    for(const auto& entry:fs::directory_iterator(user/"Logs"))
        if(entry.path().extension()==".log" && !entry.is_symlink() && fs::hard_link_count(entry.path())==1) ++retained;
    assert(retained==2);
    assert(fs::is_symlink(user/"Logs/link.log") && fs::exists(user/"Logs/hard.log") && fs::exists(user/"Logs/keep.txt"));
    std::ifstream protectedFile(game/"protected.log"); std::string protectedText; protectedFile>>protectedText; assert(protectedText=="asset");
    fs::remove_all(root);
    puts("PASS: actual ToshiCreate + TApplication Execute/scheduler + log literal percent + console cleanup + ToshiDestroy + user-only log retention; NOT game startup");
}

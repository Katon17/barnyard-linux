// Actual upstream TRenderGLInterface integration. Synthetic pixels, NOT gameplay.
#include <ToshiPCH.h>
#include <Platform/GL/TRenderInterface_GL.h>
#include <Platform/GL/TTextureFactoryHAL_GL.h>
#include <Platform/GL/TTextureResourceHAL_GL.h>
#include <Toshi/TSystem.h>
#include "Platform/Linux/Input/TInputInterface_Linux.h"
#include "Platform/Linux/Input/TInputDeviceKeyboard_Linux.h"
#include "Platform/Linux/Input/TInputDeviceMouse_Linux.h"
#include <File/TFileStream.h>
#include <atomic>
#include <vector>
#include <tuple>
struct InputCapture {
    std::vector<std::tuple<int,int,int,int>> events;
    static TBOOL __stdcall receive(InputCapture* self, Toshi::TInputInterface*, Toshi::TInputInterface::InputEvent* event) {
        self->events.emplace_back(event->GetDoodad(), event->GetEventType(),
            event->GetAxisCount()>0 ? event->GetMagnitudeInt(0) : 0, event->GetAxisCount()>1 ? event->GetMagnitudeInt(1) : 0);
        return TTRUE;
    }
    int count(int key, int type) const {
        int result=0;
        for (const auto& e : events) if (std::get<0>(e)==key && std::get<1>(e)==type) ++result;
        return result;
    }
};
#include <cassert>
#include <cstdio>
#include <unistd.h>

__attribute__((constructor(101))) static void memory_init() {
    if (!Toshi::TMemory::Initialise(128*1024*1024, 0, 0)) _exit(90);
}
static void streaming_test() {
    using namespace Toshi;
    struct Worker : TThread {
        std::atomic<int> runs{0};
        void Main() override { assert(!Destroy()); ++runs; }
        ~Worker() override { Destroy(); }
    } worker;
    assert(!worker.Create(0, TThread::THREAD_PRIORITY_NORMAL, TThread::FLAGS_SUSPENDED));
    assert(!worker.Create(0, TThread::THREAD_PRIORITY_HIGHEST, TThread::FLAGS_NONE));
    assert(!worker.Create(1, TThread::THREAD_PRIORITY_NORMAL, TThread::FLAGS_NONE));
    for (int i=0; i<50; ++i) {
        assert(worker.Create(0, TThread::THREAD_PRIORITY_NORMAL, TThread::FLAGS_NONE));
        assert(worker.Destroy()); assert(!worker.Destroy());
    }
    assert(worker.runs == 50);
    struct Job : TFileStreamJob {
        explicit Job() : TFileStreamJob(nullptr) {}
        int result = 0;
        void Process() override { result = 12345; }
    };
    // Queued work exceeds FIFO capacity. Stop must drain and join before job destruction.
    for (int cycle=0; cycle<10; ++cycle) {
        Job jobs[100];
        TFileStream stream;
        assert(stream.Create(0, TThread::THREAD_PRIORITY_NORMAL, TThread::FLAGS_NONE));
        for (auto& job : jobs) stream.AddStream(&job);
        stream.Stop(); stream.Stop();
        assert(stream.m_hThreadHnd == nullptr);
        for (const auto& job : jobs) { assert(job.IsProcessed()); assert(job.result == 12345); }
    }
    puts("PASS: joinable native thread lifecycle + real Toshi FIFO/streamer drains 1000 jobs before shutdown");
}
int main() {
    using namespace Toshi;
    static_assert(sizeof(void*) == 4);
    streaming_test();
    assert(TSystemManager::Create());
    {
        TRenderGLInterface renderer;
        assert(renderer.Create("Real Toshi renderer integration - not Barnyard gameplay"));
        printf("GL_VERSION=%s\nGL_RENDERER=%s\n", glGetString(GL_VERSION), glGetString(GL_RENDERER));
        assert(renderer.GetMemBlock());
        auto* factory = renderer.CreateResource(&TGetClass(TTextureFactoryHAL), "textures", nullptr);
        assert(factory);
        renderer.SetResourceExplicit(factory, SYSRESOURCE_TEXTUREFACTORY);
        TRenderInterface::DISPLAYPARAMS params{};
        params.uiWidth = 640; params.uiHeight = 480; params.uiColourDepth = 32;
        params.eDepthStencilFormat = TRenderInterface::DEPTHSTENCILFORMAT_D24_S8;
        params.bWindowed = TTRUE;
        assert(renderer.CreateDisplay(params));
        TInputLinuxInterface input;
        assert(input.Initialise());
        assert(!input.Initialise());
        InputCapture captured;
        TListener<TInputInterface, TInputInterface::InputEvent, InputCapture> listener;
        listener.Connect(input.GetInputEventEmitter(), &captured, InputCapture::receive, 0);
        auto* keyboard = input.GetDeviceByIndex<TInputDeviceKeyboard>();
        auto* mouse = input.GetDeviceByIndex<TInputDeviceMouse>();
        assert(keyboard && mouse);
        Uint32 windowID = SDL_GetWindowID(renderer.GetWindow()->GetSDLWindow());
        auto pumpInput = [&] { assert(renderer.Update(1.0f/60)); input.ProcessEvents(1.0f/60); };
        auto focus = [&](Uint8 type) {
            SDL_Event e{}; e.type=SDL_WINDOWEVENT; e.window.windowID=windowID; e.window.event=type;
            assert(SDL_PushEvent(&e)==1);
        };
        auto key = [&](Uint32 type, SDL_Scancode code, bool repeat=false) {
            SDL_Event e{}; e.type=type; e.key.windowID=windowID; e.key.keysym.scancode=code; e.key.repeat=repeat;
            assert(SDL_PushEvent(&e)==1);
        };
        auto button = [&](Uint32 type) {
            SDL_Event e{}; e.type=type; e.button.windowID=windowID; e.button.button=SDL_BUTTON_LEFT;
            assert(SDL_PushEvent(&e)==1);
        };
        pumpInput(); focus(SDL_WINDOWEVENT_FOCUS_GAINED); pumpInput(); captured.events.clear();
        key(SDL_KEYDOWN, SDL_SCANCODE_W); button(SDL_MOUSEBUTTONDOWN); pumpInput();
        assert(keyboard->IsDown(TInputDeviceKeyboard::KEY_W));
        assert(mouse->IsDown(TInputDeviceMouse::BUTTON_1));
        assert(captured.count(TInputDeviceKeyboard::KEY_W, TInputInterface::EVENT_TYPE_GONE_DOWN)==1);
        key(SDL_KEYDOWN, SDL_SCANCODE_W, true); pumpInput();
        assert(captured.count(TInputDeviceKeyboard::KEY_W, TInputInterface::EVENT_TYPE_GONE_DOWN)==1);
        focus(SDL_WINDOWEVENT_FOCUS_LOST); pumpInput();
        assert(!keyboard->IsDown(TInputDeviceKeyboard::KEY_W));
        assert(!mouse->IsDown(TInputDeviceMouse::BUTTON_1));
        assert(captured.count(TInputDeviceKeyboard::KEY_W, TInputInterface::EVENT_TYPE_GONE_UP)==1);
        key(SDL_KEYDOWN, SDL_SCANCODE_W); pumpInput(); assert(!keyboard->IsDown(TInputDeviceKeyboard::KEY_W));
        focus(SDL_WINDOWEVENT_FOCUS_GAINED); pumpInput(); captured.events.clear();
        // Preserve fast taps and >64 edges in the same frame, without stuck keys.
        for (int i=0; i<100; ++i) { key(SDL_KEYDOWN, SDL_SCANCODE_A); key(SDL_KEYUP, SDL_SCANCODE_A); }
        button(SDL_MOUSEBUTTONDOWN); button(SDL_MOUSEBUTTONUP); pumpInput();
        assert(captured.count(TInputDeviceKeyboard::KEY_A, TInputInterface::EVENT_TYPE_GONE_DOWN)==100);
        assert(captured.count(TInputDeviceKeyboard::KEY_A, TInputInterface::EVENT_TYPE_GONE_UP)==100);
        assert(captured.count(TInputDeviceMouse::BUTTON_1, TInputInterface::EVENT_TYPE_GONE_DOWN)==1);
        assert(captured.count(TInputDeviceMouse::BUTTON_1, TInputInterface::EVENT_TYPE_GONE_UP)==1);
        assert(!keyboard->IsDown(TInputDeviceKeyboard::KEY_A));
        SDL_Event motion{}; motion.type=SDL_MOUSEMOTION; motion.motion.windowID=windowID;
        motion.motion.xrel=7; motion.motion.yrel=-3; assert(SDL_PushEvent(&motion)==1);
        SDL_Event wheel{}; wheel.type=SDL_MOUSEWHEEL; wheel.wheel.windowID=windowID;
        wheel.wheel.y=1; assert(SDL_PushEvent(&wheel)==1); pumpInput();
        assert(mouse->GetAxisInt(TInputDeviceMouse::AXIS_CURSOR,0)==7);
        assert(mouse->GetAxisInt(TInputDeviceMouse::AXIS_CURSOR,1)==-3);
        assert(mouse->GetAxisFloat(TInputDeviceMouse::AXIS_WHEEL,0)==120);
        pumpInput(); assert(mouse->GetAxisInt(TInputDeviceMouse::AXIS_CURSOR,0)==0);
        key(SDL_KEYDOWN, SDL_SCANCODE_LSHIFT); pumpInput();
        assert(keyboard->IsShiftDown());
        key(SDL_KEYUP, SDL_SCANCODE_LSHIFT); pumpInput(); assert(!keyboard->IsShiftDown());
        key(SDL_KEYDOWN, SDL_SCANCODE_D); pumpInput();
        assert(input.UnacquireAll()); assert(!keyboard->IsDown(TInputDeviceKeyboard::KEY_D));
        key(SDL_KEYDOWN, SDL_SCANCODE_D); pumpInput(); assert(!keyboard->IsDown(TInputDeviceKeyboard::KEY_D));
        assert(input.AcquireAll());
        // Actual SDL capture, hide/show, focus release, manual escape and click
        // recapture. The keyboard is never grabbed, preserving desktop Alt+Tab.
        auto* native=renderer.GetWindow()->GetSDLWindow();
        assert(SDL_SetWindowInputFocus(native)==0);SDL_PumpEvents();
        focus(SDL_WINDOWEVENT_FOCUS_GAINED);pumpInput();
        assert(SDL_GetRelativeMouseMode()==SDL_TRUE);
        assert(SDL_GetWindowGrab(native)==SDL_TRUE);
        assert(SDL_GetWindowKeyboardGrab(native)==SDL_FALSE);
        assert(SDL_ShowCursor(SDL_QUERY)==SDL_DISABLE);
        auto captureShortcut=[&] {
            SDL_Event e{};e.type=SDL_KEYDOWN;e.key.windowID=windowID;
            e.key.keysym.scancode=SDL_SCANCODE_F1;e.key.keysym.mod=KMOD_SHIFT;
            assert(SDL_PushEvent(&e)==1);
            e.type=SDL_KEYUP;assert(SDL_PushEvent(&e)==1);pumpInput();
        };
        captureShortcut();assert(SDL_GetRelativeMouseMode()==SDL_FALSE);
        assert(SDL_ShowCursor(SDL_QUERY)==SDL_ENABLE);assert(SDL_GetWindowGrab(native)==SDL_FALSE);
        button(SDL_MOUSEBUTTONDOWN);pumpInput();
        assert(SDL_GetRelativeMouseMode()==SDL_TRUE);
        assert(!mouse->IsDown(TInputDeviceMouse::BUTTON_1));
        focus(SDL_WINDOWEVENT_FOCUS_LOST);pumpInput();
        assert(SDL_GetRelativeMouseMode()==SDL_FALSE);assert(SDL_ShowCursor(SDL_QUERY)==SDL_ENABLE);
        focus(SDL_WINDOWEVENT_FOCUS_GAINED);pumpInput();assert(SDL_GetRelativeMouseMode()==SDL_TRUE);
        captureShortcut();captureShortcut();assert(SDL_GetRelativeMouseMode()==SDL_TRUE);
        SDL_DisplayMode desktop{};assert(SDL_GetDesktopDisplayMode(0,&desktop)==0);
        assert(renderer.ChangeDisplay(desktop.w,desktop.h,TFALSE,TTRUE));
        assert(renderer.GetBackBufferWidth()==unsigned(desktop.w));
        assert(renderer.GetBackBufferHeight()==unsigned(desktop.h));
        assert(!renderer.GetCurrentDisplayParams()->bWindowed);
        assert((SDL_GetWindowFlags(native)&SDL_WINDOW_FULLSCREEN_DESKTOP)==SDL_WINDOW_FULLSCREEN_DESKTOP);
        assert(renderer.ChangeDisplay(640,480,TTRUE,TFALSE));
        assert(renderer.GetCurrentDisplayParams()->bWindowed);
        assert(renderer.GetBackBufferWidth()==640 && renderer.GetBackBufferHeight()==480);
        puts("PASS: actual relative mouse/grab/cursor, no keyboard grab, Shift+F1 release/toggle, click recapture, focus release/restore; engine fullscreen/backbuffer roundtrip");
        puts("PASS: SDL -> real Toshi input emitter, key/mouse edges, 200-edge burst, mouse delta/wheel, modifier, focus release, unacquire");
        for (int frame = 0; frame < 3; ++frame) {
            assert(renderer.Update(1.0f / 60));
            assert(renderer.BeginScene());
            renderer.ClearRegion(0, 0, 640, 480, TRenderGLInterface::CLEARFLAGS_TARGET, 32, 96, 160, 1, 0);
            unsigned char pixel[4]{};
            glReadPixels(100, 100, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
            assert(pixel[0] == 32 && pixel[1] == 96 && pixel[2] == 160);
            assert(glGetError() == GL_NO_ERROR);
            assert(renderer.EndScene());
        }
        SDL_Event close{}; close.type = SDL_QUIT;
        assert(SDL_PushEvent(&close) == 1);
        assert(!renderer.Update(1.0f / 60));
        assert(renderer.GetWindow()->IsDestroyed());
        listener.Disconnect();
        assert(input.Deinitialise());
        assert(input.Deinitialise());
        assert(renderer.Destroy());
    }
    TSystemManager::DestroySystemTimer();
    puts("PASS: real Toshi i386 GL renderer create/display/clear/readback/present/quit/destroy; NOT gameplay");
}

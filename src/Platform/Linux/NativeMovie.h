#pragma once
#include <cstdint>
// Source-level adapter used only by the Linux port of ABINKMoviePlayer.
// NOT the proprietary Bink SDK/ABI: decoding is FFmpeg, audio is SDL.
struct NativeMovieState;
struct BINK {
    uint32_t Width=0, Height=0, Frames=0, LastFrameNum=0;
    NativeMovieState* native=nullptr;
};
using HBINK=BINK*;
constexpr uint32_t BINKCOPYALL=0x80000000u;
constexpr uint32_t BINKSURFACE32=0, BINKSURFACE32A=1;
HBINK BinkOpen(const char* path, uint32_t flags);
void BinkClose(HBINK);
int BinkWait(HBINK);
int BinkDoFrame(HBINK); // 0=new frame, 1=EOF, -1=decode failure
void BinkNextFrame(HBINK);
void BinkPause(HBINK,int paused);
void NativeMovieSystemPause(HBINK,bool);
bool NativeMoviePaused(HBINK);
void BinkSetSoundOnOff(HBINK,int enabled);
void BinkCopyToBuffer(HBINK,void* destination,int pitch,uint32_t height,uint32_t x,uint32_t y,uint32_t format);
bool NativeMovieFinished(HBINK);
void NativeMovieMute(bool);
const char* NativeMovieError();

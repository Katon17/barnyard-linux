// Real FSB3 sample bank supplied privately by the user. No PCM export.
#include <pch.h>
#include <fmod/fmod.h>
#include <File/TFile.h>
#include "Platform/Linux/NativeAudio.h"
#include <SDL2/SDL.h>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <unistd.h>
__attribute__((constructor(101))) static void init(){if(!Toshi::TMemory::Initialise(64*1024*1024,0))_exit(90);}
static float observe(int channel){
    float peak=0;const auto before=BarnyardLinux::audioSnapshot().callbacks;
    for(int i=0;i<200;++i){auto s=BarnyardLinux::audioSnapshot();peak=std::max(peak,std::max(fabsf(s.left),fabsf(s.right)));SDL_Delay(1);}
    assert(BarnyardLinux::audioSnapshot().callbacks>before+2);
    assert(FSOUND_IsPlaying(channel));return peak;
}
int main(int argc,char** argv){
    assert(argc==2);static_assert(sizeof(void*)==4);
    namespace fs=std::filesystem;auto game=fs::canonical(argv[1]);char user[]="/tmp/barnyard-retail-audio-XXXXXX";assert(mkdtemp(user));
    setenv("BARNYARD_GAME_DIR",game.c_str(),1);setenv("BARNYARD_USER_DIR",user,1);assert(Toshi::TFileManager::Create());
    assert(FSOUND_Init(44100,32,0));const auto file=game/"Data/Sound/FSB/DBird.fsb";
    auto* mod=FMUSIC_LoadSongEx(file.c_str(),0,0,0,nullptr,0);assert(mod && FMUSIC_GetNumSamples(mod)==3);
    for(int i=0;i<3;++i){
        auto* sample=FMUSIC_GetSample(mod,i);assert(sample);int freq,vol,pan,pri;assert(FSOUND_Sample_GetDefaults(sample,&freq,&vol,&pan,&pri));assert(freq==16000);
        assert(FSOUND_Sample_SetMode(sample,FSOUND_LOOP_NORMAL));
        int channel=FSOUND_PlaySoundEx(FSOUND_FREE,sample,nullptr,1);assert(channel>=0);assert(FSOUND_SetPan(channel,0));assert(FSOUND_SetPaused(channel,0));
        float peak=observe(channel);assert(peak>0.00001f && peak<=1);
        assert(FSOUND_SetPaused(channel,1));SDL_Delay(40);auto paused=BarnyardLinux::audioSnapshot();assert(paused.left==0 && paused.right==0);
        assert(FSOUND_StopSound(channel));printf("PASS real FSB sample=%d frequency=%d observed_callback_peak=%.6f\n",i,freq,peak);
    }
    assert(FMUSIC_FreeSong(mod));
    auto* stream=FSOUND_Stream_Open(file.c_str(),FSOUND_LOOP_NORMAL,0,0);assert(stream && FSOUND_Stream_GetNumSubStreams(stream)==3);
    for(int i=0;i<3;++i){
        assert(FSOUND_Stream_SetSubStream(stream,i));int channel=FSOUND_Stream_PlayEx(FSOUND_FREE,stream,nullptr,0);assert(channel>=0);
        float peak=observe(channel);assert(peak>0.00001f && peak<=1);assert(FSOUND_Stream_Stop(stream));
        printf("PASS real FSB stream/substream=%d observed_callback_peak=%.6f\n",i,peak);
    }
    assert(FSOUND_Stream_Close(stream));unsigned used,peak;FSOUND_GetMemoryStats(&used,&peak);assert(used==0 && peak>0);
    FSOUND_Close();auto callbacks=BarnyardLinux::audioSnapshot().callbacks;SDL_Delay(30);assert(BarnyardLinux::audioSnapshot().callbacks==callbacks);
    Toshi::TFileManager::Destroy();fs::remove_all(user);SDL_Quit();
    puts("PASS: real retail FSB3/IMA three samples + three substreams, SDL callback nonzero PCM, pause/stop/free/device shutdown; dummy audio, NOT speaker/game audio integration");
}

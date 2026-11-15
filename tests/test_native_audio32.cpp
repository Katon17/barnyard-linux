#include <pch.h>
#include <fmod/fmod.h>
#include "Platform/Linux/NativeAudio.h"
#include <cassert>
#include <vector>
#include <cstring>
#include <thread>
#include <chrono>
#include <unistd.h>
#include <SDL2/SDL.h>
#include <filesystem>
#include <fstream>
#include <File/TFile.h>
__attribute__((constructor(101))) static void allocator(){if(!Toshi::TMemory::Initialise(32*1024*1024,0))_exit(90);}
static void put32(std::vector<unsigned char>& b,int at,uint32_t v){memcpy(b.data()+at,&v,4);}
static void put16(std::vector<unsigned char>& b,int at,uint16_t v){memcpy(b.data()+at,&v,2);}
static std::vector<unsigned char> bank(bool ima=false){
    const int frames=ima?64:2048, bytes=ima?36:frames*2;
    std::vector<unsigned char> b(24+64+bytes);
    memcpy(b.data(),"FSB3",4);put32(b,4,1);put32(b,8,64);put32(b,12,bytes);put32(b,16,0x30000);
    put16(b,24,64);memcpy(b.data()+26,"synthetic",10);
    put32(b,24+32,frames);put32(b,24+36,bytes);put32(b,24+44,frames-1);
    put32(b,24+48,FSOUND_LOOP_NORMAL|FSOUND_2D|(ima?FSOUND_IMAADPCM:FSOUND_16BITS));
    put32(b,24+52,44100);put16(b,24+56,255);put16(b,24+58,0);put16(b,24+60,128);put16(b,24+62,1);
    if(ima) put16(b,88,10000);else for(int i=0;i<frames;++i) put16(b,88+i*2,16384);
    return b;
}
static BarnyardLinux::AudioSnapshot next(){
    auto old=BarnyardLinux::audioSnapshot().callbacks;
    for(int i=0;i<200;++i){std::this_thread::sleep_for(std::chrono::milliseconds(5));auto s=BarnyardLinux::audioSnapshot();if(s.callbacks>old+1)return s;}
    assert(false && "SDL audio callback stalled");return {};
}
int main(){
    assert(!FSOUND_Init(22050,32,0));assert(FSOUND_Init(44100,32,0));
    assert(!FSOUND_SetOutput(FSOUND_OUTPUT_DSOUND));
    auto b=bank();auto* mod=FMUSIC_LoadSongEx((const char*)b.data(),0,b.size(),FSOUND_LOADMEMORY,nullptr,0);assert(mod);
    auto* sample=FMUSIC_GetSample(mod,0);assert(sample);
    int ch=FSOUND_PlaySoundEx(FSOUND_FREE,sample,nullptr,1);assert(ch>=0);auto snap=next();assert(snap.left==0 && snap.right==0);
    assert(FSOUND_SetPaused(ch,0));snap=next();assert(snap.left>0.49f && snap.left<0.51f && snap.right==0);
    assert(FSOUND_SetVolume(ch,128));snap=next();assert(snap.left>0.24f && snap.left<0.26f);
    assert(FSOUND_SetPan(ch,255));snap=next();assert(snap.left==0 && snap.right>0.24f);
    assert(FSOUND_SetPaused(ch,1));snap=next();assert(snap.left==0 && snap.right==0);
    assert(FSOUND_StopSound(ch));assert(FMUSIC_FreeSong(mod));
    b=bank(true);mod=FMUSIC_LoadSongEx((const char*)b.data(),0,b.size(),FSOUND_LOADMEMORY,nullptr,0);assert(mod);
    ch=FSOUND_PlaySoundEx(FSOUND_FREE,FMUSIC_GetSample(mod,0),nullptr,0);assert(ch>=0);snap=next();assert(snap.left>0.25f);
    assert(FMUSIC_FreeSong(mod));
    b=bank();auto* stream=FSOUND_Stream_Open((const char*)b.data(),FSOUND_LOADMEMORY|FSOUND_LOOP_NORMAL,0,b.size());assert(stream);
    ch=FSOUND_Stream_PlayEx(FSOUND_FREE,stream,nullptr,0);assert(ch>=0);snap=next();assert(snap.left>0.49f);
    assert(FSOUND_Stream_Stop(stream));assert(FSOUND_Stream_Close(stream));
    // Packed native file path from MakeInternalPath, with nonzero bank offset.
    char pattern[]="/tmp/barnyard-audio-XXXXXX";char* temp=mkdtemp(pattern);assert(temp);
    namespace fs=std::filesystem;fs::path root(temp),game=root/"game",user=root/"user";
    fs::create_directory(game);fs::create_directory(user);
    auto path=game/"packed.bin";
    {std::ofstream file(path,std::ios::binary);file<<std::string(17,'x');file.write((char*)b.data(),b.size());}
    setenv("BARNYARD_GAME_DIR",game.c_str(),1);setenv("BARNYARD_USER_DIR",user.c_str(),1);
    assert(Toshi::TFileManager::Create());
    stream=FSOUND_Stream_Open(path.c_str(),FSOUND_LOOP_NORMAL,17,b.size());assert(stream);
    ch=FSOUND_Stream_PlayEx(FSOUND_FREE,stream,nullptr,0);assert(ch>=0);snap=next();assert(snap.left>0.49f);
    assert(FSOUND_Stream_Close(stream));
    // File streaming must not inherit eager decode's frame-count or RAM limit.
    // Sparse synthetic banks: exact retail Music.fsb file SIZE (not contents),
    // plus a >256 MiB bank. Both have one long stereo PCM16 sample.
    for(uint32_t total:{264962972u, 88u+70u*1024*1024*4}) {
        auto h=bank();h.resize(88);const uint32_t bytes=total-88,frames=bytes/4;
        put32(h,12,bytes);put32(h,56,frames);put32(h,60,bytes);
        put32(h,68,frames-1);put32(h,72,FSOUND_LOOP_NORMAL|FSOUND_2D|FSOUND_16BITS|FSOUND_STEREO);
        put16(h,86,2);
        {std::ofstream f(path,std::ios::binary|std::ios::trunc);
         f.write((char*)h.data(),h.size());
         std::vector<int16_t> pcm(44100*2,16384);f.write((char*)pcm.data(),pcm.size()*2);
         f.seekp(total-1);f.put(0);}
        stream=FSOUND_Stream_Open(path.c_str(),FSOUND_LOOP_NORMAL,0,total);assert(stream);
        assert(FSOUND_Stream_GetLengthMs(stream)==int(1000.0*frames/44100));
        unsigned allocated,high;FSOUND_GetMemoryStats(&allocated,&high);assert(allocated<1024*1024);
        ch=FSOUND_Stream_PlayEx(FSOUND_FREE,stream,nullptr,0);assert(ch>=0);
        assert(FSOUND_SetPan(ch,128));
        snap=next();assert(snap.left>0.34f && snap.right>0.34f);
        assert(FSOUND_Stream_SetTime(stream,1000000));next();
        assert(FSOUND_Stream_Close(stream));
        printf("PASS long sparse stream: file_bytes=%u frames=%u; bounded audio allocation, PCM callback, seek\n",total,frames);
    }
    // More than 1 MiB of legitimate per-sample headers, not a fixed read window.
    const uint32_t count=16384,headers=count*80;
    std::vector<unsigned char> many(24+headers+count*2);
    memcpy(many.data(),"FSB3",4);put32(many,4,count);put32(many,8,headers);put32(many,12,count*2);put32(many,16,0x30000);
    for(uint32_t i=0;i<count;++i){
        int at=24+i*80;put16(many,at,80);put32(many,at+32,1);put32(many,at+36,2);
        put32(many,at+48,FSOUND_LOOP_NORMAL|FSOUND_2D|FSOUND_16BITS);put32(many,at+52,44100);
        put16(many,at+56,255);put16(many,at+60,128);put16(many,at+62,1);put16(many,24+headers+i*2,16384);
    }
    {std::ofstream f(path,std::ios::binary|std::ios::trunc);f.write((char*)many.data(),many.size());}
    stream=FSOUND_Stream_Open(path.c_str(),FSOUND_LOOP_NORMAL,0,many.size());assert(stream);
    assert(FSOUND_Stream_GetNumSubStreams(stream)==int(count));assert(FSOUND_Stream_SetSubStream(stream,count-1));
    ch=FSOUND_Stream_PlayEx(FSOUND_FREE,stream,nullptr,0);assert(ch>=0);snap=next();assert(snap.left>0.49f);
    assert(FSOUND_Stream_Close(stream));
    // Declared payload beyond EOF is a real error, not a silently empty stream.
    fs::resize_file(path,24+headers);assert(!FSOUND_Stream_Open(path.c_str(),0,0,0));
    Toshi::TFileManager::Destroy();fs::remove_all(root);
    puts("PASS >1 MiB complete stream header and rejection of truncated on-disk payload");
    // Parser rejects short headers/invalid counts/data lengths before sample decode.
    for(int len=0;len<88;++len) assert(!FMUSIC_LoadSongEx((const char*)b.data(),0,len,FSOUND_LOADMEMORY,nullptr,0));
    put32(b,4,0x7fffffff);assert(!FMUSIC_LoadSongEx((const char*)b.data(),0,b.size(),FSOUND_LOADMEMORY,nullptr,0));
    unsigned used,peak;FSOUND_GetMemoryStats(&used,&peak);assert(used==0 && peak>0);
    FSOUND_Close();auto stopped=BarnyardLinux::audioSnapshot().callbacks;std::this_thread::sleep_for(std::chrono::milliseconds(40));assert(stopped==BarnyardLinux::audioSnapshot().callbacks);
    for(int i=0;i<10;++i){assert(FSOUND_Init(44100,32,0));next();FSOUND_Close();}
    SDL_Quit();puts("PASS: real SDL FSB3 PCM16/IMA/stream mixer callback, volume/pan/pause/stop, malformed banks, allocation release, device close/join/reopen (dummy output, not speakers/retail banks)");
}

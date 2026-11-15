// Private retail Bink input, no decoded frame/audio files are written.
#include "Platform/Linux/NativeMovie.h"
#include <SDL2/SDL.h>
#include <cassert>
#include <chrono>
#include <thread>
#include <vector>
#include <cstring>
#include <cstdio>
int main(int argc,char** argv){
    assert(argc==2);static_assert(sizeof(void*)==4);
    NativeMovieMute(false);HBINK b=BinkOpen(argv[1],0);
    if(!b) fprintf(stderr,"open failed: %s\n",NativeMovieError());assert(b);
    assert(b->Width==640 && b->Height==480);
    // FFmpeg's Bink demuxer reports duration, but may leave nb_frames unset.
    // The actual EOF decode below must still produce exactly 35 frames.
    assert(b->Frames==35 || b->Frames==UINT32_MAX);
    assert(BinkDoFrame(b)==0);
    const int pitch=b->Width*4+16;
    std::vector<unsigned char> frame(size_t(pitch)*b->Height+16,0xa5);
    BinkCopyToBuffer(b,frame.data(),pitch,b->Height,0,0,BINKSURFACE32);
    for(unsigned y=0;y<b->Height;++y) for(int p=b->Width*4;p<pitch;++p)assert(frame[size_t(y)*pitch+p]==0xa5);
    assert(frame.back()==0xa5);
    std::vector<unsigned char> tile(80*4*9);
    BinkCopyToBuffer(b,tile.data(),80*4,9,13,17,BINKSURFACE32);
    for(int y=0;y<9;++y) assert(!memcmp(tile.data()+y*80*4,frame.data()+(17+y)*pitch+13*4,80*4));
    BinkPause(b,1);NativeMovieSystemPause(b,false);assert(NativeMoviePaused(b));
    NativeMovieSystemPause(b,true);BinkPause(b,0);assert(NativeMoviePaused(b));
    BinkNextFrame(b);std::this_thread::sleep_for(std::chrono::milliseconds(80));assert(BinkWait(b));
    NativeMovieSystemPause(b,false);assert(!NativeMoviePaused(b));
    int count=1;uint64_t digest=1469598103934665603ull;
    const auto start=std::chrono::steady_clock::now();
    while(!NativeMovieFinished(b)){
        assert(std::chrono::steady_clock::now()-start<std::chrono::seconds(8));
        if(!BinkWait(b)){
            const int code=BinkDoFrame(b);
            if(code<0)fprintf(stderr,"decode: %s\n",NativeMovieError());assert(code>=0);
            if(code==0){
                ++count;BinkCopyToBuffer(b,frame.data(),pitch,b->Height,0,0,BINKSURFACE32);
                for(unsigned y=0;y<b->Height;++y)for(unsigned x=0;x<b->Width*4;++x){digest^=frame[size_t(y)*pitch+x];digest*=1099511628211ull;}
            }
            BinkNextFrame(b);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    assert(count==35);
    auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count();
    assert(ms>=1000 && ms<8000);
    BinkClose(b);
    for(int i=0;i<3;++i){b=BinkOpen(argv[1],0);assert(b && BinkDoFrame(b)==0);BinkClose(b);}
    SDL_Quit();
    printf("PASS: retail BIKi/Bink video 640x480, decoded_frames=%d, timed_ms=%lld, checksum=%016llx, crop/pitch/canaries, manual+system pause, EOF, reopen; this sample has NO audio track; NOT GUI/gameplay\n",count,(long long)ms,(unsigned long long)digest);
}

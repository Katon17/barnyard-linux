#include "Platform/Linux/NativeMovie.h"
#include <SDL2/SDL.h>
#include <cassert>
#include <chrono>
#include <thread>
#include <vector>
#include <cstdio>
int main(int argc,char** argv){
    assert(argc==2);assert(!BinkOpen("/no/such/movie",0));
    NativeMovieMute(false);auto* movie=BinkOpen(argv[1],0);
    if(!movie) fprintf(stderr,"movie: %s\n",NativeMovieError());assert(movie);
    assert(movie->Width==64 && movie->Height==48);
    assert(BinkDoFrame(movie)==0);
    const size_t pitch=movie->Width*4+16;
    std::vector<unsigned char> pixels(pitch*movie->Height+16,0xa5);
    BinkCopyToBuffer(movie,pixels.data(),pitch,movie->Height,0,0,BINKSURFACE32);
    // Generated fixture is solid red. Native surface is BGRA.
    assert(pixels[0]<10 && pixels[1]<10 && pixels[2]>240 && pixels[3]==255);
    assert(pixels[movie->Width*4]==0xa5 && pixels.back()==0xa5);
    std::vector<unsigned char> crop(16*4,0);
    BinkCopyToBuffer(movie,crop.data(),16*4,1,4,5,BINKSURFACE32);
    assert(crop[2]>240);
    BinkPause(movie,1);BinkNextFrame(movie);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));assert(BinkWait(movie));
    BinkPause(movie,0);
    auto start=std::chrono::steady_clock::now();int decoded=1;
    while(!NativeMovieFinished(movie)){
        assert(std::chrono::steady_clock::now()-start<std::chrono::seconds(5));
        if(!BinkWait(movie)) {int code=BinkDoFrame(movie);assert(code>=0);if(!code)++decoded;BinkNextFrame(movie);}
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    assert(decoded==5);BinkClose(movie);
    for(int i=0;i<10;++i){NativeMovieMute(true);movie=BinkOpen(argv[1],0);assert(movie);assert(BinkDoFrame(movie)==0);BinkClose(movie);}
    SDL_Quit();puts("PASS: real FFmpeg video/audio decode + SDL queue, BGRA/crop/pitch, pause/resume, EOF and repeated cleanup (synthetic FFV1/PCM fixture, not retail Bink)");
}

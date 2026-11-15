#include "NativeMovie.h"
#include <SDL2/SDL.h>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/channel_layout.h>
}
#include <chrono>
#include <vector>
#include <string>
#include <cstring>
#include <cmath>
#include <algorithm>

using Clock=std::chrono::steady_clock;
struct NativeMovieState {
    AVFormatContext* format=nullptr;
    AVCodecContext *video=nullptr,*audio=nullptr;
    AVFrame *frame=nullptr,*sound=nullptr;
    AVPacket* packet=nullptr;
    SwsContext* scale=nullptr;
    SwrContext* resample=nullptr;
    SDL_AudioDeviceID device=0;
    int videoIndex=-1,audioIndex=-1;
    bool eof=false,flushed=false,paused=false,manualPause=false,systemPause=false,failed=false,mute=false,audioInitialized=false;
    double nextTime=0,frameDuration=1.0/30.0,firstPts=AV_NOPTS_VALUE;
    Clock::time_point start=Clock::now(),pauseStart{};
    std::vector<uint8_t> pixels;
};
static thread_local std::string movieError;
static bool muteNext=false;
static int failure(NativeMovieState* s,const char* text) {movieError=text;if(s)s->failed=true;return -1;}
const char* NativeMovieError(){return movieError.c_str();}
void NativeMovieMute(bool value){muteNext=value;}
static AVCodecContext* openCodec(AVFormatContext* f,int index) {
    auto* codec=avcodec_find_decoder(f->streams[index]->codecpar->codec_id);
    if(!codec) return nullptr;
    auto* ctx=avcodec_alloc_context3(codec);
    if(!ctx) return nullptr;
    ctx->thread_count=2;
    if(avcodec_parameters_to_context(ctx,f->streams[index]->codecpar)<0 || avcodec_open2(ctx,codec,nullptr)<0) avcodec_free_context(&ctx);
    return ctx;
}
HBINK BinkOpen(const char* path,uint32_t flags) {
    if(!path || !*path || flags) {failure(nullptr,"Invalid native movie path/flags");return nullptr;}
    auto* b=new BINK;auto* s=new NativeMovieState;b->native=s;s->mute=muteNext;
    auto bad=[&](const char* why)->HBINK{failure(nullptr,why);BinkClose(b);return nullptr;};
    if(avformat_open_input(&s->format,path,nullptr,nullptr)<0 || avformat_find_stream_info(s->format,nullptr)<0) return bad("Cannot open/probe movie");
    s->videoIndex=av_find_best_stream(s->format,AVMEDIA_TYPE_VIDEO,-1,-1,nullptr,0);
    if(s->videoIndex<0 || !(s->video=openCodec(s->format,s->videoIndex))) return bad("Video decoder unavailable");
    int w=s->video->width,h=s->video->height;
    if(w<=0 || h<=0 || w>4096 || h>4096) return bad("Movie dimensions exceed native limits");
    b->Width=w;b->Height=h;
    auto* stream=s->format->streams[s->videoIndex];
    b->Frames=stream->nb_frames>0 && stream->nb_frames<UINT32_MAX ? uint32_t(stream->nb_frames) : UINT32_MAX;
    AVRational rate=av_guess_frame_rate(s->format,stream,nullptr);
    if(rate.num>0 && rate.den>0) s->frameDuration=av_q2d(av_inv_q(rate));
    if(!std::isfinite(s->frameDuration) || s->frameDuration<=0 || s->frameDuration>10) return bad("Invalid movie frame rate");
    s->frame=av_frame_alloc();s->sound=av_frame_alloc();s->packet=av_packet_alloc();
    if(!s->frame || !s->sound || !s->packet) return bad("Movie allocation failed");
    s->pixels.resize(size_t(w)*h*4);
    s->audioIndex=av_find_best_stream(s->format,AVMEDIA_TYPE_AUDIO,-1,-1,nullptr,0);
    if(s->audioIndex>=0) {
        if(!(s->audio=openCodec(s->format,s->audioIndex))) return bad("Movie audio decoder unavailable");
        if(s->audio->sample_rate<8000 || s->audio->sample_rate>192000 || s->audio->ch_layout.nb_channels<1 || s->audio->ch_layout.nb_channels>8) return bad("Invalid movie audio format");
        AVChannelLayout stereo=AV_CHANNEL_LAYOUT_STEREO;
        if(swr_alloc_set_opts2(&s->resample,&stereo,AV_SAMPLE_FMT_FLT,44100,&s->audio->ch_layout,s->audio->sample_fmt,s->audio->sample_rate,0,nullptr)<0 || swr_init(s->resample)<0) return bad("Movie resampler failed");
        if(!s->mute) {
            if(SDL_InitSubSystem(SDL_INIT_AUDIO)) return bad(SDL_GetError());
            s->audioInitialized=true;
            SDL_AudioSpec desired{};desired.freq=44100;desired.format=AUDIO_F32SYS;desired.channels=2;desired.samples=1024;
            s->device=SDL_OpenAudioDevice(nullptr,0,&desired,nullptr,0);
            if(!s->device) return bad(SDL_GetError());
            SDL_PauseAudioDevice(s->device,0);
        }
    }
    s->start=Clock::now();movieError.clear();return b;
}
void BinkClose(HBINK b) {
    if(!b)return;auto* s=b->native;
    if(s) {
        if(s->device) SDL_CloseAudioDevice(s->device);
        if(s->audioInitialized) SDL_QuitSubSystem(SDL_INIT_AUDIO);
        sws_freeContext(s->scale);swr_free(&s->resample);
        av_packet_free(&s->packet);av_frame_free(&s->frame);av_frame_free(&s->sound);
        avcodec_free_context(&s->video);avcodec_free_context(&s->audio);avformat_close_input(&s->format);
        delete s;
    }
    delete b;
}
static int audioFrames(NativeMovieState* s) {
    for(;;) {
        int result=avcodec_receive_frame(s->audio,s->sound);
        if(result==AVERROR(EAGAIN) || result==AVERROR_EOF) return 0;
        if(result<0) return failure(s,"Movie audio decode failed");
        int count=swr_get_out_samples(s->resample,s->sound->nb_samples);
        if(count<0 || count>441000) return failure(s,"Invalid decoded movie audio size");
        std::vector<float> samples(size_t(count)*2);
        uint8_t* output=reinterpret_cast<uint8_t*>(samples.data());
        count=swr_convert(s->resample,&output,count,const_cast<const uint8_t**>(s->sound->extended_data),s->sound->nb_samples);
        av_frame_unref(s->sound);
        if(count<0) return failure(s,"Movie audio conversion failed");
        if(s->device && !s->mute) {
            if(SDL_GetQueuedAudioSize(s->device)>44100*2*4*4) return failure(s,"Movie audio lead exceeded four seconds");
            if(SDL_QueueAudio(s->device,samples.data(),count*2*sizeof(float))) return failure(s,SDL_GetError());
        }
    }
}
int BinkDoFrame(HBINK b) {
    if(!b || !b->native) return -1;auto* s=b->native;
    if(s->failed) return -1;
    for(;;) {
        int result=avcodec_receive_frame(s->video,s->frame);
        if(result==0) {
            if(s->frame->width!=int(b->Width) || s->frame->height!=int(b->Height)) return failure(s,"Dynamic movie dimensions unsupported");
            s->scale=sws_getCachedContext(s->scale,b->Width,b->Height,AVPixelFormat(s->frame->format),b->Width,b->Height,AV_PIX_FMT_BGRA,SWS_BILINEAR,nullptr,nullptr,nullptr);
            if(!s->scale) return failure(s,"Movie pixel converter failed");
            uint8_t* dest=s->pixels.data();int pitch=b->Width*4;
            if(sws_scale(s->scale,s->frame->data,s->frame->linesize,0,b->Height,&dest,&pitch)!=int(b->Height)) return failure(s,"Movie frame conversion failed");
            if(s->frame->best_effort_timestamp!=AV_NOPTS_VALUE) {
                double pts=s->frame->best_effort_timestamp*av_q2d(s->format->streams[s->videoIndex]->time_base);
                if(s->firstPts==double(AV_NOPTS_VALUE)) s->firstPts=pts;
                s->nextTime=std::max(0.0,pts-s->firstPts);
            }
            av_frame_unref(s->frame);++b->LastFrameNum;return 0;
        }
        if(result==AVERROR_EOF) {s->eof=true;return 1;}
        if(result!=AVERROR(EAGAIN)) return failure(s,"Movie video decode failed");
        if(s->flushed) {s->eof=true;return 1;}
        result=av_read_frame(s->format,s->packet);
        if(result<0) {
            if(result!=AVERROR_EOF) return failure(s,"Movie demux failed");
            s->flushed=true;
            if(avcodec_send_packet(s->video,nullptr)<0) return failure(s,"Movie video flush failed");
            if(s->audio) {avcodec_send_packet(s->audio,nullptr);if(audioFrames(s)<0)return -1;}
            continue;
        }
        int sent=0;
        if(s->packet->stream_index==s->videoIndex) sent=avcodec_send_packet(s->video,s->packet);
        else if(s->audio && s->packet->stream_index==s->audioIndex) {
            sent=avcodec_send_packet(s->audio,s->packet);
            if(sent>=0) sent=audioFrames(s);
        }
        av_packet_unref(s->packet);
        if(sent<0) return failure(s,"Movie decoder rejected packet");
    }
}
int BinkWait(HBINK b) {
    if(!b || !b->native) return 1;auto* s=b->native;
    return s->paused || std::chrono::duration<double>(Clock::now()-s->start).count()<s->nextTime;
}
void BinkNextFrame(HBINK b) {if(b && b->native) b->native->nextTime+=b->native->frameDuration;}
static void applyPause(HBINK b,int pause) {
    if(!b || !b->native)return;auto* s=b->native;
    if(bool(pause)==s->paused)return;
    if(pause) s->pauseStart=Clock::now();else s->start+=Clock::now()-s->pauseStart;
    s->paused=pause;if(s->device) SDL_PauseAudioDevice(s->device,pause);
}
void BinkPause(HBINK b,int pause) {
    if(!b || !b->native) return;
    b->native->manualPause=pause;applyPause(b,pause || b->native->systemPause);
}
void NativeMovieSystemPause(HBINK b,bool pause) {
    if(!b || !b->native) return;
    b->native->systemPause=pause;applyPause(b,pause || b->native->manualPause);
}
bool NativeMoviePaused(HBINK b) {return b && b->native && b->native->paused;}
void BinkSetSoundOnOff(HBINK b,int enabled) {
    if(!b || !b->native)return;auto* s=b->native;s->mute=!enabled || muteNext;
    if(s->mute && s->device) SDL_ClearQueuedAudio(s->device);
}
bool NativeMovieFinished(HBINK b) {
    return !b || !b->native || b->native->failed || (b->native->eof && (!b->native->device || SDL_GetQueuedAudioSize(b->native->device)==0));
}
void BinkCopyToBuffer(HBINK b,void* destination,int pitch,uint32_t height,uint32_t x,uint32_t y,uint32_t format) {
    if(!b || !b->native || !destination || x>=b->Width || y>=b->Height || pitch<=0 || (format&~BINKCOPYALL)>1) return;
    const auto width=std::min<uint32_t>(b->Width-x,uint32_t(pitch/4));
    height=std::min(height,b->Height-y);
    for(uint32_t row=0;row<height;++row) memcpy(static_cast<uint8_t*>(destination)+size_t(row)*pitch,b->native->pixels.data()+(size_t(y+row)*b->Width+x)*4,size_t(width)*4);
}

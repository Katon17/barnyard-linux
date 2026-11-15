#pragma once
#include <cmath>
#include <cstdint>
namespace BarnyardLinux {
// Counts rendered GUI frame intervals against a monotonic real clock, not
// simulation delta/time scaling. No frame limiting, sleeping or heap allocation.
class FPSCounter {
public:
    void reset(){started_=false;sample_=false;start_=last_=fps_=0.0;frames_=0;}
    bool frame(double seconds) {
        if(!std::isfinite(seconds) || seconds<0.0){reset();return false;}
        if(!started_){started_=true;start_=last_=seconds;return false;}
        if(seconds<last_ || seconds-last_>2.0){reset();started_=true;start_=last_=seconds;return false;}
        last_=seconds;++frames_;
        const double elapsed=seconds-start_;
        if(elapsed+1e-9<0.5)return false;
        fps_=frames_/elapsed;sample_=true;start_=seconds;frames_=0;return true;
    }
    double fps() const{return fps_;}
    bool hasSample() const{return sample_;}
private:
    double start_=0.0,last_=0.0,fps_=0.0;
    uint32_t frames_=0;
    bool started_=false,sample_=false;
};
}

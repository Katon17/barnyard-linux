#include "Platform/Linux/NativeFPSCounter.h"
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
using BarnyardLinux::FPSCounter;
int main(){
 FPSCounter counter;assert(!counter.hasSample());assert(counter.fps()==0.0);
 for(double rate:{15.0,30.0,60.0,120.0,144.0}) {
  counter.reset();assert(!counter.frame(1000.0));int samples=0;
  for(int i=1;i<=static_cast<int>(rate*2);++i)if(counter.frame(1000.0+i/rate))++samples;
  assert(samples>=3);assert(counter.hasSample());assert(std::abs(counter.fps()-rate)<0.001);
 }
 counter.reset();assert(!counter.frame(0.0));
 assert(!counter.frame(0.1));assert(!counter.frame(0.4));assert(counter.frame(0.6));
 assert(std::abs(counter.fps()-5.0)<0.00001); // Three frame intervals / 0.6 sec, not reciprocal of last interval.
 assert(!counter.frame(10.0));assert(!counter.hasSample()); // Long focus/loading gap resets stale data.
 assert(!counter.frame(9.0));assert(!counter.hasSample()); // Clock reversal cannot underflow or show negative FPS.
 assert(!counter.frame(std::numeric_limits<double>::infinity()));assert(!counter.hasSample());
 assert(!counter.frame(std::numeric_limits<double>::quiet_NaN()));
 assert(!counter.frame(-1.0));
 counter.reset();assert(!counter.frame(0.0));
 for(int i=0;i<5;++i)assert(!counter.frame(0.0)); // Zero elapsed time: never divides by zero.
 assert(std::isfinite(counter.fps()));
 counter.reset();assert(!counter.frame(5000.0));assert(!counter.frame(5000.01));assert(!counter.hasSample());
 std::cout<<"PASS: rendered-frame mean at 15/30/60/120/144 Hz; variable timing; half-second refresh; reset/re-enable; focus gaps, reversal, NaN/infinity and zero elapsed protection. No FPS cap or frame sleep added.\n";
}

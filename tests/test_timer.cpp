#include "../src/Platform/Linux/THPTimer_Linux.h"
#include <cassert>
#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>
int main() {
    Toshi::THPTimer timer;
    assert(timer.GetDelta() == 0);
    assert(std::isfinite(timer.GetCurrentSeconds()));
    const auto before = timer.GetRaw64();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    timer.Update();
    assert(timer.GetRaw64() > before);
    assert(timer.GetDelta() >= 0.005f);
    // No strict upper deadline: CI scheduler may pause the process.
    assert(std::isfinite(timer.GetDelta()));
    timer.Reset();
    assert(timer.GetDelta() == 0);
    std::cout << "PASS: monotonic timer/reset/delta initialization\n";
}

#include "../src/Platform/Linux/T2Atomic_Linux.h"
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
int main() {
    Toshi::T2Atomic32 value(0);
    assert(value.Add(2) == 0);
    assert(value.Exchange(8) == 2);
    assert(value.CompareExchange(10, 7) == 8);
    assert(value.Load() == 8);
    assert(value.CompareExchange(10, 8) == 8);
    assert(value.Load() == 10);
    assert(value.Or(1) == 10);
    assert(value.And(3) == 11);
    assert(value.Load() == 3);
    value.Exchange(0);
    std::vector<std::thread> threads;
    for (int t=0; t<4; ++t) threads.emplace_back([&] { for (int i=0; i<25000; ++i) value.Add(1); });
    for (auto& t: threads) t.join();
    assert(value.Load() == 100000);
    // Repeat to exercise both wait-before-wake and wake-before-wait schedules.
    for (int round=0; round<100; ++round) {
        value.Exchange(0);
        std::thread waiter([&] { value.WaitForChange(0); assert(value.Load() == 1); });
        if (round % 2) std::this_thread::sleep_for(std::chrono::milliseconds(1));
        value.Exchange(1);
        value.Signal();
        waiter.join();
    }
    value.WaitForChange(0); // Returns immediately for a different value.
    std::cout << "PASS: atomics, 4-thread increments, CAS, futex wait/wake\n";
}

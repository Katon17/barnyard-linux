#pragma once
#include <cstdint>
namespace BarnyardLinux {
struct AudioSnapshot { uint64_t callbacks; float left,right; };
// Last actual SDL mixer callback, not a synthetic success signal.
AudioSnapshot audioSnapshot();
}

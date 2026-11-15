#pragma once
#include <string>
#include <string_view>
namespace BarnyardLinux {
struct NativeOptions {
    int width=800, height=600;
    float gamma=0.420849f, sensitivity=0.5f;
    bool windowed=true, borderless=false, highDetailGrass=false, controller=false;
    // Transactional: invalid input leaves this object unchanged.
    bool parse(std::string_view text, std::string& error);
    enum class LoadResult { Loaded, Missing, Invalid };
    LoadResult load(std::string& error); // Settings/Linux.cfg, through real Toshi overlay IO
};
}

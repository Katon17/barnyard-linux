#include "../src/Platform/Linux/AssetPaths.h"
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <unistd.h>
namespace fs = std::filesystem;
using BarnyardLinux::AssetPaths;
template<class F> void fails(F fn) {
    bool caught = false;
    try { fn(); } catch (const std::exception&) { caught = true; }
    assert(caught);
}
int main() {
    char pattern[] = "/tmp/barnyard-paths-XXXXXX";
    const char* result = mkdtemp(pattern);
    assert(result);
    fs::path tmp(result), root = tmp / "Game with spaces";
    struct Cleanup { fs::path p; ~Cleanup() { fs::remove_all(p); } } cleanup{tmp};
    fs::create_directories(root / "Data/GUI");
    std::ofstream(root / "Data/GUI/Fonts.trb") << "synthetic test fixture, not game data";
    AssetPaths paths(root);
    assert(paths.resolve("data/gui/fonts.trb") == root / "Data/GUI/Fonts.trb");
    assert(paths.resolve("DATA\\GUI\\FONTS.TRB") == root / "Data/GUI/Fonts.trb");
    assert(paths.resolve("./Data//GUI/fonts.trb") == root / "Data/GUI/Fonts.trb");
    fails([&] { paths.resolve("/etc/passwd"); });
    fails([&] { paths.resolve("../outside"); });
    fails([&] { paths.resolve("data/../Data"); });
    fails([&] { paths.resolve("C:\\Data\\file"); });
    fails([&] { paths.resolve(std::string("data\0hidden", 11)); });
    fails([&] { paths.resolve("data/gui/missing.trb"); });
    fs::create_directory_symlink(tmp, root / "escape");
    paths.clear();
    fails([&] { paths.resolve("escape"); });
    fs::create_directory_symlink(root / "Data/GUI", root / "inside");
    paths.clear();
    assert(paths.resolve("inside/fonts.trb") == root / "Data/GUI/Fonts.trb");
    std::ofstream(root / "Data/GUI/FONTS.TRB") << "collision";
    paths.clear();
    fails([&] { paths.resolve("data/gui/fonts.trb"); });
    fs::remove(root / "Data/GUI/FONTS.TRB");
    paths.clear();
    assert(fs::is_regular_file(paths.resolve("Data/GUI/Fonts.trb")));
    std::cout << "PASS: case/slash/cache/collisions/traversal/symlinks\n";
}

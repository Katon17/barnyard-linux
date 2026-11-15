#!/usr/bin/env bash
# Requires eight PRIVATE user-supplied files; never downloads or bundles assets.
set -euo pipefail
cd "$(dirname "$0")/.."
if (( $# != 1 )); then echo 'Usage: bash scripts/test-retail-sample32.sh /private/sample/root' >&2; exit 64; fi
sample=$(realpath -e -- "$1")
mkdir -p build/retail-tests reports/retail-sample
bash scripts/build-core32.sh > reports/retail-sample/core-build.txt 2>&1
cxx=${CXX:-g++}
flags=(-m32 -std=c++20 -D_FILE_OFFSET_BITS=64 -fshort-wchar -fno-strict-aliasing
 -DTOSHI_SKU_LINUX -DTRENDERINTERFACE_GL -DBARNYARD_COMMUNITY_PATCH -DTOSHI_USER_ENGINE
 -ffunction-sections -fdata-sections -include src/Platform/Linux/TCRT_Linux.h
 -I src -I vendor/openbarnyard/Toshi/Source -I vendor/openbarnyard/OpenBarnyard/Source
 -I vendor/openbarnyard/PrimeDebug/Source -I vendor/openbarnyard/Toshi/Vendor/Opcode/Source
 -I vendor/openbarnyard/Toshi/Vendor/fmod/include -I vendor/openbarnyard/Toshi/Vendor/bink/include
 -I vendor/openbarnyard/Toshi/Vendor/imgui -I /usr/include/stb)
"$cxx" "${flags[@]}" tests/test_retail_trb32.cpp artifacts/core32/*.o -Wl,--gc-sections -pthread -ldl \
 -o build/retail-tests/test-trb > reports/retail-sample/trb-compile.txt 2>&1
timeout 30 build/retail-tests/test-trb "$sample" | tee reports/retail-sample/trb.txt
"$cxx" -m32 -std=c++20 -I src tests/test_retail_movie32.cpp src/Platform/Linux/NativeMovie.cpp \
 -lavformat -lavcodec -lavutil -lswscale -lswresample -lSDL2 -o build/retail-tests/test-movie
SDL_AUDIODRIVER=dummy timeout 20 build/retail-tests/test-movie "$sample/Data/Movies/GoatOnFence.bik" | tee reports/retail-sample/movie.txt
"$cxx" "${flags[@]}" -c src/Platform/Linux/AFMOD_Linux.cpp -o build/retail-tests/AFMOD_Linux.o > reports/retail-sample/audio-compile.txt 2>&1
"$cxx" "${flags[@]}" tests/test_retail_audio32.cpp build/retail-tests/AFMOD_Linux.o artifacts/core32/*.o \
 -Wl,--gc-sections -pthread -ldl -lSDL2 -o build/retail-tests/test-audio > reports/retail-sample/audio-link.txt 2>&1
SDL_AUDIODRIVER=dummy timeout 20 build/retail-tests/test-audio "$sample" | tee reports/retail-sample/audio.txt

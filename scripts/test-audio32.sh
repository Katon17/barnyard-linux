#!/usr/bin/env bash
# Native audio regressions only, including sparse long-file streaming boundaries.
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build/media32 reports/media32
bash scripts/build-core32.sh > reports/media32/core-build.txt 2>&1
cxx=${CXX:-g++}
flags=(-m32 -std=c++20 -D_FILE_OFFSET_BITS=64 -fshort-wchar -fno-strict-aliasing
 -DTOSHI_SKU_LINUX -DTRENDERINTERFACE_GL -DBARNYARD_COMMUNITY_PATCH -DTOSHI_USER_ENGINE
 -ffunction-sections -fdata-sections -include src/Platform/Linux/TCRT_Linux.h
 -I src -I vendor/openbarnyard/Toshi/Source -I vendor/openbarnyard/OpenBarnyard/Source
 -I vendor/openbarnyard/PrimeDebug/Source -I vendor/openbarnyard/Toshi/Vendor/Opcode/Source
 -I vendor/openbarnyard/Toshi/Vendor/fmod/include -I vendor/openbarnyard/Toshi/Vendor/bink/include
 -I vendor/openbarnyard/Toshi/Vendor/imgui -I /usr/include/stb)
"$cxx" "${flags[@]}" -c src/Platform/Linux/AFMOD_Linux.cpp -o build/media32/AFMOD_Linux.o > reports/media32/audio-compile.txt 2>&1
"$cxx" "${flags[@]}" tests/test_native_audio32.cpp build/media32/AFMOD_Linux.o artifacts/core32/*.o \
 -Wl,--gc-sections -pthread -ldl -lSDL2 -o build/media32/test-audio > reports/media32/audio-link.txt 2>&1
SDL_AUDIODRIVER=dummy timeout 40 build/media32/test-audio | tee reports/media32/audio.txt

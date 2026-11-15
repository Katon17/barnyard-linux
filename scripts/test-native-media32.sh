#!/usr/bin/env bash
# Real save/decoder/mixer component tests. No proprietary media/gameplay claim.
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build/media32 reports/media32
bash scripts/build-core32.sh > reports/media32/core-build.txt 2>&1
cxx=${CXX:-g++}
for mode in host i386; do
 flags=(); [[ $mode == host ]] || flags+=(-m32)
 "$cxx" "${flags[@]}" -std=c++20 -D_FILE_OFFSET_BITS=64 -Wall -Wextra -Werror -I src \
  tests/test_native_save.cpp src/Platform/Linux/NativeSave.cpp -o "build/media32/test-save-$mode"
 timeout 15 "build/media32/test-save-$mode" | tee "reports/media32/save-$mode.txt"
done
ffmpeg -v error -y -f lavfi -i color=c=red:s=64x48:r=10:d=0.5 \
 -f lavfi -i sine=frequency=440:sample_rate=44100:duration=0.5 \
 -c:v ffv1 -c:a pcm_s16le -shortest build/media32/movie.mkv
"$cxx" -m32 -std=c++20 -I src tests/test_native_movie32.cpp src/Platform/Linux/NativeMovie.cpp \
 -lavformat -lavcodec -lavutil -lswscale -lswresample -lSDL2 -o build/media32/test-movie
SDL_AUDIODRIVER=dummy timeout 20 build/media32/test-movie build/media32/movie.mkv | tee reports/media32/movie.txt
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
SDL_AUDIODRIVER=dummy timeout 30 build/media32/test-audio | tee reports/media32/audio.txt

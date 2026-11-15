#!/usr/bin/env bash
# Real core application lifecycle test, not the Barnyard main/game executable.
set -euo pipefail
cd "$(dirname "$0")/.."
# Default clean dependency build; --reuse requires already current integration objects.
if [[ ${1:-} != --reuse ]]; then bash scripts/build-render32.sh > reports/render32-build.txt 2>&1; fi
mkdir -p artifacts/startup32 reports/startup32
flags=(-m32 -std=c++20 -D_FILE_OFFSET_BITS=64 -fshort-wchar -fno-strict-aliasing
 -DTOSHI_SKU_LINUX -DTRENDERINTERFACE_GL -DBARNYARD_COMMUNITY_PATCH
 -DTOSHI_USER_ENGINE -ffunction-sections -fdata-sections
 -include src/Platform/Linux/TCRT_Linux.h -I src -I vendor/openbarnyard/Toshi/Source
 -I vendor/openbarnyard/PrimeDebug/Source)
sources=(src/Platform/Linux/NativeOptions.cpp src/Platform/Linux/NativeStartup.cpp src/Platform/Linux/TDebugConsole_Linux.cpp
 src/Platform/Linux/TUtil_Linux.cpp vendor/openbarnyard/Toshi/Source/Toshi/TUtil.cpp
 vendor/openbarnyard/Toshi/Source/Toshi/TApplication.cpp vendor/openbarnyard/Toshi/Source/Toshi/TDateTime.cpp
 vendor/openbarnyard/Toshi/Source/File/TLogFile.cpp)
objects=()
for source in "${sources[@]}"; do
 name=$(basename "$source" .cpp)
 "${CXX:-g++}" "${flags[@]}" -c "$source" -o "artifacts/startup32/$name.o" > "reports/startup32/$name.log" 2>&1
 objects+=("artifacts/startup32/$name.o")
done
for obj in artifacts/core32/*.o artifacts/render32/*.o artifacts/gl32/*.o; do
 case "$obj" in artifacts/render32/TUtil.o) continue;; esac
 objects+=("$obj")
done
"${CXX:-g++}" "${flags[@]}" tests/test_startup32.cpp "${objects[@]}" -Wl,--gc-sections \
 -pthread -ldl -lSDL2 -lGLEW -lGL -o artifacts/test_startup32 > reports/startup32/link.log 2>&1
timeout 30 artifacts/test_startup32 > reports/startup32/integration-test.txt 2>&1
cat reports/startup32/integration-test.txt

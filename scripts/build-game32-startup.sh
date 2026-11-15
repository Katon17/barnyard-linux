#!/usr/bin/env bash
# Compile selected real game units. NOT a link/gameplay gate or game executable.
set -euo pipefail
cd "$(dirname "$0")/.."
bash scripts/bootstrap.sh
mkdir -p artifacts/game32 reports/game32
rm -f artifacts/game32/*.o
flags=(-m32 -std=c++20 -D_FILE_OFFSET_BITS=64 -fshort-wchar -fno-strict-aliasing
 -DTOSHI_SKU_LINUX -DTRENDERINTERFACE_GL -DBARNYARD_COMMUNITY_PATCH
 -DTOSHI_USER_ENGINE -DTOSHI_RELEASE -DICE_NO_DLL -DBAN_OPCODE_AUTOLINK
 -ffunction-sections -fdata-sections -fmax-errors=8
 -include src/Platform/Linux/TCRT_Linux.h -I src
 -I vendor/openbarnyard/Toshi/Source -I vendor/openbarnyard/Toshi/Resources
 -I vendor/openbarnyard/OpenBarnyard/Source -I vendor/openbarnyard/PrimeDebug/Source
 -I vendor/openbarnyard/Toshi/Vendor/Opcode/Source
 -I vendor/openbarnyard/Toshi/Vendor/fmod/include -I vendor/openbarnyard/Toshi/Vendor/bink/include
 -I vendor/openbarnyard/Toshi/Vendor/imgui -I /usr/include/stb)
sources=(tests/test_game_layout32.cpp vendor/openbarnyard/OpenBarnyard/Source/AppBoot.cpp
 vendor/openbarnyard/OpenBarnyard/Source/ClassLink_Win.cpp
 vendor/openbarnyard/OpenBarnyard/Source/Platform/Windows/ADisplayModes_Win.cpp
 vendor/openbarnyard/OpenBarnyard/Source/Platform/Windows/ARenderer_Win.cpp
 vendor/openbarnyard/OpenBarnyard/Source/Platform/Windows/Input/AInputHandler_Win.cpp
 vendor/openbarnyard/OpenBarnyard/Source/Platform/GL/*.cpp
 vendor/openbarnyard/OpenBarnyard/Source/Platform/GL/*/*.cpp
 vendor/openbarnyard/OpenBarnyard/Source/Tasks/ARootTask.cpp
 vendor/openbarnyard/OpenBarnyard/Source/Memory/AMemory.cpp
 vendor/openbarnyard/OpenBarnyard/Source/Locale/ALocaleManager.cpp
 vendor/openbarnyard/OpenBarnyard/Source/AOptions.cpp)
: > reports/game32/summary.txt
failed=0
for source in "${sources[@]}"; do
 name=${source#vendor/openbarnyard/}; name=${name//\//_}; name=${name%.cpp}
 rm -f "artifacts/game32/$name.o"
 if "${CXX:-g++}" "${flags[@]}" -c "$source" -o "artifacts/game32/$name.o" > "reports/game32/$name.log" 2>&1; then
  echo "PASS $source" | tee -a reports/game32/summary.txt
 else
  echo "FAIL $source" | tee -a reports/game32/summary.txt
  grep -E 'error:|fatal error:' "reports/game32/$name.log" | head -8 || true
  failed=$((failed + 1))
 fi
done
if (( failed )); then echo "$failed selected game units failed; not a game build"; exit 1; fi
echo 'Selected game compile gate PASS. Game linking/gameplay NOT established.'

#!/usr/bin/env bash
# Links a real engine renderer integration test, NOT the Barnyard executable.
set -euo pipefail
cd "$(dirname "$0")/.."
bash scripts/build-core32.sh > reports/render32-core.txt 2>&1
bash scripts/build-gl32.sh > reports/gl32-build.txt 2>&1
mkdir -p artifacts/render32 reports/render32
rm -f artifacts/render32/*.o
flags=(-m32 -std=c++20 -D_FILE_OFFSET_BITS=64 -fshort-wchar -fno-strict-aliasing
 -DTOSHI_SKU_LINUX -DTRENDERINTERFACE_GL -DBARNYARD_COMMUNITY_PATCH
 -DTOSHI_USER_ENGINE -DTOSHI_NO_LOGS -ffunction-sections -fdata-sections
 -include src/Platform/Linux/TCRT_Linux.h -I src -I vendor/openbarnyard/Toshi/Source)
sources=(vendor/openbarnyard/Toshi/Source/Render/*.cpp vendor/openbarnyard/Toshi/Source/Math/*.cpp)
for f in TClass TObject TFreeList T2RedBlackTree T2DList T2SortedList TPString8 TSystem TScheduler TTask TEvent TUtil TSceneObject T2StringTraits TPriList TFifo; do
 sources+=("vendor/openbarnyard/Toshi/Source/Toshi/$f.cpp")
done
sources+=(vendor/openbarnyard/Toshi/Source/ToshiTools/T2DynamicVector.cpp
 src/Platform/Linux/TThread_Linux.cpp vendor/openbarnyard/Toshi/Source/File/TFileStream.cpp
 vendor/openbarnyard/Toshi/Source/Thread/TSemaphore.cpp)
sources+=(vendor/openbarnyard/Toshi/Source/Input/TInputInterface.cpp vendor/openbarnyard/Toshi/Source/Input/TInputDevice.cpp
 vendor/openbarnyard/Toshi/Source/Input/TInputDeviceKeyboard.cpp vendor/openbarnyard/Toshi/Source/Input/TInputDeviceMouse.cpp
 src/Platform/Linux/Input/*.cpp)
: > reports/render32/summary.txt
failed=0
for source in "${sources[@]}"; do
 name=$(basename "$source" .cpp)
 if "${CXX:-g++}" "${flags[@]}" -c "$source" -o "artifacts/render32/$name.o" > "reports/render32/$name.log" 2>&1; then
  echo "PASS $source" | tee -a reports/render32/summary.txt
 else
  echo "FAIL $source" | tee -a reports/render32/summary.txt
  grep -E 'error:|fatal error:' "reports/render32/$name.log" | head -10 || true
  failed=$((failed + 1))
 fi
done
(( failed == 0 )) || exit 1
"${CXX:-g++}" "${flags[@]}" tests/test_renderer32.cpp artifacts/render32/*.o artifacts/gl32/*.o artifacts/core32/*.o -Wl,--gc-sections -pthread -ldl -lSDL2 -lGLEW -lGL -o artifacts/test_renderer32 > reports/render32/link.log 2>&1
xvfb-run -a -s '-screen 0 1280x720x24' timeout 30 artifacts/test_renderer32 > reports/render32/integration-test.txt 2>&1
cat reports/render32/integration-test.txt

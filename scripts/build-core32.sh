#!/usr/bin/env bash
# Native x86 engine integration, not the game executable. No assertions disabled.
set -euo pipefail
cd "$(dirname "$0")/.."
bash scripts/bootstrap.sh
mkdir -p artifacts/core32 reports/core32
rm -f artifacts/core32/*.o
cxx=${CXX:-g++}
flags=(-m32 -std=c++20 -D_FILE_OFFSET_BITS=64 -fshort-wchar -fno-strict-aliasing
       -DTOSHI_SKU_LINUX -DTOSHI_USER_ENGINE -DTOSHI_NO_LOGS
       -ffunction-sections -fdata-sections -include src/Platform/Linux/TCRT_Linux.h
       -I src -I vendor/openbarnyard/Toshi/Source)
upstream=(Core/TMemory.cpp File/TFile.cpp File/TFileSystem.cpp File/TFileManager.cpp
          File/TMemFile.cpp File/TNullFile.cpp File/TTRB.cpp File/TTSF.cpp
          File/TCompress_Decompress.cpp Toshi/TString8.cpp Toshi/TString16.cpp
          Toshi/T2String8.cpp Toshi/T2String16.cpp Toshi/TStringManager.cpp
          Toshi/T2Allocator.cpp Toshi/TDList.cpp Thread/TMutex.cpp Thread/T2AtomicMutex.cpp)
local_sources=(AssetPaths TNativeFile_Linux TFileMount_Linux TDebug_Linux T2Atomic_Linux
               THPTimer_Linux TStringHelper_Linux TWideFormat_Linux TSystemDLL_Linux)
objects=()
: > reports/core32/summary.txt
for source in "${upstream[@]}"; do
    name=${source//\//_}; name=${name%.cpp}
    "$cxx" "${flags[@]}" -c "vendor/openbarnyard/Toshi/Source/$source" -o "artifacts/core32/$name.o" > "reports/core32/$name.log" 2>&1
    objects+=("artifacts/core32/$name.o")
    echo "PASS upstream $source" | tee -a reports/core32/summary.txt
done
for name in "${local_sources[@]}"; do
    "$cxx" "${flags[@]}" -c "src/Platform/Linux/$name.cpp" -o "artifacts/core32/Linux_$name.o" > "reports/core32/Linux_$name.log" 2>&1
    objects+=("artifacts/core32/Linux_$name.o")
    echo "PASS Linux $name" | tee -a reports/core32/summary.txt
done
"$cxx" "${flags[@]}" tests/test_engine_io32.cpp "${objects[@]}" -Wl,--gc-sections -pthread -ldl -o artifacts/test_engine_io32 > reports/core32/link.log 2>&1
file artifacts/test_engine_io32 | tee reports/core32/executable-type.txt
timeout 20 artifacts/test_engine_io32 > reports/core32/integration-test.txt 2>&1
cat reports/core32/integration-test.txt
echo 'This integration test is not Barnyard gameplay. Native game runtime still incomplete.'

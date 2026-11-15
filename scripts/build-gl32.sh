#!/usr/bin/env bash
# Compile the real upstream GL renderer plus Linux window adapter. Not a game build.
set -euo pipefail
cd "$(dirname "$0")/.."
bash scripts/bootstrap.sh
mkdir -p artifacts/gl32 reports/gl32
rm -f artifacts/gl32/*.o
flags=(-m32 -std=c++20 -D_FILE_OFFSET_BITS=64 -fshort-wchar -fno-strict-aliasing
 -DTOSHI_SKU_LINUX -DTRENDERINTERFACE_GL -DBARNYARD_COMMUNITY_PATCH
 -DTOSHI_USER_ENGINE -DTOSHI_NO_LOGS -ffunction-sections -fdata-sections
 -include src/Platform/Linux/TCRT_Linux.h -I src -I vendor/openbarnyard/Toshi/Source -I /usr/include/stb)
: > reports/gl32/summary.txt
failed=0
sources=(src/Platform/Linux/NativeWindow.cpp src/Platform/Linux/TWindow_Linux.cpp)
for f in vendor/openbarnyard/Toshi/Source/Platform/GL/*.cpp; do
 case "$f" in */TMSWindow.cpp) continue;; esac
 sources+=("$f")
done
for source in "${sources[@]}"; do
 name=$(basename "$source" .cpp)
 if "${CXX:-g++}" "${flags[@]}" -c "$source" -o "artifacts/gl32/$name.o" > "reports/gl32/$name.log" 2>&1; then
   echo "PASS $source" | tee -a reports/gl32/summary.txt
 else
   echo "FAIL $source" | tee -a reports/gl32/summary.txt
   grep -E 'error:|fatal error:' "reports/gl32/$name.log" | head -12 || true
   failed=$((failed + 1))
 fi
done
if (( failed )); then echo "$failed compile units failed"; exit 1; fi
echo 'GL compile gate PASS. This does not establish runtime linking or gameplay.'

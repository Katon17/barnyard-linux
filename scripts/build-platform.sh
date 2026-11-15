#!/usr/bin/env bash
# Builds early native platform and upstream math objects, NOT a game executable.
set -euo pipefail
cd "$(dirname "$0")/.."
bash scripts/bootstrap.sh
mkdir -p artifacts/math reports
cxx=${CXX:-g++}
flags=(-std=c++20 -fshort-wchar -fno-strict-aliasing -DTOSHI_SKU_LINUX -I src -I vendor/openbarnyard/Toshi/Source)
"$cxx" "${flags[@]}" -fsyntax-only tests/upstream_header_probe.cpp > reports/upstream-header-patched.log 2>&1
: > reports/math-build-summary.txt
objects=()
for source in vendor/openbarnyard/Toshi/Source/Math/*.cpp; do
    name=$(basename "$source" .cpp)
    "$cxx" "${flags[@]}" -c "$source" -o "artifacts/math/$name.o" > "reports/math-$name.log" 2>&1
    printf 'PASS %s\n' "$name" | tee -a reports/math-build-summary.txt
    objects+=("artifacts/math/$name.o")
done
ar rcs artifacts/libtoshi-math.a "${objects[@]}"
"$cxx" -std=c++20 -O2 -pthread -c src/Platform/Linux/AssetPaths.cpp -o artifacts/AssetPaths.o
"$cxx" "${flags[@]}" -O2 -pthread -c src/Platform/Linux/T2Atomic_Linux.cpp -o artifacts/T2Atomic_Linux.o
"$cxx" "${flags[@]}" -O2 -pthread -c src/Platform/Linux/THPTimer_Linux.cpp -o artifacts/THPTimer_Linux.o
ar rcs artifacts/libbarnyard-platform.a artifacts/AssetPaths.o artifacts/T2Atomic_Linux.o artifacts/THPTimer_Linux.o
echo 'PASS: upstream header, 10 math translation units, and platform archives built.'
echo 'NOT produced: game executable or ready-to-play release .deb.'

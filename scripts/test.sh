#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
cxx=${CXX:-g++}
for mode in host utf16; do
  flags=()
  if [[ $mode == utf16 ]]; then flags+=(-fshort-wchar -fno-builtin-wcslen); fi
  echo "=== string subset / $mode ==="
  "$cxx" -std=c++20 -Wall -Wextra -Werror -O1 "${flags[@]}" \
    tests/test_stringhelper.cpp src/Platform/Linux/TStringHelper_Linux.cpp src/Platform/Linux/TWideFormat_Linux.cpp -o "$work/strings"
  "$work/strings"
  echo "=== Win32 type widths / $mode ==="
  "$cxx" -std=c++20 -Wall -Wextra -Werror -DTOSHI_SKU_LINUX "${flags[@]}" \
    tests/test_types.cpp -o "$work/types"
  "$work/types"
done
echo '=== native asset paths ==='
"$cxx" -std=c++20 -Wall -Wextra -Werror -O2 -pthread \
  tests/test_assetpaths.cpp src/Platform/Linux/AssetPaths.cpp -o "$work/paths"
"$work/paths"
if [[ -d vendor/openbarnyard/Toshi/Source ]]; then
  echo '=== Linux atomic backend (uses pinned upstream typedefs) ==='
  "$cxx" -std=c++20 -Wall -Wextra -Werror -O2 -pthread -DTOSHI_SKU_LINUX \
    -I src -I vendor/openbarnyard/Toshi/Source \
    tests/test_atomic.cpp src/Platform/Linux/T2Atomic_Linux.cpp -o "$work/atomic"
  timeout 15 "$work/atomic"
  echo '=== monotonic Linux timer ==='
  "$cxx" -std=c++20 -Wall -Wextra -Werror -O2 -pthread -DTOSHI_SKU_LINUX \
    -I src -I vendor/openbarnyard/Toshi/Source \
    tests/test_timer.cpp src/Platform/Linux/THPTimer_Linux.cpp -o "$work/timer"
  "$work/timer"
else
  echo 'SKIP atomic test: run scripts/bootstrap.sh first.'
fi
python3 -B tests/test_launcher.py
bash -n packaging/build-deb.sh scripts/bootstrap.sh scripts/build-platform.sh
if [[ ! -e artifacts/barnyard-native ]]; then
  if bash packaging/build-deb.sh > "$work/package.log" 2>&1; then
    echo 'FAIL: packager accepted a missing engine' >&2
    exit 1
  fi
  grep -q 'RELEASE BLOCKED' "$work/package.log"
  echo 'PASS: release packaging refuses missing native engine.'
fi
echo 'PASS: unit tests only; entire engine/gameplay and GUI display not tested.'

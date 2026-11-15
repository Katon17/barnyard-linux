#!/usr/bin/env bash
# Tests the SDL/OpenGL backend, not the game. Requires an X server or Xvfb.
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p artifacts reports/gl32
"${CXX:-g++}" -m32 -std=c++20 -I src tests/test_native_window32.cpp \
 src/Platform/Linux/NativeWindow.cpp -lSDL2 -lGLEW -lGL -o artifacts/test_native_window32
xvfb-run -a -s '-screen 0 1280x720x24' timeout 30 artifacts/test_native_window32 \
 > reports/gl32/native-window-test.txt 2>&1
cat reports/gl32/native-window-test.txt

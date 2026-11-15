#!/usr/bin/env bash
# Build on the user's own distro, then capture an experimental startup attempt.
# Never installs packages, downloads game data, or copies existing saves/assets.
set -euo pipefail
if [[ ${1:-} == --help || $# != 1 ]]; then
  echo 'Usage: bash scripts/test-local-installation.sh /path/to/PC/Barnyard'
  echo 'Builds from pinned upstream source, then runs for up to 120 seconds.'
  echo 'Uses a NEW isolated user directory; does not import existing saves.'
  echo 'Uploads are manual: inspect the resulting .txt log before sharing.'
  [[ ${1:-} == --help ]] && exit 0 || exit 64
fi
cd "$(dirname "$0")/.."
[[ $EUID != 0 ]] || { echo 'Do not run this script with sudo/root.' >&2; exit 1; }
game=$(realpath -e -- "$1")
[[ -d $game ]] || { echo 'Game root must be a directory.' >&2; exit 1; }
missing=0
for tool in git python3 g++ pkg-config timeout; do
  command -v "$tool" >/dev/null || { echo "Missing tool: $tool"; missing=1; }
done
if (( ! missing )); then
  PKG_CONFIG_LIBDIR=/usr/lib/i386-linux-gnu/pkgconfig:/usr/share/pkgconfig \
    pkg-config --exists sdl2 glew libavformat libavcodec libavutil libswscale libswresample || missing=1
  printf '#include <cstdint>\n#include <SDL2/SDL.h>\n#include <GL/glew.h>\nint main(){return 0;}\n' | \
    g++ -m32 -std=c++20 -x c++ -fsyntax-only - >/dev/null 2>&1 || missing=1
  [[ -f /usr/include/stb/stb_image.h ]] || missing=1
fi
if (( missing )); then
  cat <<'DEPS'
Development dependencies are missing. On Mint/Ubuntu, install from YOUR distro:
  sudo dpkg --add-architecture i386
  sudo apt update
  sudo apt install git python3 pkg-config build-essential g++-multilib libstb-dev \
    libsdl2-dev:i386 libglew-dev:i386 libgl1-mesa-dev:i386 \
    libavcodec-dev:i386 libavformat-dev:i386 libavutil-dev:i386 \
    libswscale-dev:i386 libswresample-dev:i386
Then repeat this script. These instructions are not a claim of Mint certification.
Do not install Debian shared libraries on Mint to run a sandbox-built executable.
DEPS
  exit 69
fi
[[ -n ${DISPLAY:-} || -n ${WAYLAND_DISPLAY:-} ]] || {
  echo 'Run from a terminal in your graphical desktop session.' >&2; exit 1;
}
stamp=$(date +%Y%m%d-%H%M%S)
state=${XDG_STATE_HOME:-$HOME/.local/state}
[[ $state = /* ]] || { echo 'XDG_STATE_HOME must be absolute.' >&2; exit 1; }
work=$(mktemp -d "$state/barnyard-local-test-XXXXXX" 2>/dev/null) || {
  mkdir -p "$state"; work=$(mktemp -d "$state/barnyard-local-test-XXXXXX");
}
chmod 700 "$work"
log="$work/barnyard-uji-lokal-$stamp.txt"
exec > >(tee "$log") 2>&1
echo "Log file: $log"
printf 'Barnyard Linux EXPERIMENTAL local test\nDate: %s\n' "$(date -Is)"
uname -a
[[ ! -f /etc/os-release ]] || cat /etc/os-release
printf '\nCompiler:\n'; g++ --version | head -1
printf '\nBuilding native i386 runtime on this machine; NOT a playable-release guarantee.\n'
if ! python3 scripts/build-full32.py --jobs 2 --link; then
  echo 'BUILD FAILED. Last compiler diagnostics:'
  python3 - <<'PY'
import json
from pathlib import Path
p=Path('reports/full32/results.json')
if p.exists():
    for row in json.loads(p.read_text()):
        if row.get('status')!='PASS':
            print(row.get('source'))
            name=str(row.get('source','')).replace('/','_').removesuffix('.cpp')
            f=Path('reports/full32')/(name+'.log')
            if f.exists(): print('\n'.join(f.read_text(errors='replace').splitlines()[-35:]))
for f in Path('reports/full32').glob('pch-*.log'):
    text=f.read_text(errors='replace')
    if 'error:' in text: print(f, '\n'.join(text.splitlines()[-25:]))
PY
  echo "Share this log after review: $log"
  exit 1
fi
python3 scripts/test-runtime32.py
export BARNYARD_GAME_DIR="$game"
export BARNYARD_USER_DIR="$work/user"
mkdir -m700 "$BARNYARD_USER_DIR"
runtime="$PWD/build/full32/barnyard-native"
sha256sum "$runtime"
echo 'Checking indicators only; a pass does not establish complete data.'
"$runtime" --check-data
printf '\nStarting experimental game for at most 120 seconds.\n'
echo 'If a window opens, observe the result; close normally if possible.'
echo 'Report separately whether you saw a menu and heard sound. Do not overwrite original saves.'
cd "$BARNYARD_USER_DIR"
set +e
timeout -k 5 120 "$runtime"
rc=$?
set -e
printf '\nRuntime exit status: %s\n' "$rc"
[[ $rc != 124 && $rc != 137 ]] || echo 'Time limit reached; this is not gameplay verification.'
echo 'Local run complete, NOT automatic menu/gameplay/save certification.'
echo "Isolated test user data retained: $BARNYARD_USER_DIR"
echo "Review and upload ONLY this text log: $log"
echo 'Do not upload game data or saves. The log may contain local usernames/paths.'
exit "$rc"

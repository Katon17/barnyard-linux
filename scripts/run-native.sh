#!/usr/bin/env bash
# Run a locally built EXPERIMENTAL game executable; requires user's PC data.
set -euo pipefail
project=$(cd "$(dirname "$0")/.." && pwd)
if (( $# < 1 )); then echo 'Usage: bash scripts/run-native.sh /path/to/PC/Barnyard [--check-data or game arguments]' >&2; exit 64; fi
export BARNYARD_GAME_DIR=$(realpath -e -- "$1"); shift
export BARNYARD_USER_DIR="${XDG_DATA_HOME:-$HOME/.local/share}/barnyard-linux"
case "$BARNYARD_USER_DIR" in /*) ;; *) echo 'XDG_DATA_HOME must be absolute' >&2; exit 64;; esac
runtime="$project/build/full32/barnyard-native"
if [[ ! -x $runtime ]]; then echo 'Build first: python3 scripts/build-full32.py --jobs 2 --link' >&2; exit 1; fi
mkdir -p -- "$BARNYARD_USER_DIR"
cd "$BARNYARD_USER_DIR"
exec "$runtime" "$@"

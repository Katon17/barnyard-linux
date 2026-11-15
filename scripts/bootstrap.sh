#!/usr/bin/env bash
# Fetch pinned upstream source, never proprietary game assets. No sudo actions.
set -euo pipefail
cd "$(dirname "$0")/.."
pin=b63d5e320c428627ac0027381cd84d3bd4c1f3a7
repo=vendor/openbarnyard
if [[ ! -d "$repo/.git" ]]; then
  git clone --depth 1 --filter=blob:none --no-checkout https://github.com/InfiniteC0re/OpenBarnyard.git "$repo"
  git -C "$repo" sparse-checkout init --cone
  git -C "$repo" sparse-checkout set Toshi/Source OpenBarnyard/Source PrimeDebug/Source
  git -C "$repo" fetch --depth 1 origin "$pin"
  git -C "$repo" checkout --detach "$pin"
fi
# Only the resource header is needed. Do not materialize binary IDE resources/SDK payloads.
git -C "$repo" sparse-checkout set --no-cone \
 /Dependencies.lua /Settings.lua /Workspace.lua /premake5.lua /License.md /README.md \
 /Toshi/Source/ /OpenBarnyard/Source/ /PrimeDebug/Source/ \
 /Toshi/Vendor/Opcode/Source/ /Toshi/Vendor/fmod/include/ \
 /Toshi/Vendor/bink/include/ /Toshi/Vendor/imgui/ /Toshi/Resources/resource.h
actual=$(git -C "$repo" rev-parse HEAD)
[[ $actual == "$pin" ]] || { echo "STOP: upstream commit differs: $actual" >&2; exit 1; }
# Later patches can intentionally change an earlier patch's context. Validate
# the final reconstructed tree, not whether each old patch reverses in isolation.
if python3 scripts/verify-patches.py >/dev/null 2>&1; then
  echo 'All patches already applied (pristine reconstruction byte-verified).'
else
  # Snapshot-restored sparse indexes can crash Git's threaded preload path.
  # A failed inspection must NEVER be treated as a pristine worktree.
  changes=$(git -C "$repo" -c core.preloadIndex=false diff --name-only HEAD) || {
    echo 'STOP: unable to inspect vendor worktree safely.' >&2
    exit 1
  }
  if [[ -n $changes ]]; then
    echo 'STOP: vendor modifications differ from the complete patch chain. Preserve local work; use a pristine pinned checkout to bootstrap.' >&2
    exit 1
  fi
  for patch in patches/*.patch; do
    git -C "$repo" apply --check "$PWD/$patch"
    git -C "$repo" apply "$PWD/$patch"
    echo "Applied: $patch"
  done
fi
echo 'Pinned source prepared. This is NOT a complete game build.'

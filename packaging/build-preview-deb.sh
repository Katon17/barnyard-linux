#!/usr/bin/env bash
# Explicit development product, NOT a bypass of the game's release gate.
set -euo pipefail
cd "$(dirname "$0")/.."
version='0.1.0~preview1'
stage=$(mktemp -d)
trap 'rm -rf "$stage"' EXIT
app="$stage/usr/lib/barnyard-linux-preview"
doc="$stage/usr/share/doc/barnyard-linux-preview"
mkdir -p "$stage/DEBIAN" "$stage/usr/bin" "$app" "$doc" \
  "$stage/usr/share/applications" "$stage/usr/share/icons/hicolor/scalable/apps"
for name in core.py preview.py gl_probe.py; do
  install -m644 "launcher/$name" "$app/$name"
done
printf '#!/bin/sh\nexec /usr/bin/python3 -B /usr/lib/barnyard-linux-preview/preview.py "$@"\n' > "$stage/usr/bin/barnyard-linux-preview"
printf '#!/bin/sh\nexec /usr/bin/python3 -B /usr/lib/barnyard-linux-preview/gl_probe.py "$@"\n' > "$stage/usr/bin/barnyard-linux-gpu-check"
chmod 755 "$stage/usr/bin/"*
install -m644 packaging/preview.desktop "$stage/usr/share/applications/barnyard-linux-preview.desktop"
install -m644 packaging/preview-icon.svg "$stage/usr/share/icons/hicolor/scalable/apps/barnyard-linux-preview.svg"
install -m644 packaging/PREVIEW-README.txt "$doc/README"
install -m644 packaging/preview-copyright "$doc/copyright"
cat > "$stage/DEBIAN/control" <<EOF
Package: barnyard-linux-preview
Version: $version
Architecture: all
Section: devel
Priority: optional
Maintainer: Barnyard Linux prototype contributors <noreply@example.invalid>
Depends: python3 (>= 3.10), python3-tk, libsdl2-2.0-0, libegl1, libgles2, libgl1-mesa-dri
Installed-Size: $(du -sk "$stage/usr" | cut -f1)
Description: Development launcher and graphics diagnostics; NOT a playable game
 Preview tooling for an unfinished native Barnyard Linux port.
 Select a PC data folder, inspect basic asset indicators, and run a genuine
 SDL2/GLES3 triangle diagnostic. Does not include or run the Barnyard engine.
 The Play button is disabled. No proprietary game data is included.
EOF
(cd "$stage" && find usr -type f -print0 | sort -z | xargs -0 md5sum > DEBIAN/md5sums)
find "$stage" -type d -exec chmod 755 {} +
if command -v desktop-file-validate >/dev/null; then
  desktop-file-validate "$stage/usr/share/applications/barnyard-linux-preview.desktop"
fi
# Portable Python source only: no binary built against sandbox's newer glibc.
if find "$stage/usr" -type f \( -iname '*.iso' -o -iname '*.exe' -o -iname '*.dll' -o -iname '*.trb' -o -iname '*.bik' -o -iname '*.fsb' -o -iname '*.pyc' \) | grep -q .; then
  echo 'Unexpected asset/binary/cache in staging tree' >&2
  exit 1
fi
mkdir -p packages
SOURCE_DATE_EPOCH=1794697200 dpkg-deb -Zxz --root-owner-group --build "$stage" "packages/barnyard-linux-preview_${version}_all.deb"
echo 'BUILT DEVELOPMENT PREVIEW ONLY. This is NOT a ready-to-play Barnyard port.'

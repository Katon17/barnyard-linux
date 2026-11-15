#!/usr/bin/env bash
# Community-release packager. Keeps milestone gates and records user vs automated evidence.
# Run in the Ubuntu 24.04 baseline used to build the supplied runtime.
set -euo pipefail
cd "$(dirname "$0")/.."
runtime=$(realpath -m -- "${1:-artifacts/barnyard-native}")
manifest=$(realpath -m -- "${2:-artifacts/runtime-manifest.json}")
python3 - "$runtime" "$manifest" <<'PY'
from pathlib import Path
import hashlib,json,platform,sys
sys.path.insert(0,'launcher')
import core
binary,manifest=map(Path,sys.argv[1:])
def stop(text):raise SystemExit('RELEASE BLOCKED: '+text)
error=core.native_error(binary)
if error:stop(error)
try:m=json.loads(manifest.read_text())
except (OSError,ValueError):stop('missing runtime validation manifest')
for key in ('gameplay_verified','save_reload_verified','audio_video_verified','xdg_adapter_verified'):
    if m.get(key) is not True:stop('unverified milestone: '+key)
    if not m.get('verification_basis',{}).get(key):stop('missing milestone evidence basis: '+key)
if m.get('launcher_contract')!='barnyard-linux-env-v1':stop('unsupported runtime/launcher contract')
if m.get('baseline')!='Ubuntu 24.04 noble':stop('wrong build baseline')
if m.get('sha256')!=hashlib.sha256(binary.read_bytes()).hexdigest():stop('binary hash mismatch')
if m.get('entrypoint_smoke_verified') is not True:stop('missing baseline entrypoint tests')
build=m.get('build_summary',{})
if not (build.get('clean_full_run') and build.get('linked') and build.get('failed')==0):stop('full source build/link missing')
if m.get('release_channel')!='community' or m.get('version')!='1.0.0':stop('wrong release identity')
evidence=Path(m.get('acceptance_evidence',''))
try:approved=json.loads(evidence.read_text())
except (OSError,ValueError):stop('missing user acceptance evidence')
if approved.get('user_requested_non_test_release') is not True:stop('user release acceptance missing')
os=platform.freedesktop_os_release()
if os.get('ID')!='ubuntu' or os.get('VERSION_ID')!='24.04':stop('package inside matching Ubuntu 24.04 baseline')
PY
stage=$(mktemp -d)
trap 'rm -rf "$stage"' EXIT
root="$stage/root"
mkdir -p "$root/DEBIAN" "$root/usr/bin" "$root/usr/lib/barnyard-linux" \
 "$root/usr/share/applications" "$root/usr/share/barnyard-linux" \
 "$root/usr/share/doc/barnyard-linux" "$stage/debian"
install -m755 "$runtime" "$root/usr/lib/barnyard-linux/barnyard-native"
strip --strip-unneeded "$root/usr/lib/barnyard-linux/barnyard-native"
install -m644 launcher/direct_launch.py launcher/native_test.py "$root/usr/lib/barnyard-linux/"
printf '#!/bin/sh\nexec /usr/bin/python3 /usr/lib/barnyard-linux/direct_launch.py "$@"\n' > "$root/usr/bin/barnyard-linux"
chmod 755 "$root/usr/bin/barnyard-linux"
# Compatibility only: old manually pinned shortcuts/terminal commands keep working.
ln -s barnyard-linux "$root/usr/bin/barnyard-linux-test"
install -m644 packaging/barnyard-linux.desktop "$root/usr/share/applications/"
for size in 16 24 32 48 64 128 256 512; do
 dir="$root/usr/share/icons/hicolor/${size}x${size}/apps"
 mkdir -p "$dir"
 install -m644 "packaging/icons/barnyard-linux-${size}.png" "$dir/barnyard-linux.png"
done
install -m644 packaging/icons/barnyard-linux.bmp "$root/usr/share/barnyard-linux/icon.bmp"
install -m644 "$manifest" "$root/usr/share/doc/barnyard-linux/build-manifest.json"
install -m644 third_party/OpenBarnyard-License.md third_party/GLEW-COPYRIGHT.txt \
 third_party/OPUS-MT-APACHE-2.0.txt third_party/TRANSLATION-NOTICE.txt launcher/LICENSE-MIT \
 packaging/icons/NOTICE.txt "$root/usr/share/doc/barnyard-linux/"
install -m644 packaging/COMMUNITY-README.txt "$root/usr/share/doc/barnyard-linux/README.txt"
cat > "$stage/debian/control" <<'EOF'
Source: barnyard-linux
Section: games
Priority: optional
Maintainer: Katon Bagaskoro <noreply@example.invalid>
Standards-Version: 4.6.2

Package: barnyard-linux
Architecture: amd64
Description: Unofficial community native Barnyard port for Linux
EOF
deps=$(cd "$stage" && dpkg-shlibdeps -O -e root/usr/lib/barnyard-linux/barnyard-native)
deps=${deps#shlibs:Depends=}
deps=$(python3 - "$deps" <<'PY'
import re,sys
print(', '.join(re.sub(r'^([a-z0-9][a-z0-9+.-]*)(?=\s|$)',r'\1:i386',p.strip()) for p in sys.argv[1].split(',')))
PY
)
cat > "$root/DEBIAN/control" <<EOF
Package: barnyard-linux
Version: 1.0.0
Section: games
Priority: optional
Architecture: amd64
Maintainer: Katon Bagaskoro <noreply@example.invalid>
Depends: python3 (>= 3.10), python3-tk, xdg-utils, hicolor-icon-theme, libgl1-mesa-dri:i386, libglx-mesa0:i386, $deps
Conflicts: barnyard-linux-test, barnyard-linux-preview
Replaces: barnyard-linux-test, barnyard-linux-preview
Description: Barnyard Linux - unofficial community native port
 Native i386 runtime built on Ubuntu 24.04 for Linux Mint 22.x amd64.
 Linux port credited to Katon Bagaskoro; original game/upstream credits retained.
 Direct launch, persistent fullscreen/language/FPS options and Indonesian text draft.
 Requires legally owned PC game data; no original game assets, Wine or DLLs bundled.
EOF
# Update shared desktop/icon caches only. Never inspect or modify any user's home.
for script in postinst postrm; do
 cat > "$root/DEBIAN/$script" <<'EOF'
#!/bin/sh
set -e
if command -v gtk-update-icon-cache >/dev/null 2>&1; then
 gtk-update-icon-cache -q -t -f /usr/share/icons/hicolor || true
fi
if command -v update-desktop-database >/dev/null 2>&1; then
 update-desktop-database -q /usr/share/applications || true
fi
exit 0
EOF
 chmod 755 "$root/DEBIAN/$script"
done
sha256sum "$root/usr/lib/barnyard-linux/barnyard-native" | sed "s|$root/||" > "$root/usr/share/doc/barnyard-linux/packaged-runtime.sha256"
(cd "$root" && find usr -type f -print0 | sort -z | xargs -0 md5sum) > "$root/DEBIAN/md5sums"
mkdir -p packages
dpkg-deb --root-owner-group --build "$root" packages/barnyard-linux_1.0.0_amd64.deb

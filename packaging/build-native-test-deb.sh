#!/usr/bin/env bash
# EXPERIMENTAL ONLY. Separate package/name; never changes release validation gate.
# Run INSIDE the Ubuntu 24.04 baseline used to build the supplied runtime.
set -euo pipefail
cd "$(dirname "$0")/.."
if (( $# != 2 )); then
  echo 'Usage: bash packaging/build-native-test-deb.sh runtime noble-test-manifest.json' >&2; exit 64
fi
runtime=$(realpath -e -- "$1")
manifest=$(realpath -e -- "$2")
python3 - "$runtime" "$manifest" <<'PY'
from pathlib import Path
import hashlib,json,platform,sys
binary,manifest=map(Path,sys.argv[1:])
m=json.loads(manifest.read_text())
b=binary.read_bytes()
assert b[:5]==b'\x7fELF\x01' and b[18:20]==b'\x03\x00', 'Expected i386 ELF'
assert hashlib.sha256(b).hexdigest()==m['sha256'], 'Binary hash mismatch'
assert m['baseline']=='Ubuntu 24.04 noble', 'Wrong baseline'
assert m['gameplay_verified'] is False, 'Use release gate for a release'
assert m['entrypoint_smoke_verified'] is True, 'Missing baseline smoke tests'
os=platform.freedesktop_os_release()
assert os['ID']=='ubuntu' and os['VERSION_ID']=='24.04', 'Package inside matching Ubuntu baseline only'
PY
stage=$(mktemp -d)
trap 'rm -rf "$stage"' EXIT
root="$stage/root"
mkdir -p "$root/DEBIAN" "$root/usr/bin" "$root/usr/lib/barnyard-linux-test" \
 "$root/usr/share/applications" "$root/usr/share/doc/barnyard-linux-test" "$stage/debian"
install -m755 "$runtime" "$root/usr/lib/barnyard-linux-test/barnyard-native"
# Strip debug/symbol metadata only; source hash and packaged hash are both recorded.
strip --strip-unneeded "$root/usr/lib/barnyard-linux-test/barnyard-native"
install -m644 launcher/direct_launch.py launcher/native_test.py "$root/usr/lib/barnyard-linux-test/"
printf '#!/bin/sh\nexec /usr/bin/python3 /usr/lib/barnyard-linux-test/direct_launch.py "$@"\n' > "$root/usr/bin/barnyard-linux-test"
chmod 755 "$root/usr/bin/barnyard-linux-test"
install -m644 "$manifest" "$root/usr/share/doc/barnyard-linux-test/build-manifest.json"
install -m644 third_party/OpenBarnyard-License.md third_party/GLEW-COPYRIGHT.txt third_party/OPUS-MT-APACHE-2.0.txt third_party/TRANSLATION-NOTICE.txt launcher/LICENSE-MIT \
 "$root/usr/share/doc/barnyard-linux-test/"
install -m644 packaging/NATIVE-TEST-README.txt "$root/usr/share/doc/barnyard-linux-test/README.txt"
cat > "$root/usr/share/applications/barnyard-linux-test.desktop" <<'EOF'
[Desktop Entry]
Type=Application
Name=Barnyard (Native Linux)
Comment=Experimental native Barnyard test; gameplay not certified
Exec=barnyard-linux-test
Icon=applications-games
Terminal=false
Categories=Game;
StartupNotify=true
Actions=ChooseFolder;Diagnostic;

[Desktop Action ChooseFolder]
Name=Pilih folder game
Exec=barnyard-linux-test --choose-folder

[Desktop Action Diagnostic]
Name=Diagnostik dan log
Exec=barnyard-linux-test --diagnostic
EOF
cat > "$stage/debian/control" <<'EOF'
Source: barnyard-linux-test
Section: games
Priority: optional
Maintainer: Barnyard Linux contributors <noreply@example.invalid>
Standards-Version: 4.6.2

Package: barnyard-linux-test
Architecture: amd64
Description: Experimental native Barnyard testing, not a release
EOF
# dpkg-shlibdeps resolves ABI requirements against Noble, never Debian's libs.
deps=$(cd "$stage" && dpkg-shlibdeps -O -e root/usr/lib/barnyard-linux-test/barnyard-native)
deps=${deps#shlibs:Depends=}
deps=$(python3 - "$deps" <<'PY'
import re,sys
print(', '.join(re.sub(r'^([a-z0-9][a-z0-9+.-]*)(?=\s|$)',r'\1:i386',p.strip()) for p in sys.argv[1].split(',')))
PY
)
cat > "$root/DEBIAN/control" <<EOF
Package: barnyard-linux-test
Version: 0.1.0~test6
Section: games
Priority: optional
Architecture: amd64
Maintainer: Barnyard Linux contributors <noreply@example.invalid>
Depends: python3 (>= 3.10), python3-tk, xdg-utils, libgl1-mesa-dri:i386, libglx-mesa0:i386, $deps
Description: Experimental native Barnyard game test - NOT a playable release
 Native i386 runtime built on Ubuntu 24.04 for testing on Mint 22.x amd64.
 Direct launch, persistent display and Indonesian text draft; owned PC data required.
 No game assets, Windows DLLs, Wine, or emulator included.
 Gameplay, save-reload and X270 compatibility are NOT certified.
EOF
sha256sum "$root/usr/lib/barnyard-linux-test/barnyard-native" | sed "s|$root/||" > "$root/usr/share/doc/barnyard-linux-test/packaged-runtime.sha256"
(cd "$root" && find usr -type f ! -name md5sums -print0 | sort -z | xargs -0 md5sum) > "$root/DEBIAN/md5sums"
mkdir -p packages
dpkg-deb --root-owner-group --build "$root" packages/barnyard-linux-test_0.1.0~test6_amd64.deb

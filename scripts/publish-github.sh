#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Katon Bagaskoro
# This independent publication helper is free software under GNU GPL version 3
# only. It comes WITHOUT ANY WARRANTY. See ../LICENSE-GPL-3.0.txt.
# This license does NOT relicense the game, patches or independent MIT launcher.
set -euo pipefail
export GH_PROMPT_DISABLED=1
export GIT_PAGER=cat
cd "$(dirname "$0")/.."
root=$PWD
owner=Katon17
name=${BARNYARD_REPO_NAME:-barnyard-linux}
[[ $name =~ ^[A-Za-z0-9][A-Za-z0-9._-]*$ ]] || { echo 'Invalid repository name / Nama repo tidak valid.' >&2; exit 1; }
for cmd in gh git python3 sha256sum; do
 command -v "$cmd" >/dev/null || { echo "Missing dependency / Dependensi belum ada: $cmd" >&2; exit 1; }
done
[[ ! -e .git ]] || { echo 'STOP: .git already exists. No overwrite/force-push. See docs/PUBLISH.md for recovery.' >&2; exit 1; }
[[ ${GH_HOST:-github.com} == github.com ]] || { echo 'STOP: use github.com, not another GH_HOST.' >&2; exit 1; }
# No token is accepted on the command line, echoed or written into repository files.
gh auth status --hostname github.com >/dev/null 2>&1 || {
 echo 'Login first / Login dahulu: gh auth login --hostname github.com --git-protocol https --web' >&2; exit 1;
}
login=$(gh api --hostname github.com user --jq .login)
[[ ${login,,} == ${owner,,} ]] || { echo "STOP: logged in as $login, expected $owner. Use gh auth switch --user $owner." >&2; exit 1; }
python3 - <<'PY'
from pathlib import Path
import re
r=Path('.')
for required in ['README.md','README.id.md','LICENSE.md','LICENSE-GPL-3.0.txt','third_party/OpenBarnyard-License.md','docs/images/main-menu.png','docs/images/about.png']:
 if not (r/required).is_file():raise SystemExit('STOP: missing / tidak ada: '+required)
blocked={'.git','vendor','build','artifacts','packages','installers','.venv','__pycache__'}
suffixes={'.deb','.exe','.dll','.iso','.trb','.bik','.fsb','.sav','.save','.pem','.key','.pyc'}
for p in r.rglob('*'):
 if not p.is_file():continue
 if any(x in blocked for x in p.parts):raise SystemExit('STOP: generated/private file / file privat: '+str(p))
 if p.suffix.lower() in suffixes or p.name=='.env' or p.name.startswith('.env.'):
  raise SystemExit('STOP: forbidden publication file / file dilarang: '+str(p))
 if p.stat().st_size>20_000_000:raise SystemExit('STOP: unexpected large file: '+str(p))
 if p.suffix.lower() in {'.png','.bmp'}:continue
 text=p.read_bytes()
 for pattern in [rb'gh[pousr]_[A-Za-z0-9]{30,}', rb'github_pat_[A-Za-z0-9_]{30,}', rb'-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----']:
  if re.search(pattern,text):raise SystemExit('STOP: possible secret / kemungkinan rahasia: '+str(p))
print('PASS: curated source preflight; not a comprehensive secret/copyright audit / bukan audit menyeluruh.')
PY
# Also honor .gitignore: stage an explicit curated source tree, never a parent folder.
echo "Target: https://github.com/$owner/$name"
echo 'Visibility / Visibilitas: PRIVATE. Optional installer release: DRAFT.'
echo 'Upstream non-commercial license and donation questions remain. This is not a blanket GPL project.'
echo 'Lisensi upstream non-komersial tetap berlaku; izin donasi belum jelas. Proyek ini bukan seluruhnya GPL.'
read -r -p 'Type PUSH to create the private repo / Ketik PUSH untuk membuat repo privat: ' answer </dev/tty
[[ $answer == PUSH ]] || { echo 'Cancelled / Dibatalkan.'; exit 0; }
if gh repo view "$owner/$name" --json name >/dev/null 2>&1; then
 echo 'STOP: repository already exists. Nothing overwritten. Choose BARNYARD_REPO_NAME or use docs/PUBLISH.md.' >&2; exit 1
fi
uid=$(gh api --hostname github.com user --jq .id)
git init -b main
git config user.name 'Katon Bagaskoro'
git config user.email "$uid+$owner@users.noreply.github.com"
git add -- .
# Patch context whitespace and original legal notices must remain byte-preserved.
git diff --cached --check -- . ':!patches' ':!third_party' ':!LICENSE-GPL-3.0.txt' ':!launcher/LICENSE-MIT'
count=$(git diff --cached --name-only | wc -l)
echo "Staged source files / File source siap commit: $count"
git commit -m 'Release Barnyard Linux 1.0.0 source, bilingual docs and screenshots'
# No --public, visibility changes, --force, remote deletion or history rewrite.
gh repo create "$owner/$name" --private --description 'Unofficial native Barnyard Linux port; owned PC data required; mixed licenses with upstream non-commercial terms.'
git remote add origin "https://github.com/$owner/$name.git"
git -c credential.helper= -c 'credential.helper=!gh auth git-credential' push -u origin main
# Keep binary outside Git history; upload only as a private DRAFT release asset.
assets="$root/../installers"
deb="$assets/barnyard-linux_1.0.0_amd64.deb"
if [[ -f $deb ]]; then
 (cd "$assets" && sha256sum -c SHA256SUMS)
 gh release create v1.0.0 "$deb" "$assets/SHA256SUMS" \
   --repo "$owner/$name" --target main --draft --title 'Barnyard Linux 1.0.0' \
   --notes-file docs/RELEASE-1.0.0.md
 echo 'Installer uploaded as draft / Installer diunggah sebagai draft.'
else
 echo 'No installer found: source push only / Installer tidak ada: hanya source yang dipush.'
fi
printf '\nDONE / SELESAI: https://github.com/%s/%s\n' "$owner" "$name"
echo 'Private repo + draft release. Review licenses before any public distribution.'
echo 'Repo privat + draft release. Tinjau lisensi sebelum distribusi publik.'

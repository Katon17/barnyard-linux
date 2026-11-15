# GitHub publication / Publikasi GitHub

## English

The supplied helper is the only newly GPL-3.0-only component. It creates
`Katon17/barnyard-linux` as a **private** repository, pushes source on `main`,
and uploads the separate installer as a **draft** release when present. It
never force-pushes, overwrites an existing repository, or makes a repo public.
It has not authenticated or performed any push in the assistant workspace.

The archive contains:

```text
barnyard-linux-github/
  START-HERE.txt
  repository/             # Only this directory is committed
    README.md
    README.id.md
    docs/images/          # Two user-supplied game screenshots
    src/ patches/ ...
    scripts/publish-github.sh
  installers/             # Outside source history
    barnyard-linux_1.0.0_amd64.deb
    SHA256SUMS
```

On Linux Mint, extract into a fresh directory, install `gh`, `git` and `unzip`,
log into GitHub with your own browser, then run the helper. The helper checks the
active account is **Katon17**, prompts for `PUSH`, and checks for common secrets
and accidentally bundled data. This is not a comprehensive security/legal audit.
Do not paste tokens into chat or put them in terminal command arguments.

```bash
sudo apt update
sudo apt install -y git gh unzip python3
# Log in locally. Verify the browser is on github.com before authorizing.
gh auth login --hostname github.com --git-protocol https --web
# From the extracted repository directory:
bash scripts/publish-github.sh
```

A different **new** repository name can be selected before first execution:

```bash
BARNYARD_REPO_NAME=barnyard-linux-community bash scripts/publish-github.sh
```

If you change the name, update the two README release URLs accordingly. The
helper only permits the authenticated Katon17 account; organization publication
is deliberately not automated. It uses a local Git author configuration with
the authenticated user's GitHub noreply address, not a personal email/global
Git configuration. Browser authentication is between you and GitHub.

### Failure/recovery: do not delete or force-push

If the repo already exists, the helper stops. Use a different new name or inspect
the existing repository manually; it will not merge unknown remote contents.
If repo creation succeeded but push failed, inspect `git status`, `git remote -v`
and `gh auth status`. After verifying the remote is your intended new repo:

```bash
git -c credential.helper= -c 'credential.helper=!gh auth git-credential' push -u origin main
```

Do not add `--force` if rejected. Inspect the remote and resolve the reason first.
If source push succeeded but draft upload failed, verify the installer checksum
in the sibling `installers/` directory and create the draft explicitly:

```bash
(cd ../installers && sha256sum -c SHA256SUMS)
gh release create v1.0.0 ../installers/barnyard-linux_1.0.0_amd64.deb \
  ../installers/SHA256SUMS --repo Katon17/barnyard-linux --target main --draft \
  --title 'Barnyard Linux 1.0.0' --notes-file docs/RELEASE-1.0.0.md
```

If a draft with that tag already exists, inspect it in GitHub first rather than
creating a duplicate or replacing assets blindly. A script error does not mean
no remote changes happened; check GitHub to determine how far it completed.

### Before public visibility

Read LICENSE.md and clarify the upstream non-commercial/donation restrictions.
Preserve all original notices. No public-visibility command is supplied as a
one-click workaround: acknowledgements do not create permission. Review both
screenshots and source for any remaining private/copyright-sensitive material.
Do not advertise the game as GPL. Review and publish the draft only after the
rights/licensing review; making a repo public would expose draft source/history
and any subsequently published assets. Private hosting is not legal clearance.

## Bahasa Indonesia

Skrip publikasi adalah satu-satunya komponen baru berlisensi GPL-3.0-only.
Skrip membuat **repo privat** `Katon17/barnyard-linux`, push source ke `main`,
lalu mengunggah installer terpisah sebagai **draft release** jika ada. Tidak
ada force-push, penimpaan repo lama, atau perubahan visibilitas menjadi publik.
Sesi asisten belum login atau melakukan push apa pun.

Susunan arsip terlihat di atas. Hanya `repository/` yang masuk Git; `.deb`
berada di `installers/` dan diunggah sebagai aset draft, bukan riwayat source.
Dua screenshot game ada di `docs/images/`; screenshot profil/browser tidak
ikut, untuk menghindari unggahan informasi pribadi yang tidak diperlukan.

Pasang `git gh unzip python3`, login lewat browser dengan perintah di atas,
kemudian jalankan `bash scripts/publish-github.sh` dari direktori repository.
Skrip memeriksa akun aktif **Katon17**, meminta ketik `PUSH`, dan memeriksa
rahasia umum/data yang tidak sengaja tersalin. Pemeriksaan itu bukan audit
keamanan/hak cipta menyeluruh. Jangan kirim token ke chat atau argumen perintah.
Konfigurasi nama/email Git hanya lokal; email memakai noreply GitHub akunmu.

Jika nama repo sudah ada, skrip berhenti tanpa menimpa. Pilih nama baru lewat
`BARNYARD_REPO_NAME` sebelum eksekusi pertama, dan sesuaikan tautan Releases di
kedua README jika namanya diubah. Publikasi ke organisasi tidak diotomatisasi.

Jika gagal di tengah, **jangan hapus repo atau force-push**. Periksa status Git,
remote, autentikasi, serta GitHub untuk melihat tahap yang sudah selesai. Blok
recovery di atas dapat digunakan setelah memastikan target benar. Jika draft
sudah ada, periksa dulu sebelum mengunggah aset lagi. Error skrip bukan jaminan
belum ada perubahan remote.

Sebelum publikasi umum, baca LICENSE.md dan minta kejelasan batas non-komersial/
donasi upstream. Jangan menyebut seluruh game GPL. Tidak ada perintah instan
untuk membuka visibilitas sebagai jalan pintas; mengetik persetujuan tidak
menciptakan izin. Tinjau screenshot, source, dan draft release. Repo privat pun
bukan bukti semua hak distribusi sudah beres.

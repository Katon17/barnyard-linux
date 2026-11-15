# Developer build / Build pengembang

## English

**Players do not need this. Install the `.deb` from the README.**

Validated build baseline: Ubuntu 24.04 Noble amd64 with GCC/G++ 14 multilib,
SDL2 2.30, OpenGL, FFmpeg development libraries for i386 and static GLEW 2.2.0.
Use a disposable Noble environment; installing multilib packages can change
host development dependencies. This is not a complete automated clean-machine
bootstrap or a claim of byte-for-byte reproducibility on every host.

A representative dependency set on Noble (universe enabled) is:

```bash
sudo dpkg --add-architecture i386
sudo apt update
sudo apt install git python3 python3-tk python3-pil build-essential \
  g++-14-multilib pkg-config dpkg-dev libsdl2-dev:i386 libgl-dev:i386 \
  libglew-dev:i386 libglu1-mesa-dev:i386 libx11-dev:i386 libstb-dev \
  libavformat-dev:i386 libavcodec-dev:i386 libavutil-dev:i386 \
  libswscale-dev:i386 libswresample-dev:i386 \
  xvfb xauth desktop-file-utils hicolor-icon-theme xdg-utils
```

The shipped binary used **static GLEW built from Ubuntu source**. Noble's i386
GLEW development package may not supply the desired static archive. If matching
that setup, enable appropriate `deb-src` sources, retrieve `apt source glew`,
build its static archive for `-m32`, and make it available to the i386 linker.
Review the downloaded source's build instructions and retain GLEW copyright.
Using a dynamic GLEW library may work for development but is not the exact
published dependency set. No prebuilt SDK or game-asset download is needed.

From the repository root:

```bash
bash scripts/bootstrap.sh
python3 scripts/verify-patches.py
CXX=g++-14 python3 scripts/build-full32.py --jobs 3 --link
python3 scripts/test-runtime32.py build/full32/barnyard-native
CXX=g++-14 bash scripts/test.sh
```

The bootstrap fetches the pinned upstream commit with sparse checkout and
applies 22 patches. Do not use a different upstream revision or mix arbitrary
local edits into the vendored tree. A partial build (`--match`/`--failed-only`)
cannot establish the full clean-build/link gate.

Executable: `build/full32/barnyard-native`. Reports: `reports/full32/`.
Runtime startup requires complete legally owned PC data and an independent,
writable user-data directory via `BARNYARD_GAME_DIR` / `BARNYARD_USER_DIR`.
Never point user data at the original game-data directory or run as root.

### Packaging

`packaging/build-deb.sh` requires a matching baseline, the actual built ELF,
a hash-matched validation manifest, milestone evidence, user acceptance and
entrypoint tests. **The public source snapshot intentionally omits private raw
logs and does not ship a ready-made approval manifest to rubber-stamp a rebuild.**
Prepare truthful evidence for your own build; do not copy historical approval
flags, alter hashes or treat a compile pass as new gameplay/save/audio testing.
The existing prebuilt installer is provided separately from Git history.

## Bahasa Indonesia

**Pemain tidak perlu kompilasi. Cukup pasang `.deb` sesuai README.**

Basis build yang digunakan: Ubuntu 24.04 Noble amd64, GCC/G++ 14 multilib,
SDL2 2.30, OpenGL, pustaka pengembangan FFmpeg i386, dan GLEW 2.2.0 statis.
Gunakan lingkungan Noble terpisah; paket multilib dapat mengubah dependensi
pengembangan host. Ini bukan bootstrap mesin bersih otomatis yang lengkap
atau jaminan hasil byte-identik di semua host.

Blok dependensi di atas adalah acuan untuk Noble dengan universe aktif.
Biner terdistribusi memakai **GLEW statis dari source Ubuntu**. Paket dev i386
mungkin tidak menyediakan arsip statis yang dibutuhkan. Untuk menyamai setup,
aktifkan sumber `deb-src` yang sesuai, ambil `apt source glew`, bangun arsip
statis dengan `-m32`, lalu sediakan untuk linker i386. Ikuti instruksi source
dan pertahankan copyright. GLEW dinamis tidak sama dengan set dependensi rilis.

Jalankan blok bootstrap/verifikasi/build/uji di atas dari root repo. Bootstrap
mengambil commit upstream terkunci dan menerapkan 22 patch. Jangan mencampur
revisi lain atau modifikasi vendor sembarangan. Build parsial bukan bukti
build penuh. Executable ada di `build/full32/barnyard-native`, laporan di
`reports/full32/`.

Runtime memerlukan data PC lengkap milik sendiri serta direktori user yang
terpisah melalui `BARNYARD_GAME_DIR` / `BARNYARD_USER_DIR`. Jangan memakai folder
data asli sebagai folder save dan jangan menjalankan game dengan root.

### Packaging

`packaging/build-deb.sh` memerlukan basis yang sesuai, ELF aktual, manifest
ber-hash cocok, bukti milestone, persetujuan pengguna, serta uji entrypoint.
**Snapshot source sengaja tidak membundel log privat/manifest siap-centang untuk
mengesahkan build baru.** Siapkan bukti jujur untuk build sendiri; jangan
menyalin flag persetujuan historis atau menganggap kompilasi sebagai pengujian
gameplay/save/audio. Installer yang sudah ada disediakan di luar riwayat Git.

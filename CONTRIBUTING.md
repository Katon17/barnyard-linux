# Contributing / Kontribusi

## English

1. Read [LICENSE.md](LICENSE.md); preserve component licenses and original credits.
2. Open a focused issue before a large change. State distro, desktop, X11/Wayland,
   GPU/driver, package version, expected behavior and reproduction steps.
3. Never submit game assets, retail binaries, private saves, credentials, or raw
   unredacted personal logs. Small relevant redacted excerpts are preferable.
4. Keep changes scoped. For upstream source, add an ordered patch against the
   pinned revision. Verify with `python3 scripts/verify-patches.py` after bootstrap.
5. Run `bash scripts/test.sh`; run relevant native component tests for native
   changes. A headless component pass is not a full-game pass.
6. State test limitations honestly. Discuss licensing before introducing code
   or dependencies; do not assume a GPL file can relicense upstream code.

No contributor agreement transfers third-party rights. Be respectful and do not
pressure people to share copyrighted data or private information.

## Bahasa Indonesia

1. Baca [LICENSE.md](LICENSE.md); pertahankan lisensi komponen dan kredit asli.
2. Buat issue terarah sebelum perubahan besar. Cantumkan distro, desktop,
   X11/Wayland, GPU/driver, versi paket, harapan, dan langkah reproduksi.
3. Jangan kirim aset game, biner retail, save pribadi, kredensial, atau log mentah
   dengan informasi pribadi. Kirim potongan relevan yang sudah disamarkan.
4. Batasi lingkup perubahan. Untuk source upstream, tambahkan patch berurutan
   terhadap revisi terkunci. Verifikasi dengan `python3 scripts/verify-patches.py`
   setelah bootstrap.
5. Jalankan `bash scripts/test.sh` dan uji komponen native yang terkait. Lulus uji
   headless bukan berarti seluruh game sudah teruji.
6. Jelaskan batas uji dengan jujur. Bahas lisensi sebelum menambah kode/dependensi;
   berkas GPL tidak otomatis mengubah lisensi upstream.

Tidak ada perjanjian kontribusi yang memindahkan hak pihak lain. Bersikap sopan
serta jangan mendesak orang membagikan data berhak cipta atau informasi pribadi.

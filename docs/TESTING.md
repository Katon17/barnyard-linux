# Testing and limitations / Pengujian dan batasan

## English

The 1.0.0 clean build compiled 1,103/1,103 units with no failures and linked.
The 22-patch chain reconstructed 122 modified upstream files byte-for-byte.
Real SDL/X11 checks validated the title, WM_CLASS and 256px icon including alpha.
APT replaced test6 with the renamed package; three disposable user files were
hash-preserved. Installed nonroot launchers were exercised with incomplete data,
which correctly exited 3. That is rejection testing, not complete-game startup.

The physical user reported smooth gameplay around 60 FPS and supplied the menu
and About screenshots. FPS is user-reported, not an automated benchmark. No
independent end-to-end certification of every mission, save/reload, cutscene,
audio device, Wayland compositor or final panel icon cache has been performed.
The package rename preserves user paths; fixture preservation is not actual
save serialization/deserialization verification. Translation remains a draft.

Run host checks with `bash scripts/test.sh`. Native rendering/FPS checks require
the compiler, upstream checkout and test environment documented in BUILD.md.
Do not rewrite report booleans or runtime hashes just to make packaging pass.

## Bahasa Indonesia

Build bersih 1.0.0 mengompilasi 1.103/1.103 unit tanpa kegagalan dan berhasil link.
Seri 22 patch merekonstruksi 122 file upstream termodifikasi byte-identik.
Uji SDL/X11 nyata memeriksa judul, WM_CLASS, serta ikon 256px beserta alfa.
APT mengganti test6 dengan paket baru; hash tiga file pengguna uji tetap sama.
Launcher nonroot terpasang diuji dengan data tidak lengkap, yang benar keluar 3.
Itu uji penolakan data, bukan startup dengan data game lengkap.

Pengguna melaporkan gameplay lancar sekitar 60 FPS serta mengirim screenshot
menu dan About. FPS itu laporan pengguna, bukan benchmark otomatis. Belum ada
sertifikasi independen seluruh misi, save/reload, cutscene, perangkat audio,
Wayland, atau cache ikon panel akhir. File fixture migrasi bukan verifikasi
serialisasi/deserialisasi save game nyata. Terjemahan tetap draf.

Jalankan uji host dengan `bash scripts/test.sh`. Uji grafik/FPS native memerlukan
compiler, checkout upstream, dan lingkungan di BUILD.md. Jangan memalsukan
boolean laporan atau hash runtime hanya agar packaging lolos.

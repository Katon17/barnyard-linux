BARNYARD LINUX 1.0.0 — RILIS KOMUNITAS TIDAK RESMI
================================================
Port native Linux oleh Katon Bagaskoro. Nama aplikasi: Barnyard Linux.
Paket: barnyard-linux. Perintah: barnyard-linux.
Basis Ubuntu 24.04, untuk Linux Mint 22.x amd64; runtime game native i386.
Tidak memakai Wine/emulator/DLL Windows. Memerlukan data game PC milik Anda.

INSTALASI / UPGRADE
------------------
Tutup game terlebih dahulu. Buka .deb dengan penginstal paket, atau:

sudo apt install "$HOME/Downloads/barnyard-linux_1.0.0_amd64.deb"

APT akan mengganti paket barnyard-linux-test/preview lama secara otomatis.
Tidak perlu menghapus save, mengekstrak data lagi atau mengompilasi source.
Untuk pemasangan pertama jika i386 belum aktif:
sudo dpkg --add-architecture i386
sudo apt update

Buka Barnyard Linux dari menu aplikasi. Lokasi data game yang sudah diingat
akan tetap digunakan. Pengguna baru memilih folder yang berisi Data sekali.
Jika ikon lama masih tersimpan pada pin panel Xfce, lepaskan pin lama dan pin
Barnyard Linux yang baru dari menu aplikasi; paket tidak mengedit panel pribadi.
Logo pada menu awal di dalam game tetap asli, sesuai pilihan pengguna.

PERINTAH OPSIONAL
-----------------
barnyard-linux --choose-folder  : pilih ulang folder data
barnyard-linux --diagnostic     : buka jendela diagnostik/log
barnyard-linux --version        : tampilkan versi
barnyard-linux --runtime-help   : bantuan executable native
Perintah lama barnyard-linux-test dipertahankan sebagai alias kompatibilitas,
bukan aplikasi kedua. Hanya satu entri aplikasi Barnyard Linux dipasang.

SAVE DAN PENGATURAN TETAP
------------------------
Agar progres tidak hilang, namespace internal lama SENGAJA dipertahankan:
${XDG_DATA_HOME:-$HOME/.local/share}/barnyard-linux-test
Lokasi game tetap diingat di:
${XDG_CONFIG_HOME:-$HOME/.config}/barnyard-linux-test/launcher.json
Nama folder internal ini bukan label versi aplikasi. Jangan memindahkan atau
mengganti namanya sendiri. Tidak ada impor/pemindahan/penghapusan save oleh paket.
Backup save tetap disarankan. Uninstall juga tidak menghapus save/config/log.
Jangan menjalankan game dengan sudo/root.
Log baru: ~/Downloads/barnyard-....txt, izin 0600. Tinjau sebelum dibagikan:
log dapat memuat username/path lokal. Jangan mengunggah aset game atau save.

FITUR
-----
- Langsung main dari ikon; pemilih folder sekali saja.
- Options/Pengaturan: fullscreen/jendela, English/Indonesia, FPS Aktif/Nonaktif.
- Preferensi tersimpan otomatis. Toggle FPS langsung berlaku, default nonaktif.
- FPS kecil di kiri atas, rata-rata sekitar setengah detik. Ini indikator,
  bukan pembatas FPS, pengubah VSync atau benchmark GPU.
- Alt+Enter: fullscreen/jendela. Shift+F1: lepas/kunci mouse.
- Alt+Tab tetap berfungsi; kehilangan fokus melepas capture.
- Tentang/About di menu awal dan Pengaturan: kredit Katon Bagaskoro,
  pembuat game asli/OpenBarnyard/Toshi, tombol Saweria dan Ko-fi.
- Ikon aplikasi baru bertuliskan Barnyard dan Linux dengan motif penguin;
  ikon menu/taskbar/dialog memakai identitas yang sama, bukan aset retail.

CAKUPAN RILIS DAN LISENSI
------------------------
Versi 1.0.0 adalah rilis komunitas yang disetujui pengguna setelah melaporkan
permainan berjalan lancar di Mint 22.3/Xfce/X11/Intel HD 620. Pengguna melaporkan
sekitar 60 FPS dan menjelaskan penurunan saat screenshot. Itu laporan pengguna,
bukan hasil benchmark otomatis atau sertifikasi independen semua perangkat/misi.
Uji komponen, build penuh, pemasangan dan kompatibilitas data dicatat terpisah.
Tidak ada jaminan bebas bug pada seluruh distro/perangkat.

Terjemahan Indonesia tetap draf mesin dengan perbaikan istilah, bukan lokalisasi
resmi atau pemeriksaan manusia penuh. Nama/merek/judul lagu bisa tetap asli;
teks gambar dan literal di luar katalog belum terverifikasi menyeluruh. Sebagian
HUD yang sudah di-cache bisa membutuhkan tutup-buka game setelah ganti bahasa.
Suara asli dan aset English UK tidak diubah. Tidak perlu membuat save baru.

Game/engine asli: Blue Tongue Entertainment / THQ; Toshi kini dimiliki THQ Nordic.
Dekompilasi OpenBarnyard/Toshi: AdventureT dan InfiniteC0re.
Engine/port mengikuti lisensi upstream non-komersial (OpenBarnyard-License.md).
Launcher independen MIT (LICENSE-MIT). GLEW ditautkan statis dari source Ubuntu;
lihat GLEW-COPYRIGHT.txt. Terjemahan: TRANSLATION-NOTICE.txt / Apache-2.0 model.
Ikon fan-made dibantu AI, bukan logo resmi; lihat NOTICE.txt.

Donasi tetap opsional, membuka browser hanya setelah diklik. Larangan penggunaan
komersial upstream (any sort of profit) tetap berlaku. Pencantuman donasi tidak
membuktikan izin monetisasi; minta kejelasan/izin pemilik lisensi sebelum
pendistribusian publik dengan tautan donasi. Bukan produk resmi/berafiliasi.
Tidak memuat aset game asli, model terjemahan, Wine, emulator atau DLL Windows.

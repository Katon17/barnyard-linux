BARNYARD NATIVE LINUX — TEST6 (EKSPERIMEN)
========================================
Paket upgrade native i386 berbasis Ubuntu 24.04 untuk Mint 22.x amd64.
Tidak memakai Wine/emulator/DLL Windows. Memerlukan data game PC milik Anda.
Pengguna melaporkan test5 sangat playable dan lancar pada Mint. Tambahan FPS
test6 belum diuji di instalasi penuh pengguna; bukan sertifikasi menyeluruh.

INSTALASI / UPGRADE
------------------
Jika test3 sudah terpasang, buka file .deb baru dengan penginstal paket, atau:

sudo apt install "$HOME/Downloads/barnyard-linux-test_0.1.0~test6_amd64.deb"

Hanya untuk pemasangan pertama yang belum mengaktifkan i386:
sudo dpkg --add-architecture i386
sudo apt update

Gunakan repository Mint/Ubuntu sistem Anda, jangan --force-depends atau library
Debian. Paket tidak mengganti repository dan tidak mengunduh data game.
Jangan menjalankan game dengan sudo/root.

LANGSUNG DARI IKON
-----------------
1. Buka "Barnyard (Native Linux)" dari menu aplikasi.
2. Sekali saja pilih folder Barnyard yang BERISI folder Data, bukan ISO/installer.
3. Game langsung dimulai. Pembukaan selanjutnya memakai folder yang diingat,
   tanpa jendela launcher atau tombol Mulai.

Ganti lokasi data: klik kanan ikon/menu Barnyard > Pilih folder game, atau:
barnyard-linux-test --choose-folder
UI diagnostik lama tetap tersedia: barnyard-linux-test --diagnostic
Folder diingat di ${XDG_CONFIG_HOME:-$HOME/.config}/barnyard-linux-test/launcher.json.
Jika folder sudah pindah/hilang, pemilih folder ditampilkan lagi.

PENGATURAN DI DALAM GAME
-----------------------
Buka Options/Pengaturan dari menu utama atau menu jeda:
- Display/Tampilan: klik untuk berganti Fullscreen/Layar penuh atau Windowed/Jendela.
- Language/Bahasa: klik untuk berganti English atau Indonesia.
Pilihan langsung disimpan untuk pembukaan berikutnya di folder save yang sama.
Default awal: fullscreen, teks Inggris. Bahasa Indonesia hanya mengganti teks,
bukan pengisi suara; jalur aset dan suara English UK tetap dipakai.

Terjemahan mencakup seluruh katalog teks Inggris UK dan tiga pustaka subtitle
yang tersedia: menu, HUD, petunjuk, misi dan dialog/subtitle. Ini DRAF TERJEMAHAN
MESIN dengan perbaikan istilah menu/kata umum, belum pemeriksaan manusia penuh.
Nama, merek dan judul lagu dapat tetap asli. Teks yang menjadi bagian gambar,
literal di luar katalog dan tata letak panjang belum terverifikasi menyeluruh.
Menu pengaturan diperbarui langsung. Sebagian HUD/petunjuk yang sudah tersimpan
di memori bisa masih memakai bahasa sebelumnya: tutup-buka game setelah memilih
bahasa untuk menerapkan secara konsisten. Tidak perlu memulai save baru.

Alt+Enter juga mengubah dan menyimpan pilihan fullscreen/jendela.
Shift+F1 melepas/mengunci mouse; klik game untuk mengunci kembali.
Alt+Tab tetap berfungsi; kehilangan fokus melepas capture dan menampilkan kursor.
Fullscreen memakai resolusi desktop tanpa mengubah refresh monitor.

SAVE DAN LOG
------------
Save test3 TETAP di ${XDG_DATA_HOME:-$HOME/.local/share}/barnyard-linux-test.
Upgrade tidak mengimpor, memindahkan atau menghapus save. Backup tetap disarankan.
Pengaturan native tersimpan di Settings/native-preferences.bin (32 byte,
checksum, tulis atomik). Jangan mengedit berkas biner itu secara manual.
Log otomatis: ~/Downloads/barnyard-uji-....txt. Setelah keluar normal tidak ada
jendela konfirmasi. Jika gagal, dialog menunjukkan lokasi log.
Tinjau log sebelum diunggah: dapat memuat username/path lokal. Jangan kirim save
atau aset game. Uninstall paket tidak menghapus save/config/log pengguna.

LISENSI DAN BATAS PENGUJIAN
-------------------------
Engine/port: lisensi upstream non-komersial OpenBarnyard/Toshi (lihat
OpenBarnyard-License.md). Launcher independen: MIT (LICENSE-MIT).
GLEW 2.2.0 ditautkan statis dari source Ubuntu Noble, lihat GLEW-COPYRIGHT.txt.
Dependensi SDL/FFmpeg/Mesa/C/C++ berasal dari repository distro.
Terjemahan lokal OPUS-MT: lihat TRANSLATION-NOTICE.txt dan OPUS-MT-APACHE-2.0.txt.
Paket tidak memuat katalog/aset game asli, model terjemahan atau audio tambahan.

Uji komponen, penyimpanan preferensi dan entrypoint bukan bukti gameplay penuh.
Pergantian bahasa/tata letak di instalasi penuh, save-reload, seluruh misi/
minigame, speaker fisik dan sesi panjang masih perlu pengujian pengguna.
Gate sertifikasi paket final tetap tidak diubah oleh paket eksperimental ini.

TAMBAHAN TEST5: TENTANG / ABOUT
-----------------------------
Tombol Tentang/About tersedia di menu awal dan Options/Pengaturan (termasuk
pengaturan dari menu jeda). Isinya:
- Port native Linux oleh Katon Bagaskoro.
- Kredit game asli dan OpenBarnyard/Toshi tetap ditampilkan.
- Saweria: https://saweria.co/Kabako
- Ko-fi: https://ko-fi.com/katonbagaskoro
Tombol donasi hanya membuka browser sistem setelah diklik; tidak ada pembayaran
atau koneksi otomatis saat membuka game/About. Kembali/Back atau Esc menutup
panel. Bila browser gagal dibuka, alamat tetap terlihat untuk dibuka manual.

Lisensi upstream melarang penggunaan komersial (any sort of profit). Pencantuman
tautan donasi bukan bukti izin monetisasi. Sebelum distribusi publik dengan
fitur donasi, minta kejelasan/izin dari pemilik lisensi; jangan klaim sebagai game
resmi atau menjual aset game asli. Lihat OpenBarnyard-License.md.

TAMBAHAN TEST6: TAMPILKAN FPS
---------------------------
Buka Options/Pengaturan dari menu awal atau menu jeda, lalu klik FPS:
- FPS: Aktif / FPS: On menampilkan penghitung kecil di pojok kiri atas.
- FPS: Nonaktif / FPS: Off menyembunyikannya. Default: nonaktif.
Perubahan langsung berlaku dan disimpan untuk pembukaan game berikutnya,
tanpa mengubah bahasa, fullscreen, save atau kredit/donasi/About.

Angka adalah rata-rata frame GUI yang benar-benar dirender selama sekitar
setengah detik, dihitung memakai jam monoton SDL. Saat awal diaktifkan atau
setelah jeda panjang akan muncul FPS: -- sebentar sebelum angka tersedia.
Fitur ini HANYA menampilkan FPS: bukan pembatas FPS, tidak mematikan VSync,
tidak mempercepat gameplay dan bukan pengukuran GPU terpisah. Pada bagian yang
tidak merender GUI, penghitung tidak terlihat. Tidak perlu restart untuk toggle.

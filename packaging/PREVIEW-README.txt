BARNYARD LINUX PREVIEW 0.1
=========================

PERINGATAN: PAKET INI BELUM DAPAT MEMAINKAN BARNYARD.
Ini installer launcher/diagnostik pengembangan, bukan port game selesai.
Tidak menyertakan engine game, aset, ISO, serial key, Wine, atau emulator.

Fungsi yang tersedia:
- Pilih folder instalasi PC dan periksa dua indikator aset.
- Simpan pilihan folder di XDG config pengguna.
- Uji jendela native SDL2 / konteks GLES3 / shader / present dengan segitiga.
- Ekspor laporan JSON.
- Menu desktop dan uninstall melalui apt.

Tombol Main sengaja nonaktif. Menginstal data game saja TIDAK mengaktifkannya:
engine Linux masih memerlukan implementasi dan integrasi lanjutan.

Instal pada Mint 22.x/Ubuntu 24.04:
  sudo apt install ./barnyard-linux-preview_0.1.0~preview1_all.deb

Buka lewat menu "Barnyard Linux Preview", atau:
  barnyard-linux-preview

Pemeriksaan terminal:
  barnyard-linux-preview --version
  barnyard-linux-preview --status
  barnyard-linux-preview --check '/folder/instalasi/Barnyard'
  barnyard-linux-gpu-check --seconds 10

Uji GPU menggambar segitiga, BUKAN adegan Barnyard. Waktu frame yang dilaporkan
BUKAN benchmark game. Renderer llvmpipe berarti software rendering, bukan Intel
HD 620. Data game asli tidak diubah. ZIP/ISO bukan folder instalasi terpasang.

Log grafik: ~/.local/state/barnyard-linux/graphics-probe.log
Config: ~/.config/barnyard-linux/launcher.json
Path mengikuti variabel XDG bila disetel dengan path absolut.

Uninstall:
  sudo apt remove barnyard-linux-preview

Konfigurasi pengguna dipertahankan; hapus manual hanya jika memang diinginkan.
Ini port tidak resmi, tidak berafiliasi dengan pemegang hak Barnyard.

Batas pengujian: paket dibangun/diuji di sandbox Debian amd64 + Xvfb/Mesa
software. Belum dites langsung di Mint 22.3/X270 atau dengan gameplay Barnyard.

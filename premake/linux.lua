-- CATATAN INTEGRASI SAJA — BUKAN BUILD ENGINE YANG SUDAH BERFUNGSI.
-- Tidak di-include otomatis. Lengkapi source list per proyek dan dispatch header.
-- Jangan menganggap filter premake sebagai if Lua: `error()` di bawah filter
-- tetap dieksekusi; validasi opsi harus memakai if biasa di workspace utama.
-- Path harus dihitung relatif file ini/repo, bukan wks.location yang bisa berubah.

filter { "system:linux" }
    architecture "x86_64"
    cppdialect "C++20"
    defines { "TOSHI_SKU_LINUX" }
    buildoptions { "-fno-strict-aliasing", "-pthread" }
    linkoptions { "-pthread" }
filter {}

-- Clang-specific MS extensions perlu filter toolset dan uji kompatibilitas.
-- -fshort-wchar hanya boleh diaktifkan setelah semua libc wide calls diaudit.
-- SDL2 include/link flags sebaiknya ditarik dari pkg-config pada lingkungan build.
-- Toshi/OpenBarnyard perlu file set Linux masing-masing; jangan mengaplikasikan
-- seluruh source platform ke semua proyek karena duplicate definitions.
-- Kandidat dependencies renderer: SDL2, GLESv2, EGL; platform: pthread, dl, m.

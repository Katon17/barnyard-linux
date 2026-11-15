#!/usr/bin/env python3
"""Installable development preview, explicitly NOT a playable game port."""
from __future__ import annotations
import argparse
import json
import os
from pathlib import Path
import platform
import subprocess
import sys
import core

VERSION = "0.1.0~preview1"


def status_report(folder=""):
    return {
        "package": "barnyard-linux-preview", "version": VERSION,
        "playable": False, "engine_included": False,
        "message": "Preview launcher/diagnostik; engine game Linux belum selesai.",
        "machine": platform.machine(), "kernel": platform.release(),
        "session_type": os.environ.get("XDG_SESSION_TYPE", "unknown"),
        "has_display": bool(os.environ.get("DISPLAY") or os.environ.get("WAYLAND_DISPLAY")),
        "assets": core.inspect_folder(Path(folder)) if folder else {"candidate": False, "issues": ["Belum memilih folder."]},
        "assets_note": "Dua file indikator bukan verifikasi kelengkapan/revisi game.",
    }


class PreviewWindow:
    def __init__(self, root):
        import tkinter as tk
        from tkinter import ttk
        self.root = root
        self.process = None
        self.stream = None
        self.folder = tk.StringVar(value=core.read_folder())
        self.message = tk.StringVar(value="Pilih folder game PC yang sudah diinstal. ZIP/ISO belum bisa dipakai langsung.")
        root.title("Barnyard Linux — Preview (belum bisa main)")
        root.geometry("750x520")
        root.minsize(650, 480)
        panel = ttk.Frame(root, padding=24)
        panel.pack(fill="both", expand=True)
        ttk.Label(panel, text="BARNYARD / LINUX", font=("Sans", 22, "bold")).pack(anchor="w")
        ttk.Label(panel, text="PREVIEW 0.1 • Launcher & diagnostik platform", font=("Sans", 11)).pack(anchor="w", pady=(4, 12))
        notice = tk.Label(panel, text="BELUM BISA MEMAINKAN GAME — ENGINE NATIVE BELUM SELESAI",
                          background="#fff0c5", foreground="#684600", anchor="w", padx=10, pady=10,
                          wraplength=650, justify="left")
        notice.pack(fill="x", pady=(0, 18))
        ttk.Label(panel, text="Folder instalasi Barnyard PC milikmu").pack(anchor="w")
        ttk.Entry(panel, textvariable=self.folder).pack(fill="x", pady=(6, 8))
        actions = ttk.Frame(panel)
        actions.pack(fill="x")
        ttk.Button(actions, text="Pilih folder…", command=self.choose).pack(side="left")
        ttk.Button(actions, text="Periksa data", command=self.check).pack(side="left", padx=8)
        self.play = ttk.Button(actions, text="Main — belum tersedia", state="disabled")
        self.play.pack(side="right")
        ttk.Label(panel, textvariable=self.message, wraplength=680, justify="left").pack(anchor="w", pady=(16, 10))
        diagnostics = ttk.Frame(panel)
        diagnostics.pack(fill="x", pady=(6, 0))
        self.probe = ttk.Button(diagnostics, text="Uji grafik native (bukan game)", command=self.start_probe)
        self.probe.pack(side="left")
        ttk.Button(diagnostics, text="Ekspor laporan…", command=self.export).pack(side="left", padx=8)
        ttk.Label(panel, text="Uji grafik menggambar segitiga melalui SDL2 + GLES3 selama 10 detik.\n"
                              "Ini bukan renderer Barnyard dan bukan benchmark performa game.",
                  wraplength=680, justify="left").pack(anchor="w", pady=(10, 0))
        ttk.Label(panel, text="Tanpa unduhan aset, Wine, emulator, atau perubahan pada data game asli.",
                  wraplength=680).pack(side="bottom", anchor="w", pady=(16, 0))
        root.protocol("WM_DELETE_WINDOW", self.close)
        if self.folder.get():
            self.check()

    def choose(self):
        from tkinter import filedialog
        folder = filedialog.askdirectory(title="Folder instalasi Barnyard PC — bukan ZIP/ISO")
        if folder:
            self.folder.set(folder)
            self.check()

    def check(self):
        folder = self.folder.get().strip()
        if not folder:
            self.message.set("Belum memilih folder instalasi.")
            return
        report = core.inspect_folder(Path(folder))
        if report["candidate"]:
            try:
                core.save_folder(Path(folder))
                self.message.set("Dua indikator data PC ditemukan; folder disimpan.\n"
                                 "Kelengkapan dan revisi aset belum diverifikasi. Engine native tetap belum tersedia.")
            except OSError as exc:
                self.message.set("Folder terdeteksi tetapi konfigurasi gagal disimpan: " + str(exc))
        else:
            self.message.set("Belum cocok sebagai folder data PC:\n" + "\n".join(report["issues"]))
        # Deliberately never enable Play: this preview has no game implementation.
        self.play.configure(state="disabled")

    def start_probe(self):
        if self.process is not None:
            return
        try:
            state = core.xdg_dir("XDG_STATE_HOME", ".local/state")
            state.mkdir(parents=True, exist_ok=True)
            self.log = state / "graphics-probe.log"
            self.stream = self.log.open("wb")
            self.process = subprocess.Popen([sys.executable, "-B", str(Path(__file__).with_name("gl_probe.py")), "--seconds", "10"],
                                            stdout=self.stream, stderr=subprocess.STDOUT)
            self.probe.configure(state="disabled")
            self.message.set("Uji grafik berjalan 10 detik. ESC menutup jendela uji. Ini bukan gameplay.")
            self.root.after(200, self.poll_probe)
        except OSError as exc:
            if self.stream:
                self.stream.close()
                self.stream = None
            self.message.set("Gagal memulai tes: " + str(exc))

    def poll_probe(self):
        if self.process is None:
            return
        code = self.process.poll()
        if code is None:
            self.root.after(200, self.poll_probe)
            return
        self.process = None
        if self.stream:
            self.stream.close()
            self.stream = None
        self.probe.configure(state="normal")
        try:
            report = json.loads(self.log.read_text())
            if code == 0 and report.get("ok"):
                suffix = " • SOFTWARE RENDERER, bukan pengujian Intel HD 620" if report.get("software_renderer") else ""
                self.message.set("Tes GLES3 berhasil: " + report.get("renderer", "unknown") + suffix + "\nLog: " + str(self.log))
            else:
                self.message.set("Tes grafik gagal: " + report.get("error", "lihat log") + "\nLog: " + str(self.log))
        except (OSError, ValueError):
            self.message.set(f"Tes grafik keluar dengan kode {code}. Log: {self.log}")

    def export(self):
        from tkinter import filedialog, messagebox
        filename = filedialog.asksaveasfilename(title="Ekspor laporan diagnostik (berisi path folder pilihanmu)",
                                              defaultextension=".json", initialfile="barnyard-diagnostic.json",
                                              filetypes=[("JSON", "*.json")])
        if filename:
            try:
                report = status_report(self.folder.get().strip())
                report["selected_game_path"] = self.folder.get().strip()
                Path(filename).write_text(json.dumps(report, indent=2, ensure_ascii=False), encoding="utf8")
                self.message.set("Laporan disimpan. Periksa/hapus path pribadi sebelum membagikannya.")
            except OSError as exc:
                messagebox.showerror("Gagal menyimpan", str(exc))

    def close(self):
        if self.process is not None:
            self.process.terminate()
            try:
                self.process.wait(timeout=2)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait()
        if self.stream:
            self.stream.close()
        self.root.destroy()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--version", action="store_true")
    parser.add_argument("--status", action="store_true", help="Laporan JSON tanpa GUI")
    parser.add_argument("--check", type=Path, help="Periksa dua indikator folder data PC (bukan uji gameplay)")
    args = parser.parse_args()
    if args.version:
        print("barnyard-linux-preview " + VERSION + " (NOT PLAYABLE)")
        return 0
    if args.status or args.check is not None:
        report = status_report(str(args.check) if args.check is not None else "")
        print(json.dumps(report, indent=2, ensure_ascii=False))
        return 0 if args.check is None or report["assets"]["candidate"] else 2
    try:
        import tkinter as tk
        root = tk.Tk()
    except (ImportError, RuntimeError) as exc:
        print("GUI tidak tersedia: " + str(exc), file=sys.stderr)
        return 2
    except Exception as exc:
        # Tk reports display failures via TclError, which is not RuntimeError.
        print("Tidak dapat membuka GUI: " + str(exc), file=sys.stderr)
        return 2
    PreviewWindow(root)
    root.mainloop()
    return 0

if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
"""Development launcher. Does not contain or emulate the Barnyard game engine."""
import argparse
import json
from pathlib import Path
import subprocess
import sys
import core


def main():
    parser = argparse.ArgumentParser(description="Barnyard Linux launcher — development, not playable yet")
    parser.add_argument("--check", type=Path, help="Periksa kandidat folder data tanpa membuka GUI")
    args = parser.parse_args()
    if args.check is not None:
        report = core.inspect_folder(args.check)
        report["runtime_error"] = core.native_error(core.RUNTIME)
        print(json.dumps(report, indent=2, ensure_ascii=False))
        return 0 if report["candidate"] else 2
    try:
        import tkinter as tk
        from tkinter import filedialog, messagebox, ttk
    except ImportError:
        print("GUI membutuhkan python3-tk. CLI --check tetap dapat digunakan.", file=sys.stderr)
        return 2
    try:
        root = tk.Tk()
    except tk.TclError as exc:
        print(f"Tidak dapat membuka display: {exc}", file=sys.stderr)
        return 2
    root.title("Barnyard Linux — Development")
    root.geometry("660x410")
    root.minsize(580, 360)
    panel = ttk.Frame(root, padding=22)
    panel.pack(fill="both", expand=True)
    ttk.Label(panel, text="Barnyard Linux", font=("Sans", 20, "bold")).pack(anchor="w")
    ttk.Label(panel, text="Launcher pengembangan • engine native belum selesai").pack(anchor="w", pady=(3, 16))
    folder = tk.StringVar(value=core.read_folder())
    ttk.Label(panel, text="Folder instalasi Barnyard PC milikmu:").pack(anchor="w")
    ttk.Entry(panel, textvariable=folder).pack(fill="x", pady=6)
    buttons = ttk.Frame(panel)
    buttons.pack(fill="x")
    status = tk.StringVar()
    child = None

    def check():
        if not folder.get().strip():
            status.set("Pilih folder instalasi. ZIP/ISO disc belum merupakan data game terpasang.")
            play.configure(state="disabled")
            return
        report = core.inspect_folder(Path(folder.get()))
        error = core.native_error(core.RUNTIME)
        lines = (["Kandidat folder PC ditemukan (belum verifikasi lengkap)."] if report["candidate"] else report["issues"])
        if error:
            lines.append(error)
        status.set("\n".join(lines))
        play.configure(state="normal" if report["candidate"] and not error and child is None else "disabled")
        if report["candidate"]:
            try:
                core.save_folder(Path(folder.get()))
            except OSError as exc:
                status.set(status.get() + "\nGagal menyimpan konfigurasi: " + str(exc))

    def choose():
        selected = filedialog.askdirectory(title="Pilih folder game PC yang sudah diinstal")
        if selected:
            folder.set(selected)
            check()

    def poll():
        nonlocal child
        if child is None:
            return
        code = child.poll()
        if code is None:
            root.after(500, poll)
        else:
            child = None
            check()
            status.set(status.get() + f"\nGame keluar dengan kode {code}; log ada di direktori XDG state.")

    def run():
        nonlocal child
        if child is not None:
            return
        try:
            command, cwd, env, log = core.launch_spec(Path(folder.get()))
            with log.open("ab") as stream:
                child = subprocess.Popen(command, cwd=cwd, env=env, stdout=stream, stderr=subprocess.STDOUT)
            play.configure(state="disabled")
            status.set("Proses game berjalan. Log: " + str(log))
            root.after(500, poll)
        except (OSError, RuntimeError, ValueError) as exc:
            messagebox.showerror("Belum dapat menjalankan game", str(exc))

    ttk.Button(buttons, text="Pilih folder…", command=choose).pack(side="left")
    ttk.Button(buttons, text="Periksa", command=check).pack(side="left", padx=8)
    play = ttk.Button(buttons, text="Main", command=run, state="disabled")
    play.pack(side="right")
    ttk.Label(panel, textvariable=status, wraplength=590, justify="left").pack(anchor="w", pady=20)
    ttk.Label(panel, text="Tidak mengunduh aset • Tidak menjalankan EXE/Wine • Data asli tidak diubah",
              wraplength=590).pack(side="bottom", anchor="w")
    check()
    root.mainloop()
    return 0

if __name__ == "__main__":
    raise SystemExit(main())

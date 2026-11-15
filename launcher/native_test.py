#!/usr/bin/env python3
"""Native-game diagnostic UI. Independent launcher code: MIT.
Does not copy assets or import existing saves.
"""
from pathlib import Path
import datetime
import struct
import os
import platform
import subprocess
import sys
import tkinter as tk
from tkinter import filedialog, messagebox, ttk

RUNTIME = Path(__file__).resolve().parent / 'barnyard-native'

def absolute_xdg(name, fallback):
    value = Path(os.environ.get(name, str(fallback)))
    if not value.is_absolute():
        raise ValueError(f'{name} harus berupa path absolut')
    return value

def log_music_metadata(game, output):
    """Log numeric FSB header metadata only; never exports encoded/decoded audio."""
    try:
        path=game
        for component in ('Data','Sound','FSB','Music.fsb'):
            matches=[p for p in path.iterdir() if p.name.lower()==component.lower()]
            if len(matches)!=1:
                output.write('Music metadata: path missing/ambiguous\n')
                return
            path=matches[0]
        with path.open('rb') as f:
            fixed=f.read(24)
            if len(fixed)!=24 or fixed[:4]!=b'FSB3':
                output.write(f'Music metadata: not FSB3; file_bytes={path.stat().st_size}\n')
                return
            count,headers,data,version,mode=struct.unpack_from('<5I',fixed,4)
            output.write(f'Music FSB3 metadata: file_bytes={path.stat().st_size} samples={count} '
                         f'header_bytes={headers} data_bytes={data} version=0x{version:x} bank_mode=0x{mode:x}\n')
            if headers>16*1024*1024 or count>16384:
                output.write('Music metadata: diagnostic header bound exceeded\n')
                return
            buf=f.read(headers)
        pos=0
        for i in range(count):
            if i and mode&2:
                frames,compressed=struct.unpack_from('<II',buf,pos)
                pos+=8
            else:
                length=struct.unpack_from('<H',buf,pos)[0]
                if length<64 or pos+length>len(buf):
                    raise ValueError(f'invalid header at sample {i}')
                frames,compressed,loopstart,loopend,samplemode,rate=struct.unpack_from('<6I',buf,pos+32)
                volume,pan,priority,channels=struct.unpack_from('<HhHH',buf,pos+56)
                pos+=length
            if i<128 or frames>16*1024*1024:
                output.write(f'Music sample {i}: frames={frames} encoded_bytes={compressed} '
                             f'mode=0x{samplemode:x} hz={rate} channels={channels} '
                             f'volume={volume} pan={pan}\n')
    except (OSError,ValueError,struct.error) as error:
        output.write(f'Music metadata diagnostic failed: {error}\n')

class TestWindow:
    def __init__(self, root):
        self.root = root
        self.process = None
        self.log = None
        self.log_path = None
        root.title('Barnyard Linux — Diagnostik')
        from direct_launch import apply_icon
        apply_icon(root)
        root.geometry('700x570')
        root.minsize(660, 540)
        root.protocol('WM_DELETE_WINDOW', self.close)
        frame = ttk.Frame(root, padding=20)
        frame.pack(fill='both', expand=True)
        ttk.Label(frame, text='Barnyard Linux · Diagnostik', font=('', 17, 'bold')).pack(anchor='w')
        ttk.Label(frame, text='Port komunitas native Linux. Memerlukan data game PC milik Anda.',
                  wraplength=620).pack(anchor='w', pady=(8, 12))
        ttk.Label(frame, text='Pilih folder game PC yang sudah diekstrak (berisi folder Data).',
                  wraplength=620).pack(anchor='w')
        guess = Path.home() / 'Games/Barnyard-PC/APPDIR:./Barnyard'
        from direct_launch import remembered_game
        cached=remembered_game()
        self.folder = tk.StringVar(value=str(cached or guess) if (cached or guess.is_dir()) else '')
        row = ttk.Frame(frame)
        row.pack(fill='x', pady=8)
        ttk.Entry(row, textvariable=self.folder).pack(side='left', fill='x', expand=True)
        self.browse = ttk.Button(row, text='Pilih folder…', command=self.choose)
        self.browse.pack(side='right', padx=(8, 0))
        settings = ttk.Frame(frame)
        settings.pack(fill='x', pady=5)
        self.fullscreen = tk.BooleanVar(value=True)
        self.capture_mouse = tk.BooleanVar(value=True)
        self.fullscreen_check = ttk.Checkbutton(settings, text='Fullscreen', variable=self.fullscreen)
        self.fullscreen_check.pack(side='left')
        self.capture_check = ttk.Checkbutton(settings, text='Kunci mouse ke game', variable=self.capture_mouse)
        self.capture_check.pack(side='left', padx=15)
        ttk.Label(frame, text='Alt+Enter: fullscreen/jendela · Shift+F1: lepas/kunci mouse\n'
                  'Alt+Tab: pindah aplikasi · Klik game: kunci mouse kembali', wraplength=640).pack(anchor='w', pady=4)
        self.start = ttk.Button(frame, text='Jalankan game', command=self.launch)
        self.start.pack(anchor='w', pady=5)
        self.stop = ttk.Button(frame, text='Hentikan game', command=self.stop_game, state='disabled')
        self.stop.pack(anchor='w', pady=5)
        self.status = tk.StringVar(value='Siap. Folder save sebelumnya tetap digunakan; tidak ada impor otomatis.')
        ttk.Label(frame, textvariable=self.status, wraplength=620).pack(anchor='w', pady=(10, 6))
        self.location = tk.StringVar(value='Log teks akan dibuat otomatis di folder Downloads.')
        ttk.Entry(frame, textvariable=self.location, state='readonly').pack(fill='x', pady=4)
        self.open_log = ttk.Button(frame, text='Buka folder log', command=self.show_log, state='disabled')
        self.open_log.pack(anchor='w', pady=4)
        ttk.Label(frame, text='Jika ada masalah: unggah log .txt saja dan ceritakan apakah menu/suara muncul.\n'
                  'Tidak ada data game, Wine, atau DLL Windows di paket ini.', wraplength=620).pack(anchor='w', pady=6)

    def choose(self):
        choice = filedialog.askdirectory(title='Pilih folder Barnyard yang berisi Data',
                                         initialdir=self.folder.get() or str(Path.home()))
        if choice:
            self.folder.set(choice)

    def launch(self):
        if self.process is not None and self.process.poll() is None:
            return
        try:
            if os.geteuid() == 0:
                raise ValueError('Jangan jalankan game dengan sudo/root.')
            game = Path(self.folder.get()).expanduser().resolve(strict=True)
            if not self.folder.get() or not game.is_dir():
                raise ValueError('Pilih folder instalasi PC, bukan ISO/installer.')
            data = [p for p in game.iterdir() if p.name.lower() == 'data' and p.is_dir()]
            if len(data) != 1:
                raise ValueError('Folder terpilih harus berisi satu folder Data. Pilih folder Barnyard di dalam hasil ekstraksi.')
            user = absolute_xdg('XDG_DATA_HOME', Path.home()/'.local/share')/'barnyard-linux-test'
            user = user.resolve()
            if game == user or game in user.parents or user in game.parents:
                raise ValueError('Folder game dan folder save harus terpisah.')
            from direct_launch import remember_game
            remember_game(game)
            user.mkdir(parents=True, exist_ok=True, mode=0o700)
            logdir = Path.home()/'Downloads'
            logdir.mkdir(parents=True, exist_ok=True)
            stamp = datetime.datetime.now().strftime('%Y%m%d-%H%M%S-%f')
            self.log_path = logdir/f'barnyard-{stamp}.txt'
            fd = os.open(self.log_path, os.O_WRONLY|os.O_CREAT|os.O_EXCL, 0o600)
            self.log = os.fdopen(fd, 'w', buffering=1)
            self.log.write('BARNYARD LINUX — 1.0.0 — COMMUNITY NATIVE PORT\n')
            self.log.write(f'Date: {datetime.datetime.now().isoformat()}\nOS: {platform.platform()}\n')
            if Path('/etc/os-release').exists():
                self.log.write(Path('/etc/os-release').read_text()+'\n')
            self.log.write(f'Game root: {game}\nUser data root: {user}\n')
            self.log.write('No existing saves imported. Log may contain local paths/usernames.\n')
            self.log.write(f'Fullscreen requested: {self.fullscreen.get()}; mouse capture requested: {self.capture_mouse.get()}\n')
            self.log.write('Shortcuts: Alt+Enter fullscreen; Shift+F1 release/capture; Alt+Tab switch apps.\n\n')
            log_music_metadata(game,self.log)
            self.log.write('\n--- Native runtime output ---\n')
            self.log.flush()
            env = os.environ.copy()
            env.update(BARNYARD_GAME_DIR=str(game), BARNYARD_USER_DIR=str(user),
                       BARNYARD_FULLSCREEN='1' if self.fullscreen.get() else '0',
                       BARNYARD_CAPTURE_MOUSE='1' if self.capture_mouse.get() else '0')
            self.process = subprocess.Popen([str(RUNTIME)], cwd=user, env=env,
                                            stdout=self.log, stderr=subprocess.STDOUT)
            self.start.configure(state='disabled')
            self.browse.configure(state='disabled')
            self.fullscreen_check.configure(state='disabled')
            self.capture_check.configure(state='disabled')
            self.stop.configure(state='normal')
            self.open_log.configure(state='normal')
            self.location.set(str(self.log_path))
            self.status.set('Game sedang berjalan. Tutup jendela game secara normal, atau klik Hentikan game.')
            self.root.after(200, self.poll)
        except (OSError, ValueError) as error:
            if self.log:
                self.log.write(f'LAUNCH ERROR: {error}\n')
                self.log.close()
                self.log = None
            messagebox.showerror('Belum bisa memulai game', str(error))

    def poll(self):
        if self.process is None:
            return
        code = self.process.poll()
        if code is None:
            self.root.after(200, self.poll)
            return
        if self.log:
            self.log.write(f'\nRuntime exit status: {code}\n')
            self.log.write('Diagnostic log: review before sharing.\n')
            self.log.close()
            self.log = None
        self.start.configure(state='normal')
        self.browse.configure(state='normal')
        self.fullscreen_check.configure(state='normal')
        self.capture_check.configure(state='normal')
        self.stop.configure(state='disabled')
        if code == 3:
            text = 'Data startup tidak lengkap. Kirim log .txt untuk melihat nama file yang hilang.'
        elif code == 0:
            text = 'Proses selesai. Kirim log dan keterangan apakah menu serta suara muncul.'
        else:
            text = f'Proses berhenti (kode {code}). Kirim log .txt untuk pemeriksaan.'
        self.status.set(text)

    def stop_game(self):
        if self.process is not None and self.process.poll() is None:
            process = self.process
            process.terminate()
            self.status.set('Menghentikan game…')
            self.root.after(5000, lambda: process.kill() if process.poll() is None else None)

    def show_log(self):
        if self.log_path:
            try:
                subprocess.Popen(['xdg-open', str(self.log_path.parent)],
                                 stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            except OSError as error:
                messagebox.showerror('Lokasi log', f'{self.log_path}\n{error}')

    def close(self):
        if self.process is not None and self.process.poll() is None:
            if not messagebox.askyesno('Tutup diagnostik?', 'Hentikan proses game dan tutup aplikasi?'):
                return
            self.process.terminate()
            try:
                self.process.wait(timeout=3)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait(timeout=3)
        if self.log:
            self.log.write('\nDiagnostic UI closed by user.\n')
            self.log.close()
            self.log = None
        self.root.destroy()

def main():
    if '--version' in sys.argv:
        print('barnyard-linux 1.0.0; native i386; unofficial community port')
        return 0
    if '--runtime-help' in sys.argv:
        return subprocess.call([str(RUNTIME), '--help'])
    root = tk.Tk()
    TestWindow(root)
    root.mainloop()
    return 0

if __name__ == '__main__':
    raise SystemExit(main())

#!/usr/bin/env python3
"""Direct native launcher; first-use folder picker only. Independent code: MIT."""
from pathlib import Path
import datetime
import json
import os
import platform
import subprocess
import sys
import tempfile
import tkinter as tk
from tkinter import filedialog, messagebox
import native_test

VERSION = '1.0.0'
RUNTIME = Path(__file__).resolve().parent / 'barnyard-native'

def config_file():
    return native_test.absolute_xdg('XDG_CONFIG_HOME', Path.home()/'.config')/'barnyard-linux-test/launcher.json'

def user_root():
    return native_test.absolute_xdg('XDG_DATA_HOME', Path.home()/'.local/share')/'barnyard-linux-test'

def validate_game(value):
    if not isinstance(value,str) or not value or len(value)>8192:
        raise ValueError('Pilih folder game yang berisi folder Data.')
    game=Path(value).expanduser().resolve(strict=True)
    if not game.is_dir():
        raise ValueError('Pilih folder instalasi PC, bukan ISO/installer.')
    data=[p for p in game.iterdir() if p.name.lower()=='data' and p.is_dir()]
    if len(data)!=1:
        raise ValueError('Folder terpilih harus berisi satu folder Data. Pilih folder Barnyard di dalam hasil ekstraksi.')
    user=user_root().resolve()
    if game==user or game in user.parents or user in game.parents:
        raise ValueError('Folder game dan folder save harus terpisah.')
    return game

def remembered_game():
    try:
        path=config_file()
        if path.is_symlink() or path.stat().st_size>16384:
            return None
        return validate_game(json.loads(path.read_text())['game_root'])
    except (OSError,ValueError,KeyError,TypeError):
        return None

def remember_game(game):
    game=validate_game(str(game))
    path=config_file()
    path.parent.mkdir(parents=True,exist_ok=True,mode=0o700)
    fd,temp=tempfile.mkstemp(prefix='.launcher-',dir=path.parent)
    try:
        with os.fdopen(fd,'w') as out:
            json.dump({'version':1,'game_root':str(game)},out)
            out.write('\n');out.flush();os.fsync(out.fileno())
        os.replace(temp,path)
    finally:
        if os.path.exists(temp):os.unlink(temp)

def apply_icon(root):
    path=Path('/usr/share/icons/hicolor/128x128/apps/barnyard-linux.png')
    if not path.is_file():return
    try:
        icon=tk.PhotoImage(file=str(path))
        root.iconphoto(True,icon);root._barnyard_icon=icon
    except tk.TclError:pass

def show_error(title,text):
    root=tk.Tk();root.withdraw();apply_icon(root)
    try:messagebox.showerror(title,text,parent=root)
    finally:root.destroy()

def choose_game():
    root=tk.Tk();root.withdraw();apply_icon(root)
    initial=remembered_game()
    if initial is None:
        guess=Path.home()/'Games/Barnyard-PC/APPDIR:./Barnyard'
        initial=guess if guess.is_dir() else Path.home()
    try:
        while True:
            value=filedialog.askdirectory(parent=root,title='Sekali saja: pilih folder Barnyard yang berisi Data',initialdir=str(initial))
            if not value:return None
            try:
                game=validate_game(value);remember_game(game);return game
            except (OSError,ValueError) as error:
                messagebox.showerror('Folder belum sesuai',str(error),parent=root)
                initial=Path(value)
    finally:root.destroy()

def run_game(game):
    if os.geteuid()==0:raise ValueError('Jangan jalankan game dengan sudo/root.')
    game=validate_game(str(game))
    user=user_root().resolve()
    user.mkdir(parents=True,exist_ok=True,mode=0o700)
    logdir=Path.home()/'Downloads';logdir.mkdir(parents=True,exist_ok=True)
    stamp=datetime.datetime.now().strftime('%Y%m%d-%H%M%S-%f')
    logfile=logdir/f'barnyard-{stamp}.txt'
    fd=os.open(logfile,os.O_WRONLY|os.O_CREAT|os.O_EXCL,0o600)
    with os.fdopen(fd,'w',buffering=1) as out:
        out.write(f'BARNYARD LINUX — {VERSION} — COMMUNITY NATIVE PORT\n')
        out.write(f'Date: {datetime.datetime.now().isoformat()}\nOS: {platform.platform()}\n')
        if Path('/etc/os-release').exists():out.write(Path('/etc/os-release').read_text()+'\n')
        out.write(f'Game root: {game}\nUser data root: {user}\n')
        out.write('Existing saves preserved; no saves imported. Log may contain local paths/usernames.\n')
        out.write('Direct launch; display/language loaded from in-game native preferences.\n')
        out.write('Alt+Enter: fullscreen/windowed; Shift+F1: release/capture mouse.\n')
        native_test.log_music_metadata(game,out)
        out.write('\n--- Native runtime output ---\n');out.flush()
        env=os.environ.copy()
        env.update(BARNYARD_GAME_DIR=str(game),BARNYARD_USER_DIR=str(user))
        try:
            code=subprocess.call([str(RUNTIME)],cwd=user,env=env,stdout=out,stderr=subprocess.STDOUT)
        except OSError as error:
            out.write(f'LAUNCH ERROR: {error}\n');raise
        out.write(f'\nRuntime exit status: {code}\n')
    return code,logfile

def main():
    if '--version' in sys.argv:
        print(f'barnyard-linux {VERSION}; native i386; unofficial community port');return 0
    if '--diagnostic' in sys.argv:
        return native_test.main()
    if '--runtime-help' in sys.argv:
        return subprocess.call([str(RUNTIME),'--help'])
    try:
        if os.geteuid()==0:raise ValueError('Jangan jalankan game dengan sudo/root.')
        game=None if '--choose-folder' in sys.argv else remembered_game()
        if game is None:game=choose_game()
        if game is None:return 0
        code,log=run_game(game)
        if code:
            reason='Data startup belum lengkap.' if code==3 else f'Game berhenti dengan kode {code}.'
            show_error('Barnyard belum dapat berjalan',f'{reason}\nLog: {log}\n\nUntuk mengganti folder: menu aplikasi > klik kanan Barnyard > Pilih folder game, atau jalankan barnyard-linux --choose-folder.')
        return code if 0<=code<=255 else 1
    except (OSError,ValueError) as error:
        show_error('Barnyard belum dapat dimulai',str(error));return 1

if __name__=='__main__':raise SystemExit(main())

"""Installed community-package diagnostic UI + actual packaged ELF; private incomplete sample.
Run as non-root under Xvfb. No fake native process or gameplay claim.
"""
from pathlib import Path
import os
import sys
import tkinter as tk
from tkinter import messagebox
sys.path.insert(0, '/usr/lib/barnyard-linux')
from native_test import TestWindow
assert os.geteuid() != 0
errors=[]
messagebox.showerror=lambda *args,**kwargs:errors.append(str(args))
root=tk.Tk()
window=TestWindow(root)
assert root.title()=="Barnyard Linux — Diagnostik"
assert hasattr(root,"_barnyard_icon") and root._barnyard_icon.width()==128
assert window.fullscreen.get() is True and window.capture_mouse.get() is True
window.fullscreen.set(False); window.capture_mouse.set(False)
window.folder.set('/private-sample')
window.launch()
assert not errors,errors
assert window.process is not None
finished=[False]
def check():
    if window.process.poll() is None:
        root.after(100,check)
        return
    window.poll()
    finished[0]=True
    root.destroy()
root.after(100,check)
root.after(15000,root.destroy)
root.mainloop()
assert finished[0], 'Timeout'
assert window.process.returncode==3, window.process.returncode
assert window.log_path and window.log_path.stat().st_mode & 0o077 == 0
text=window.log_path.read_text()
assert 'Data/MiscTextures/MaterialProperties.trb' in text
assert 'Runtime exit status: 3' in text
assert 'Fullscreen requested: False; mouse capture requested: False' in text
assert 'Data startup tidak lengkap' in window.status.get()
assert not errors,errors
print('PASS: installed Tk UI starts real packaged ELF as nonroot, captures partial-data rejection exit 3 and auto-log (0600). NOT full data/menu/gameplay.')

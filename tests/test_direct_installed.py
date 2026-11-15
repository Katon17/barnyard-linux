"""Installed native launcher uses the real native ELF with private incomplete data.
Run nonroot under Xvfb; only picker/error dialogs are intercepted for automation.
"""
from pathlib import Path
import os,subprocess,sys
sys.path.insert(0,'/usr/lib/barnyard-linux')
import direct_launch as app
assert os.geteuid()!=0
errors=[]
app.show_error=lambda *args:errors.append(args)
# Real Tk root; return a supplied folder instead of waiting for a human click.
app.filedialog.askdirectory=lambda **kwargs:'/private-sample'
assert app.choose_game()==Path('/private-sample')
assert app.remembered_game()==Path('/private-sample')
assert app.config_file().stat().st_mode & 0o077==0
# Once remembered, there must be no picker/launcher window on future launches.
def forbidden():raise AssertionError('Unexpected repeat folder picker')
app.choose_game=forbidden
sys.argv=['barnyard-linux']
for attempt in range(2):
    assert app.main()==3
assert len(errors)==2
logs=sorted((Path.home()/'Downloads').glob('barnyard-*.txt'))
assert len(logs)==2
for log in logs:
    assert log.stat().st_mode & 0o077==0
    text=log.read_text()
    assert '1.0.0' in text and 'Runtime exit status: 3' in text
    assert 'Data/MiscTextures/MaterialProperties.trb' in text
    assert 'Direct launch;' in text
assert '1.0.0' in subprocess.check_output(['barnyard-linux','--version'],text=True)
assert app.user_root()==Path.home()/'.local/share/barnyard-linux-test'
assert 'barnyard-linux 1.0.0' in subprocess.check_output(['barnyard-linux-test','--version'],text=True)
assert app.RUNTIME==Path('/usr/lib/barnyard-linux/barnyard-native')
assert not Path('/usr/share/applications/barnyard-linux-test.desktop').exists()
desktop=Path('/usr/share/applications/barnyard-linux.desktop').read_text()
assert 'Name=Barnyard Linux\n' in desktop and 'Icon=barnyard-linux\n' in desktop
assert 'Exec=barnyard-linux\n' in desktop and 'StartupWMClass=barnyard-linux\n' in desktop
print('PASS: installed community release 1.0.0 nonroot first-use folder memory, two direct starts without launcher/picker, actual packaged ELF incomplete-data exit 3, private logs, version and unchanged save root. NOT gameplay.')

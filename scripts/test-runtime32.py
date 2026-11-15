#!/usr/bin/env python3
"""Exercise the ACTUAL linked game's early entry point, without fake gameplay."""
from pathlib import Path
import json,os,subprocess,tempfile,sys
binary=Path(sys.argv[1] if len(sys.argv)>1 else 'build/full32/barnyard-native').resolve(strict=True)
report=Path('reports/runtime32');report.mkdir(parents=True,exist_ok=True)
transcript=[]
def run(args,env,expected):
    result=subprocess.run([str(binary),*args],env=env,capture_output=True,text=True,timeout=15)
    transcript.append({'args':args,'exit':result.returncode,'stdout':result.stdout,'stderr':result.stderr})
    assert result.returncode==expected,transcript[-1]
    return result
base=os.environ.copy()
for key in ('BARNYARD_GAME_DIR','BARNYARD_USER_DIR'):base.pop(key,None)
assert 'barnyard-linux-native 1.0.0; native i386' in run(['--version'],base,0).stdout
assert 'PC data' in run(['--help'],base,0).stdout
run([],base,1)
with tempfile.TemporaryDirectory(prefix='barnyard-runtime-') as temp:
    root=Path(temp);game=root/'game';user=root/'user';game.mkdir();user.mkdir()
    env=base|{'BARNYARD_GAME_DIR':str(game),'BARNYARD_USER_DIR':str(user)}
    run([],env,3)  # Real main rejects missing data BEFORE graphics or game tasks.
    run(['--check-data'],env,3)
    run(['--check-data'],env|{'BARNYARD_USER_DIR':str(game)},1)
    # Headerless sentinel files test ONLY indicator resolver, not TRB compatibility.
    (game/'DaTa/gUi').mkdir(parents=True)
    for name in ('fOnTs.trb','tExSeC.trb'):(game/'DaTa/gUi'/name).write_bytes(b'NOT A GAME ASSET - indicator test only')
    result=run(['--check-data'],env,0)
    assert 'NOT verified' in result.stdout
    # Indicators alone must not enter GL/model-loader with missing resources.
    result=run([],env,3)
    assert 'incomplete PC installation' in result.stderr
    assert 'Data/Locale/eng-uk.trb' in result.stderr
    # Every known early requirement is checked individually, including empty
    # files. Headerless fixtures are never allowed to enter the game loop.
    startup=['Data/Locale/eng-uk.trb','Data/Assets/lib_startup.trb',
             'Data/lib_startup.trb','Data/MiscTextures/MaterialProperties.trb']
    for i,name in enumerate(startup):
        path=game/'DaTa'/name.removeprefix('Data/');path.parent.mkdir(parents=True,exist_ok=True)
        path.write_bytes(b'')
        result=run([],env,3)
        assert name in result.stderr
        if i+1<len(startup):path.write_bytes(b'NOT A GAME ASSET - preflight test only')
    for path in (game/'DaTa/gUi').iterdir():assert path.read_bytes()==b'NOT A GAME ASSET - indicator test only'
(report/'entrypoint-tests.json').write_text(json.dumps(transcript,indent=2)+'\n')
for command,name in [(['file',str(binary)],'executable-type.txt'),(['ldd',str(binary)],'dependencies.txt')]:
    result=subprocess.run(command,capture_output=True,text=True,check=True)
    (report/name).write_text(result.stdout+result.stderr)
    assert 'not found' not in result.stdout
print('PASS: actual game ELF32 entry point version/help, root failure, missing data rejection, case-resolved indicators; NO gameplay/data load claim')

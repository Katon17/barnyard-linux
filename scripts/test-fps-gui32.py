#!/usr/bin/env python3
"""Headless actual AGUI2 component test; requires a successful full native build.

Does not run game startup or create a window. Only the test uses disabled C++
access checks. No game object/header is edited; a disposable AppBoot copy has
its main symbol renamed so the test supplies its own main.
"""
from pathlib import Path
import json,subprocess,tempfile,os
root=Path(__file__).resolve().parents[1]
summary=json.loads((root/'reports/full32/summary.json').read_text())
assert summary['linked'] and summary['failed']==0
flags=json.loads((root/'reports/full32/flags.json').read_text())
rows=json.loads((root/'reports/full32/results.json').read_text())
with tempfile.TemporaryDirectory(prefix='barnyard-fps-gui-') as tmp:
    work=Path(tmp);objects=[];renamed=False
    for row in rows:
        assert row['status']=='PASS'
        source=row['object']
        if row['source'].endswith('/AppBoot.cpp'):
            target=work/'AppBoot-no-main.o'
            subprocess.run(['objcopy','--redefine-sym','main=barnyard_unused_game_main',source,str(target)],cwd=root,check=True)
            source=str(target);renamed=True
        objects.append(source)
    assert renamed
    response=work/'objects.rsp';response.write_text('\n'.join(objects)+'\n')
    binary=work/'test-fps-gui'
    subprocess.run([os.environ.get('CXX','g++-14'),*flags,'-fno-access-control','-include','build/full32/game-pch.h',
                    'tests/test_native_fps_gui32.cpp','@'+str(response),'-Wl,--gc-sections','-pthread','-ldl',
                    '-lSDL2','-lGLEW','-lGL','-lavformat','-lavcodec','-lavutil','-lswscale','-lswresample','-o',str(binary)],cwd=root,check=True)
    subprocess.run([str(binary)],cwd=root,check=True,timeout=15)

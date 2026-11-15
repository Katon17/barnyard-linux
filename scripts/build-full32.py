#!/usr/bin/env python3
"""Full selected Linux/GL source compilation. Assertions remain enabled.

Outputs under build/ are disposable and excluded from workspace snapshots.
No asset download, Wine, or stub-success media backend. A link is attempted only
with --link after a CLEAN full successful source compilation.
"""
from pathlib import Path
import argparse
import concurrent.futures as futures
import json
import os
import re
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parent.parent
os.chdir(ROOT)
p = argparse.ArgumentParser()
p.add_argument('--jobs', type=int, default=2)
p.add_argument('--failed-only', action='store_true', help='Diagnostic incremental retry; never sufficient to link')
p.add_argument('--match', default='', help='Diagnostic regex subset; never sufficient to link')
p.add_argument('--link', action='store_true')
a = p.parse_args()
if not 1 <= a.jobs <= 4:
    p.error('jobs must be 1..4')
if a.link and (a.failed_only or a.match):
    p.error('--link requires a full build, not a diagnostic retry')
subprocess.run(['bash', 'scripts/bootstrap.sh'], check=True)
UP = Path('vendor/openbarnyard')
BUILD = Path('build/full32')
REPORT = Path('reports/full32')
BUILD.mkdir(parents=True, exist_ok=True)
REPORT.mkdir(parents=True, exist_ok=True)
subprocess.run([sys.executable, 'tools/pooledstrings.py', str(UP/'OpenBarnyard/Source/APooledStrings.txt'), str(BUILD/'APooledStrings.cpp')], check=True)
flags = ['-m32','-std=c++20','-D_FILE_OFFSET_BITS=64','-fshort-wchar','-fno-strict-aliasing',
         '-DTOSHI_SKU_LINUX','-DTRENDERINTERFACE_GL','-DBARNYARD_COMMUNITY_PATCH',
         '-DTOSHI_USER_ENGINE','-DTOSHI_RELEASE','-DICE_NO_DLL','-DBAN_OPCODE_AUTOLINK',
         '-ffunction-sections','-fdata-sections','-fmax-errors=4',
         '-Wno-invalid-constexpr','-Wno-invalid-offsetof']
for directory in ['src', str(UP/'Toshi/Source'),str(UP/'Toshi/Resources'),
                  str(UP/'OpenBarnyard/Source'),str(UP/'PrimeDebug/Source'),
                  str(UP/'Toshi/Vendor/Opcode/Source'),str(UP/'Toshi/Vendor/fmod/include'),
                  str(UP/'Toshi/Vendor/bink/include'),str(UP/'Toshi/Vendor/imgui'),'/usr/include/stb']:
    flags += ['-I',directory]
cxx = os.environ.get('CXX','g++')
headers = {'engine':UP/'Toshi/Source/ToshiPCH.h', 'game':UP/'OpenBarnyard/Source/pch.h',
           'prime':UP/'PrimeDebug/Source/PrimeDebugPCH.h','opcode':UP/'Toshi/Vendor/Opcode/Source/StdAfx.h'}
for group, header in headers.items():
    wrapper = BUILD/f'{group}-pch.h'
    wrapper.write_text(f'#include "{ROOT / "src/Platform/Linux/TCRT_Linux.h"}"\n#include "{ROOT/header}"\n')
    with (REPORT/f'pch-{group}.log').open('w') as log:
        result = subprocess.run([cxx,*flags,'-x','c++-header',str(wrapper),'-o',str(wrapper)+'.gch'],stdout=log,stderr=subprocess.STDOUT)
    if result.returncode:
        print(f'PCH FAILED: {group}; see {REPORT}/pch-{group}.log',flush=True)
        raise SystemExit(1)

sources = []
# Match upstream common directories, without Windows-only mod loader/ImGui integration.
allowed = {'Animation','Assets','Cameras','Debug','Helpers','GameInterface','MiniGame','GUI','HUD',
           'Input','Locale','Memory','Movie','Render','Sound','SoundBank','Tasks','UnitMovement','FX',
           'Terrain','World','Collision','Physics','FSM','CVar','Player','Units','Quests'}
game = UP/'OpenBarnyard/Source'
for source in sorted(game.rglob('*.cpp')):
    rel=source.relative_to(game)
    if len(rel.parts)==1 or rel.parts[0] in allowed or rel.as_posix()=='Mod/AModEvents.cpp' or rel.parts[:2]==('Platform','GL'):
        sources.append((source,'game'))
for name in ['Platform/Windows/ADisplayModes_Win.cpp','Platform/Windows/ARenderer_Win.cpp','Platform/Windows/Input/AInputHandler_Win.cpp']:
    sources.append((game/name,'game'))
sources.append((BUILD/'APooledStrings.cpp','game'))
engine=UP/'Toshi/Source'
for source in sorted(engine.rglob('*.cpp')):
    rel=source.relative_to(engine)
    if rel.parts[0]=='Mod':
        continue  # ModMemory belongs to separately linked mods, not the host.
    if rel.parts[0]=='Platform':
        if len(rel.parts)<3 or rel.parts[1]!='GL' or source.name=='TMSWindow.cpp':
            continue
    sources.append((source,'engine'))
for source in sorted((UP/'PrimeDebug/Source').rglob('*.cpp')):
    sources.append((source,'prime'))
for source in sorted((UP/'Toshi/Vendor/Opcode/Source').rglob('*.cpp')):
    sources.append((source,'opcode'))
for source in sorted(Path('src/Platform/Linux').rglob('*.cpp')):
    sources.append((source,'game' if source.name in {'AFMOD_Linux.cpp','ANativeAboutState.cpp'} else 'engine'))
assert len({str(source) for source, group in sources})==len(sources)
manifest=[{'source':str(source),'group':group} for source,group in sources]
(REPORT/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
(REPORT/'flags.json').write_text(json.dumps(flags,indent=2)+'\n')
previous={}
if (REPORT/'results.json').exists():
    previous={r['source']:r for r in json.loads((REPORT/'results.json').read_text())}
todo=sources
if a.failed_only:
    todo=[(s,g) for s,g in todo if previous.get(str(s),{}).get('status')!='PASS']
if a.match:
    todo=[(s,g) for s,g in todo if re.search(a.match,str(s))]
clean=not a.failed_only and not a.match
print(f'Selected {len(sources)} total source units; compiling {len(todo)}; clean={clean}; jobs={a.jobs}',flush=True)

def compile_one(item):
    source, group=item
    name=str(source).replace('/','_').removesuffix('.cpp')
    obj=BUILD/f'{name}.o'
    obj.unlink(missing_ok=True)
    log=REPORT/f'{name}.log'
    start=time.monotonic()
    cmd=[cxx,*flags,'-Werror=invalid-pch','-include',str(BUILD/f'{group}-pch.h'),'-c',str(source),'-o',str(obj)]
    with log.open('w') as out:
        result=subprocess.run(cmd,stdout=out,stderr=subprocess.STDOUT)
    errors=[line.strip() for line in log.read_text(errors='replace').splitlines() if 'error:' in line][:5]
    return {'source':str(source),'group':group,'status':'PASS' if result.returncode==0 else 'FAIL',
            'code':result.returncode,'seconds':round(time.monotonic()-start,2),'object':str(obj),'log':str(log),'errors':errors}
results={} if clean else previous.copy()
start=time.monotonic()
with futures.ThreadPoolExecutor(max_workers=a.jobs) as pool:
    for i, result in enumerate(pool.map(compile_one,todo),1):
        results[result['source']]=result
        if result['status']=='FAIL':
            print(f'FAIL {result["source"]}: '+(' | '.join(result['errors']) or f'see {result["log"]}'),flush=True)
        elif i%50==0:
            print(f'{i}/{len(todo)} compiled',flush=True)
        # Persist progress so an interrupted diagnostic still has a truthful record.
        (REPORT/'results.json').write_text(json.dumps(list(results.values()),indent=2)+'\n')
ordered=[results[str(s)] for s,g in sources if str(s) in results]
failed=[r for r in ordered if r['status']!='PASS']
summary={'selected':len(sources),'compiled_this_run':len(todo),'recorded':len(ordered),
         'passed':len(ordered)-len(failed),'failed':len(failed),'clean_full_run':clean,
         'elapsed_seconds':round(time.monotonic()-start,2),'gameplay':False,'linked':False}
(REPORT/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
(REPORT/'failures.txt').write_text('\n\n'.join(r['source']+'\n'+'\n'.join(r['errors']) for r in failed)+'\n')
print(json.dumps(summary),flush=True)
if failed or len(ordered)!=len(sources):
    print('LINK BLOCKED: source compilation incomplete; no game executable.',flush=True)
    raise SystemExit(1)
if a.link:
    objects=[r['object'] for r in ordered]
    (BUILD/'objects.rsp').write_text('\n'.join(objects)+'\n')
    with (REPORT/'link.log').open('w') as log:
        result=subprocess.run([cxx,'-m32','@'+str(BUILD/'objects.rsp'),'-Wl,--gc-sections','-pthread','-ldl',
                               '-lSDL2','-lGLEW','-lGL','-lavformat','-lavcodec','-lavutil','-lswscale','-lswresample','-o',str(BUILD/'barnyard-native')],stdout=log,stderr=subprocess.STDOUT)
    if result.returncode:
        print(f'LINK FAILED: see {REPORT}/link.log; no playable claim.',flush=True)
        raise SystemExit(1)
    summary['linked']=True
    (REPORT/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
    print('Linked experimental executable under build/. Gameplay/release gates remain untested.',flush=True)

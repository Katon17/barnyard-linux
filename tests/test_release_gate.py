#!/usr/bin/env python3
"""Negative release-gate checks against a real runtime/approved manifest. No packaging bypass."""
from pathlib import Path
import json,subprocess,sys,tempfile,copy
root=Path(__file__).resolve().parents[1]
runtime=Path(sys.argv[1]).resolve();manifest=Path(sys.argv[2]).resolve()
m=json.loads(manifest.read_text())
with tempfile.TemporaryDirectory(prefix='barnyard-gate-') as temp:
    p=Path(temp)/'manifest.json'
    variants=[]
    for key in ['gameplay_verified','save_reload_verified','audio_video_verified','xdg_adapter_verified','entrypoint_smoke_verified']:
        x=copy.deepcopy(m);x[key]=False;variants.append((key,x))
    x=copy.deepcopy(m);x['sha256']='not-the-runtime-hash';variants.append(('hash',x))
    x=copy.deepcopy(m);x['baseline']='not Noble';variants.append(('baseline',x))
    x=copy.deepcopy(m);x['launcher_contract']='unknown';variants.append(('contract',x))
    x=copy.deepcopy(m);x['verification_basis']={};variants.append(('evidence basis',x))
    x=copy.deepcopy(m);x['build_summary']['linked']=False;variants.append(('full link',x))
    for name,value in variants:
        p.write_text(json.dumps(value))
        r=subprocess.run(['bash','packaging/build-deb.sh',str(runtime),str(p)],cwd=root,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=15)
        assert r.returncode!=0 and 'RELEASE BLOCKED' in r.stdout,(name,r.stdout)
print('PASS: release packager refuses five missing milestones, bad runtime hash/baseline/contract/evidence and incomplete full link; old safety gates retained.')

#!/usr/bin/env python3
"""Verify applied patches reconstruct every modified tracked vendor source byte-for-byte."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
repo = root / 'vendor/openbarnyard'
patches = sorted((root / 'patches').glob('*.patch'))
# Compare materialized index blobs directly. Restored partial-clone/sparse
# worktrees can crash Git's diff attribute/preload path; never interpret a
# failed diff as a pristine tree, or lazily fetch excluded binary assets.
subprocess.run(['git','-C',str(repo),'diff-index','--cached','--quiet','HEAD'],check=True)
records=subprocess.check_output(['git','-C',str(repo),'ls-files','-s','-t','-z']).split(b'\0')
originals={}
with subprocess.Popen(['git','-C',str(repo),'cat-file','--batch'],stdin=subprocess.PIPE,stdout=subprocess.PIPE) as cat:
    for record in records:
        if not record:continue
        meta,path=record.split(b'\t',1)
        flag,mode,oid,stage=meta.split()
        if flag==b'S':continue  # Outside sparse checkout; never materialize.
        if stage!=b'0':raise SystemExit('Unmerged upstream index')
        name=path.decode();current=repo/name
        if not current.is_file():raise SystemExit(f'Missing tracked source: {name}')
        cat.stdin.write(oid+b'\n');cat.stdin.flush()
        header=cat.stdout.readline().split()
        if len(header)!=3 or header[1]!=b'blob':raise SystemExit(f'Invalid upstream blob: {name}')
        expected=cat.stdout.read(int(header[2]));assert cat.stdout.read(1)==b'\n'
        if expected!=current.read_bytes():originals[name]=expected
    cat.stdin.close();cat.wait()
    if cat.returncode:raise SystemExit('Upstream object reader failed')
files=sorted(originals)
if not files:
    raise SystemExit('No applied source modifications to verify; run bootstrap first.')
with tempfile.TemporaryDirectory(prefix='barnyard-patch-check-') as temp:
    # git archive can prefetch unrelated promisor blobs before path filtering.
    # Materialize only the exact source files needed by these patches instead.
    for source in files:
        pristine = Path(temp) / source
        pristine.parent.mkdir(parents=True, exist_ok=True)
        pristine.write_bytes(originals[source])
    for patch in patches:
        subprocess.run(['git', 'apply', '--check', str(patch)], cwd=temp, check=True)
        subprocess.run(['git', 'apply', str(patch)], cwd=temp, check=True)
    for source in files:
        if (Path(temp) / source).read_bytes() != (repo / source).read_bytes():
            raise SystemExit(f'Patch/source mismatch: {source}')
message = f'PASS: pristine pinned upstream -> all {len(patches)} patches -> byte-identical working source ({len(files)} tracked files)\n'
report = root / 'reports/core32/patch-chain.txt'
report.parent.mkdir(parents=True, exist_ok=True)
report.write_text(message)
print(message, end='')

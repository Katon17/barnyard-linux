#!/usr/bin/env python3
"""Audit private translation pairs against the distributable hash-only table."""
from pathlib import Path
import argparse,collections,hashlib,json,re
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--work-dir',type=Path,required=True)
p.add_argument('--project',type=Path,default=Path(__file__).resolve().parents[2])
a=p.parse_args();base=a.work_dir;root=a.project
pairs=json.loads((base/'translations-private.json').read_text())
locale=json.loads((base/'source-locale.json').read_text())
subs=json.loads((base/'source-subtitles.json').read_text())
assert set(pairs)==set(locale+[r['text'] for r in subs])
pattern=re.compile(r'%c_[A-Za-z0-9_]+\[[^\]]+\]|%\{[^}]+\}|%player_[A-Za-z0-9_]+|%%|%(?:[-+0 #]*\d*(?:\.\d+)?[csdifuxXgGeE])|%\d+')
protected=0
for src,dst in pairs.items():
    assert src.count('%')==dst.count('%')
    for mark in ['\n','\r']:assert src.count(mark)==dst.count(mark)
    for token,count in collections.Counter(pattern.findall(src)).items():
        assert dst.count(token)==count
        if token.startswith('%player_'):
            assert len(re.findall(re.escape(token)+r'(?![A-Za-z0-9_])',dst))==count
        protected+=count
    assert all(ord(c)<0x10000 for c in dst), 'Use valid C++ universal characters'
def fnv(s):
    h=14695981039346656037
    for b in s.encode('utf-16le'):h=((h^b)*1099511628211)&0xffffffffffffffff
    return h
inc=root/'src/Platform/Linux/IndonesianData.inc'
compiled={}
for line in inc.read_text().splitlines():
    if line.startswith('//'):continue
    m=re.fullmatch(r'\{0x([0-9a-f]{16})ULL, u(".*")\},',line);assert m
    h=int(m[1],16);assert h not in compiled;compiled[h]=json.loads(m[2])
assert len(compiled)==len([s for s in pairs if s])
for src,dst in pairs.items():
    if src:assert compiled[fnv(src)]==dst
report={'locale_entries':len(locale),'subtitle_lines':len(subs),'unique_source_texts':len(pairs),
        'compiled_nonempty_records':len(compiled),'protected_format_tokens_checked':protected,
        'percent_and_linebreak_counts_preserved':True,'player_token_boundaries_preserved':True,
        'hashes_unique_and_values_equal_private_catalog':True,
        'unique_strings_changed':sum(s!=d for s,d in pairs.items()),
        'unique_strings_unchanged':sum(s==d for s,d in pairs.items()),
        'unchanged_note':'Includes empty strings, names, key names, brands, song titles, interjections, shared vocabulary, and machine outputs not context-reviewed. Not proof of 100% rendered-text localization.',
        'table_sha256':hashlib.sha256(inc.read_bytes()).hexdigest(),
        'physical_game_language_switch_tested':False}
(root/'reports/localization-audit.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))

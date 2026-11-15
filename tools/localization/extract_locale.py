#!/usr/bin/env python3
"""Extract a PRIVATE English UK catalog from an owned uncompressed TRBF file."""
from pathlib import Path
import argparse,json,os,struct
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('trb',type=Path);p.add_argument('output',type=Path)
a=p.parse_args();os.umask(0o077)
b=a.trb.read_bytes();assert len(b)<=4*1024*1024
assert b[:4]==b'TSFL' and b[8:12]==b'TRBF'
chunks={};at=12
while at+8<=len(b):
    name=b[at:at+4];n=struct.unpack_from('<I',b,at+4)[0]
    assert name not in chunks and at+8+n<=len(b)
    chunks[name]=b[at+8:at+8+n];at+=8+((n+3)&~3)
h=chunks[b'HDRX'];nsec=struct.unpack_from('<I',h,4)[0]
assert 0<nsec<=32 and len(h)>=8+16*nsec
sections=[];at=0
for i in range(nsec):
    n=struct.unpack_from('<I',h,12+16*i)[0]
    assert at+n<=len(chunks[b'SECT'])
    sections.append(chunks[b'SECT'][at:at+n]);at+=n
r=chunks[b'RELC'];nr=struct.unpack_from('<I',r)[0]
assert len(r)==4+8*nr
relocs={}
for i in range(nr):
    src,dst,off=struct.unpack_from('<HHI',r,4+8*i)
    assert src<nsec and dst<nsec and off+4<=len(sections[src])
    relocs[src,off]=dst
sy=chunks[b'SYMB'];ns=struct.unpack_from('<I',sy)[0];names=4+ns*12
symbol=None
for i in range(ns):
    sec,no,pad,ha,off=struct.unpack_from('<HHHhI',sy,4+12*i)
    end=sy.index(0,names+no)
    if sy[names+no:end]==b'LocaleStrings':symbol=(sec,off)
assert symbol is not None
sec,off=symbol;count,ptr=struct.unpack_from('<II',sections[sec],off)
assert 0<count<10000
psec=relocs[sec,off+4];pointers=sections[psec]
assert ptr+count*4<=len(pointers)
out=[]
for i in range(count):
    pos=ptr+4*i;offset=struct.unpack_from('<I',pointers,pos)[0]
    dst=relocs[psec,pos];s=sections[dst];end=offset
    assert offset%2==0 and offset+2<=len(s)
    while s[end:end+2]!=b'\0\0':
        end+=2;assert end+2<=len(s) and end-offset<131072
    out.append(s[offset:end].decode('utf-16le'))
a.output.write_text(json.dumps(out,ensure_ascii=False,indent=2))
print('Private locale extraction:',len(out),'entries; original text not printed.')

#!/usr/bin/env python3
"""Resize supplied original fan artwork and write PNG icons + SDL-readable BMPv4."""
from pathlib import Path
from collections import deque
import struct
from PIL import Image
root=Path(__file__).resolve().parents[1]
folder=root/'packaging/icons'
im=Image.open(folder/'barnyard-linux-original.png').convert('RGBA')
# Make only connected near-white exterior corners transparent, not sign/clouds.
w,h=im.size;px=im.load();queue=deque([(0,0),(w-1,0),(0,h-1),(w-1,h-1)]);seen=set()
while queue:
    x,y=queue.popleft()
    if (x,y) in seen or not (0<=x<w and 0<=y<h):continue
    seen.add((x,y));r,g,b,a=px[x,y]
    if min(r,g,b)<=230:continue
    px[x,y]=(r,g,b,0)
    queue.extend(((x-1,y),(x+1,y),(x,y-1),(x,y+1)))
for size in (16,24,32,48,64,128,256,512):
    im.resize((size,size),Image.Resampling.LANCZOS).save(folder/f'barnyard-linux-{size}.png',optimize=True)
bitmap=im.resize((256,256),Image.Resampling.LANCZOS)
raw=bitmap.transpose(Image.Transpose.FLIP_TOP_BOTTOM).tobytes('raw','BGRA')
w,h=bitmap.size
header=struct.pack('<IiiHHIIiiII',108,w,h,1,32,3,len(raw),2835,2835,0,0)
header+=struct.pack('<IIIII',0x00ff0000,0x0000ff00,0x000000ff,0xff000000,0x73524742)
header+=bytes(36)+struct.pack('<III',0,0,0)
assert len(header)==108
(folder/'barnyard-linux.bmp').write_bytes(struct.pack('<2sIHHI',b'BM',14+len(header)+len(raw),0,0,14+len(header))+header+raw)
print('Created eight PNG icon sizes and 256px alpha BMPv4, without touching original game artwork.')

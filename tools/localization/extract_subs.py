from pathlib import Path
import struct,json,argparse,os
os.umask(0o077)
p=argparse.ArgumentParser(description="Extract private TRB subtitles; never distribute output")
p.add_argument("sample",type=Path);p.add_argument("output",type=Path)
a=p.parse_args()
out=[]
for path in sorted((a.sample/'Data/Subs/GBR').glob('*.trb')):
 b=path.read_bytes();chunks={};at=12
 while at+8<=len(b):
  n=struct.unpack_from('<I',b,at+4)[0];chunks[b[at:at+4]]=b[at+8:at+8+n];at+=8+((n+3)&~3)
 h=chunks[b'HDRX'];nsec=struct.unpack_from('<I',h,4)[0];sizes=[struct.unpack_from('<I',h,12+16*i)[0] for i in range(nsec)];sections=[];at=0
 for size in sizes:sections.append(chunks[b'SECT'][at:at+size]);at+=size
 r=chunks[b'RELC'];relocs={}
 for i in range(struct.unpack_from('<I',r)[0]):
  src,dst,offset=struct.unpack_from('<HHI',r,4+8*i);relocs[src,offset]=dst
 sy=chunks[b'SYMB'];ns=struct.unpack_from('<I',sy)[0];names=4+ns*12;found=0
 for i in range(ns):
  sec,no,pad,ha,off=struct.unpack_from('<HHHhI',sy,4+12*i);end=sy.index(0,names+no);name=sy[names+no:end].decode()
  if name=='SubtitleVersion':continue
  n=struct.unpack_from('<i',sections[sec],off)[0];assert 0<=n<100
  for j in range(n):
   loc=off+4+12*j;start,end,ptr=struct.unpack_from('<iiI',sections[sec],loc);dst=relocs[sec,loc+8];s=sections[dst];stop=ptr
   while s[stop:stop+2]!=b'\0\0':stop+=2;assert stop+2<=len(s)
   text=s[ptr:stop].decode('utf-16le');out.append({'file':path.name,'symbol':name,'line':j,'text':text});found+=1
 print(path.name,'lines',found)
a.output.write_text(json.dumps(out,ensure_ascii=False,indent=2))
print('TOTAL',len(out),'chars',sum(len(r['text']) for r in out))
# Do not print original dialogue into distributable logs.

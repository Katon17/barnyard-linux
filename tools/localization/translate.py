from pathlib import Path
import json,re,collections,hashlib,os
os.umask(0o077)
import ctranslate2,sentencepiece as sp
import argparse
parser=argparse.ArgumentParser(description='Offline private catalog -> hash-keyed Indonesian overlay')
parser.add_argument('--work-dir',type=Path,required=True,help='Private catalogs, opus/ and ct2/ models, checkpoints')
parser.add_argument('--project',type=Path,default=Path(__file__).resolve().parents[2])
args=parser.parse_args()
base=args.work_dir.resolve()
project=args.project.resolve()
locale=json.loads((base/'source-locale.json').read_text());subs=json.loads((base/'source-subtitles.json').read_text());strings=sorted(set(locale+[r['text'] for r in subs]))
pattern=re.compile(r'%c_[A-Za-z0-9_]+\[[^\]]+\]|%\{[^}]+\}|%player_[A-Za-z0-9_]+|%%|%(?:[-+0 #]*\d*(?:\.\d+)?[csdifuxXgGeE])|%\d+|%|\r\n|\n|\r')
manual={'Sound':'Suara','Controls':'Kontrol','Video':'Video','Back':'Kembali','Cost:':'Harga:','Delete':'Hapus','More':'Lainnya','Progress:':'Progres:','This Feature Is Not Available':'Fitur Ini Tidak Tersedia','Go To':'Pergi Ke','Load Game':'Muat Permainan','New Game':'Permainan Baru','Screen Resolution':'Resolusi Layar','Texture Quality':'Kualitas Tekstur','Terrain Detail':'Detail Medan','Terrain Quality':'Kualitas Medan','Model Quality':'Kualitas Model','Effects':'Efek','Grass':'Rumput','Draw Distance':'Jarak Pandang','Reflective Water':'Pantulan Air','Accept Changes':'Terapkan Perubahan','Options':'Pengaturan','Settings':'Pengaturan','Save Game':'Simpan Permainan','Continue':'Lanjutkan','Quit':'Keluar','Yes':'Ya','No':'Tidak','Cancel':'Batal','Confirm':'Konfirmasi','On':'Aktif','Off':'Nonaktif','Loading...':'Memuat...','Saving...':'Menyimpan...','Music':'Musik','Language':'Bahasa','Paused':'Dijeda','Resume':'Lanjutkan','Return To Game':'Kembali ke Permainan','Main Menu':'Menu Utama','Easy':'Mudah','Normal':'Normal','Hard':'Sulit','Game Over':'Permainan Berakhir'}
manual.update(json.loads((project/'tools/localization/menu-glossary.json').read_text()))
proper={'Otis','Ben','Bessy','Hanna','Pip','Peck','Duke','Freddy','Root','Twink','Dag','THQ','Barnyard','FMOD','Bink'}
segments={}
def pieces(text):
 out=[];at=0
 for m in pattern.finditer(text):
  out.extend([(False,text[at:m.start()]),(True,m.group())]);at=m.end()
 out.append((False,text[at:]));return out
for text in strings:
 for protected,part in pieces(text):
  clean=part.strip()
  if protected or not re.search('[A-Za-z]',clean) or clean in manual or clean in proper:continue
  if len(clean)<=2 or (clean.startswith('"') and clean.endswith('"')):continue
  segments[clean]=None
source=sp.SentencePieceProcessor(model_file=str(base/'opus/source.spm'));target=sp.SentencePieceProcessor(model_file=str(base/'opus/target.spm'))
t=ctranslate2.Translator(str(base/'ct2'),device='cpu',compute_type='int8',inter_threads=1,intra_threads=2)
keys=sorted(segments,key=lambda s:len(s));print('Unique strings',len(strings),'MT segments',len(keys),flush=True)
checkpoint=base/'segments-id.json'
if checkpoint.exists():segments.update(json.loads(checkpoint.read_text()))
remaining=[s for s in keys if segments[s] is None]
for start in range(0,len(remaining),32):
 batch=remaining[start:start+32]
 results=t.translate_batch([source.encode(s,out_type=str) for s in batch],beam_size=4,max_decoding_length=192)
 for src,res in zip(batch,results):segments[src]=target.decode(res.hypotheses[0])
 if start%128==0:
  checkpoint.write_text(json.dumps({k:v for k,v in segments.items() if v is not None},ensure_ascii=False));print('Translated',start+len(batch),'/',len(remaining),flush=True)
checkpoint.write_text(json.dumps(segments,ensure_ascii=False))
translations={}
for text in strings:
 out=''
 for protected,part in pieces(text):
  clean=part.strip()
  if protected or not clean:out+=part;continue
  result=manual.get(clean,segments.get(clean) or clean)
  result=result.replace('\u2019',"'").replace('\u2018',"'").replace('\u201c','"').replace('\u201d','"').replace('\u2013','-').replace('\u2014','-')
  piece=part[:len(part)-len(part.lstrip())]+result+part[len(part.rstrip()):]
  # A model may drop a leading full stop after %player_name; never merge tokens.
  if out and re.search(r'%player_[A-Za-z0-9_]+$',out) and piece and piece[0].isalnum():piece=' '+piece
  out+=piece
 assert out.count('\n')==text.count('\n') and out.count('%')==text.count('%'),repr(text)
 translations[text]=out
(base/'translations-private.json').write_text(json.dumps(translations,ensure_ascii=False,indent=2))
def fnv(text):
 h=14695981039346656037
 for b in text.encode('utf-16le'):h=((h^b)*1099511628211)&0xffffffffffffffff
 return h
rows=sorted((fnv(s),v) for s,v in translations.items() if s)
assert len({h for h,v in rows})==len(rows),'hash collision'
out=(project/'src/Platform/Linux/IndonesianData.inc')
out.write_text('// Fan translation draft: local OPUS-MT en-id + reviewed menu terminology.\n// Key is FNV-1a of original UTF-16LE; original locale/TRB assets are NOT bundled.\n'+''.join('{0x%016xULL, u%s},\n'%(h,json.dumps(v,ensure_ascii=True)) for h,v in rows))
summary={'locale_entries':len(locale),'subtitle_lines':len(subs),'unique_source_texts':len(strings),'nonempty_translation_records':len(rows),'placeholder_and_newline_preservation':True,'engine':'Helsinki-NLP OPUS-MT en-id 2019-12-18, CTranslate2 int8 CPU, local only','quality':'machine-translated draft, menu terms manually reviewed; not fully context-reviewed','physical_game_language_switch_tested':False}
(project/'reports/localization-coverage.json').write_text(json.dumps(summary,indent=2)+'\n')
print('DONE',summary,flush=True)

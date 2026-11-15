"""Numeric Music FSB metadata diagnostic; no native playback claim."""
from pathlib import Path
import io
import struct
import sys
import tempfile
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'launcher'))
from native_test import log_music_metadata
with tempfile.TemporaryDirectory() as temp:
    root=Path(temp)
    path=root/'dAtA/sOuNd/FsB/mUsIc.FsB'
    path.parent.mkdir(parents=True)
    h=bytearray(24+80+8)
    h[:4]=b'FSB3'
    struct.pack_into('<5I',h,4,2,88,6,0x30001,2)
    struct.pack_into('<H',h,24,80)
    struct.pack_into('<6I',h,56,1,2,0,0,0x10,44100)
    struct.pack_into('<HhHH',h,80,255,-1,128,1)
    struct.pack_into('<II',h,104,2,4)
    path.write_bytes(h+b'PRIVATE-AUDIO-PAYLOAD-NOT-FOR-LOG')
    output=io.StringIO()
    log_music_metadata(root,output)
    text=output.getvalue()
    assert 'samples=2' in text and 'Music sample 1: frames=2 encoded_bytes=4' in text
    assert 'channels=1 volume=255 pan=-1' in text
    assert 'PRIVATE-AUDIO' not in text
    path.write_bytes(h[:30])
    output=io.StringIO();log_music_metadata(root,output)
    assert 'diagnostic failed' in output.getvalue()
print('PASS numeric FSB metadata: case-resolved path, basic headers, bounded reads, no payload export, malformed input logged')

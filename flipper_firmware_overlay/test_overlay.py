#!/usr/bin/env python3
import hashlib
import json
import tempfile
import subprocess
import sys
from pathlib import Path
from PIL import Image
ME=Path(__file__).resolve().parent
SCRIPT=ME/'apply_theme.py'
with tempfile.TemporaryDirectory() as td:
 p=Path(td)/'fw';(p/'.git').mkdir(parents=True)
 for name,count in [('Sub1ghz_14',6),('NFC_14',4)]:
  d=p/'assets/icons/MainMenu'/name;d.mkdir(parents=True)
  for k in range(1,count+1):Image.new('1',(14,14),0).save(d/f'frame_{k:02d}.png')
 d=p/'assets/dolphin/internal/L1_Tv_128x47';d.mkdir(parents=True)
 for k in range(8):Image.new('1',(128,47),1).save(d/f'frame_{k}.png')
 pre={str(q.relative_to(p)):hashlib.sha256(q.read_bytes()).hexdigest() for q in p.rglob('*.png')}
 base=[sys.executable,str(SCRIPT),'--firmware',str(p)]
 subprocess.run(base+['--dry-run'],check=True,capture_output=True)
 assert not (p/'.inspector918_asset_backup').exists()
 subprocess.run(base+['--apply'],check=True,capture_output=True)
 mp=p/'.inspector918_asset_backup/manifest.json';assert mp.exists()
 manifest=json.loads(mp.read_text());assert len(manifest['targets'])==18
 assert any(hashlib.sha256((p/rel).read_bytes()).hexdigest()!=digest for rel,digest in pre.items())
 # A second apply must refuse, preserving the first clean backup.
 second=subprocess.run(base+['--apply'],capture_output=True)
 assert second.returncode!=0
 # Tampering blocks restoration and preserves original saved backups.
 target=p/manifest['targets'][0]['path'];save=target.read_bytes();target.write_bytes(b'bad')
 assert subprocess.run(base+['--restore'],capture_output=True).returncode!=0
 target.write_bytes(save)
 subprocess.run(base+['--restore'],check=True,capture_output=True)
 assert not (p/'.inspector918_asset_backup').exists()
 after={rel:hashlib.sha256((p/rel).read_bytes()).hexdigest() for rel in pre}
 assert after==pre
print('PASS: overlay dry-run, apply, precise asset backup, tamper rejection, restore')

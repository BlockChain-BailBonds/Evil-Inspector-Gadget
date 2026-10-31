#!/usr/bin/env python3
"""Backed-up, checksum-gated asset-only Flipper firmware overlay.

This edits no C sources, radio protocols, saved data or device firmware. To install,
compile the patched firmware yourself against its matching SDK and flash manually.
"""
import argparse
import hashlib
import io
import json
import os
from pathlib import Path
import shutil
import sys
from PIL import Image

CURRENT_DIR = Path(__file__).resolve().parent
THEME = CURRENT_DIR / "assets"
BACKUP_NAME = ".inspector918_asset_backup"
MANIFEST = "manifest.json"

def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()

def png_binary(img: Image.Image, size: tuple[int, int]) -> bytes:
    img = img.convert("1").resize(size, Image.Resampling.NEAREST)
    buf = io.BytesIO()
    img.save(buf, format="PNG", optimize=True)
    return buf.getvalue()

def collect(repo: Path) -> list[tuple[str,bytes]]:
    found=[]
    main=repo/"assets/icons/MainMenu"
    glyph=THEME/"inspector_icons"
    if not main.is_dir():
        raise RuntimeError(f"Not a supported Flipper firmware checkout: {main}")
    for source in sorted(glyph.glob("*.png")):
        directory=main/source.stem
        if not directory.is_dir():
            continue
        src=Image.open(source)
        for target in sorted(directory.glob("frame_*.png")):
            with Image.open(target) as old_im:
                size=old_im.size
            if size[0]<=0 or size[1]<=0 or size[0]>128 or size[1]>64:
                raise RuntimeError(f"Unexpected icon size at {target}: {size}")
            found.append((str(target.relative_to(repo)), png_binary(src,size)))
    # Replace ONLY the existing internal animation frames; preserve its meta and
    # distribution manifest to avoid perturbing the idle animation scheduler.
    animation=repo/"assets/dolphin/internal/L1_Tv_128x47"
    if animation.is_dir():
        for n in range(8):
            target=animation/f"frame_{n}.png"
            source=THEME/"inspector_dolphin"/f"frame_{n}.png"
            if not target.is_file() or not source.is_file():
                raise RuntimeError(f"Expected existing animation frame missing: {target}")
            with Image.open(target) as old_im:
                size=old_im.size
            if size!=(128,47):
                raise RuntimeError(f"Unexpected source animation dimensions: {target} -> {size}")
            found.append((str(target.relative_to(repo)), png_binary(Image.open(source),size)))
    if not found:
        raise RuntimeError("No compatible stock icon/animation source images were located; no changes made")
    return found

def apply(repo:Path, dry_run:bool) -> None:
    targets=collect(repo)
    backup=repo/BACKUP_NAME
    manifest_path=backup/MANIFEST
    if backup.exists():
        raise RuntimeError(f"Backup already exists: {backup}; restore previous theme before reapplying")
    entries=[]
    for rel,newbytes in targets:
        before=(repo/rel).read_bytes()
        entries.append(dict(path=rel,sha_before=sha(before),sha_after=sha(newbytes),size_before=len(before)))
    print(f"Theme targets: {len(entries)} files; dry_run={dry_run}")
    for e in entries:
        print("  "+e["path"])
    if dry_run:
        return
    backup.mkdir()
    try:
        for item in entries:
            path=Path(item['path'])
            dest=backup/path
            dest.parent.mkdir(parents=True,exist_ok=True)
            dest.write_bytes((repo/path).read_bytes())
        manifest_path.write_text(json.dumps({'schema':1,'brand':'918 Technologies Evil Inspector Gadget','targets':entries},indent=2)+'\n')
        # All originals persisted and verified before first write.
        for e in entries:
            if sha((backup/e['path']).read_bytes())!=e['sha_before']:
                raise RuntimeError("Backup verification failed; refusing to change asset tree")
        for (rel,newbytes),entry in zip(targets,entries):
            if sha((repo/rel).read_bytes())!=entry['sha_before']:
                raise RuntimeError(f"Firmware tree changed during apply: {rel}")
            (repo/rel).write_bytes(newbytes)
        print("ASSET OVERLAY INSTALLED; source backup in", backup)
    except BaseException:
        for item in entries:
            rel=item['path']
            original=backup/rel
            if original.is_file() and sha(original.read_bytes())==item['sha_before']:
                (repo/rel).write_bytes(original.read_bytes())
        print("Overlay failed; backed-up originals restored where available",file=sys.stderr)
        raise

def restore(repo:Path) -> None:
    backup=repo/BACKUP_NAME
    mp=backup/MANIFEST
    if not mp.is_file():raise RuntimeError(f"No overlay manifest at {mp}")
    obj=json.loads(mp.read_text())
    if obj.get('schema')!=1:raise RuntimeError('Unknown backup manifest schema')
    targets=obj['targets']
    # Fail closed if any asset was modified by another person or generator.
    for e in targets:
        dest=repo/e['path']; old=backup/e['path']
        if not old.is_file() or sha(old.read_bytes())!=e['sha_before']:
            raise RuntimeError(f"Original backup missing or damaged: {e['path']}")
        if not dest.is_file() or sha(dest.read_bytes())!=e['sha_after']:
            raise RuntimeError(f"Edited theme asset does not match manifest; no restoration: {e['path']}")
    for e in targets:
        dest=repo/e['path'];dest.write_bytes((backup/e['path']).read_bytes())
    for e in targets:
        if sha((repo/e['path']).read_bytes())!=e['sha_before']:
            raise RuntimeError(f"Original restore verification failed: {e['path']}")
    shutil.rmtree(backup)
    print(f"RESTORED {len(targets)} exact original firmware assets")

def main():
    ap=argparse.ArgumentParser(description='Backed-up Evil Inspector Gadget asset-only Flipper firmware theme')
    ap.add_argument('--firmware',type=Path,required=True,help='Flipper firmware source checkout root')
    group=ap.add_mutually_exclusive_group(required=True)
    group.add_argument('--apply',action='store_true')
    group.add_argument('--dry-run',action='store_true')
    group.add_argument('--restore',action='store_true')
    args=ap.parse_args()
    repo=args.firmware.resolve()
    if not (repo/'.git').exists() or not (repo/'assets/icons/MainMenu').is_dir():
        raise SystemExit('STOP: expected the root of a checked-out Flipper firmware repository')
    if args.restore:restore(repo)
    else:apply(repo,args.dry_run)
if __name__=='__main__':
    main()

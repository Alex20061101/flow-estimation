#!/usr/bin/env python3
#To dry run:
#python3 scripts/rename_cam0506.py --root Data/RGB
#To apply changes:
#python3 scripts/rename_cam0506.py --root Data/RGB --apply --overwrite

"""
Rename files whose basename contains 'Cam0506_' to 'Cam05_'.

Usage:
  python3 scripts/rename_cam0506.py [--root ROOT] [--apply] [--overwrite]

Defaults to searching recursively under `Data/RGB`.
By default the script performs a dry-run and prints planned renames. Use `--apply` to perform renames.
Use `--overwrite` with `--apply` to overwrite existing targets.
"""
import argparse
import os
import sys
from pathlib import Path


def find_cam0506_files(root: Path):
    for p in root.rglob('*'):
        if p.is_file() and 'Cam0506_' in p.name:
            yield p


def main():
    ap = argparse.ArgumentParser(description='Rename Cam0506_ files to Cam05_')
    ap.add_argument('--root', '-r', default='Data/RGB', help='Root directory to search (default Data/RGB)')
    ap.add_argument('--apply', action='store_true', help='Perform the renames (default: dry-run)')
    ap.add_argument('--overwrite', action='store_true', help='Allow overwriting target files when --apply')
    args = ap.parse_args()

    root = Path(args.root)
    if not root.exists():
        print(f'Root not found: {root}', file=sys.stderr)
        sys.exit(2)

    files = list(find_cam0506_files(root))
    if not files:
        print('No files found containing "Cam0506_" under', root)
        return

    planned = []
    for f in files:
        new_name = f.name.replace('Cam0506_', 'Cam05_')
        target = f.with_name(new_name)
        planned.append((f, target))

    print('Planned renames:')
    for src, dst in planned:
        exists = dst.exists()
        note = ' (target exists)' if exists else ''
        print(f'  {src} -> {dst}{note}')

    if not args.apply:
        print('\nDry-run complete. Re-run with --apply to perform the renames.')
        return

    # perform renames
    moved = 0
    skipped = 0
    for src, dst in planned:
        if dst.exists() and not args.overwrite:
            print(f'Skipping (exists): {dst}')
            skipped += 1
            continue
        try:
            # ensure parent dir exists
            dst.parent.mkdir(parents=True, exist_ok=True)
            if dst.exists() and args.overwrite:
                dst.unlink()
            src.rename(dst)
            print(f'Renamed: {src} -> {dst}')
            moved += 1
        except Exception as e:
            print(f'Failed to rename {src} -> {dst}: {e}', file=sys.stderr)

    print(f'Completed: moved={moved}, skipped={skipped}, total={len(planned)}')


if __name__ == '__main__':
    main()

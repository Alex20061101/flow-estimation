#!/usr/bin/env python3
#to Dry run:
#python3 scripts/organize_by_month.py --source Data/x --dry-run
# or
# python3 scripts/organize_by_month.py --source Data/Thermal --collect --dry-run
# to move the files add --move
# to move them to parent folder first and then organize
# python3 scripts/organize_by_month.py --source Data/Thermal --collect --move
"""
Organize files in a directory by month based on YYYYMMDD in filenames.

Usage:
  python3 scripts/organize_by_month.py --source Data/Thermal --dry-run
  python3 scripts/organize_by_month.py --source Data/Thermal --move

The script looks for a 8-digit date fragment (YYYYMMDD) in each filename
and moves files into subfolders named YYYY-MM (e.g. 2025-03).
"""
import argparse
import re
from pathlib import Path
import shutil
import sys
import datetime


DATE_RE = re.compile(r"(\d{4})(\d{2})(\d{2})")


def find_date_part(name: str):
    m = DATE_RE.search(name)
    if not m:
        return None
    year, month, day = m.group(1), m.group(2), m.group(3)
    return f"{year}-{month}"


def organize(source: Path, move: bool = False):
    if not source.exists():
        print(f"Source path does not exist: {source}")
        return 1
    if not source.is_dir():
        print(f"Source is not a directory: {source}")
        return 1

    files = sorted([p for p in source.iterdir() if p.is_file()])
    if not files:
        print(f"No files found in {source}")
        return 0

    moves = []
    for f in files:
        month = find_date_part(f.name)
        if not month:
            # skip files without date fragment
            continue
        dest_dir = source / month
        dest = dest_dir / f.name
        moves.append((f, dest_dir, dest))

    if not moves:
        print("No files with a YYYYMMDD date pattern found to organize.")
        return 0

    for src, dest_dir, dest in moves:
        if move:
            dest_dir.mkdir(parents=True, exist_ok=True)
            try:
                shutil.move(str(src), str(dest))
                print(f"Moved: {src} -> {dest}")
            except Exception as e:
                print(f"Error moving {src}: {e}")
        else:
            print(f"Would move: {src} -> {dest}")

    if not move:
        print("Dry-run complete. Rerun with --move to apply changes.")

    return 0


def main(argv=None):
    parser = argparse.ArgumentParser(description="Organize files by month based on filenames")
    parser.add_argument("--source", "-s", default="Data/Thermal", help="Source directory to organize (ignored if --month is used)")
    parser.add_argument("--month", "-m", help="Month as two digits (01-12). When provided, script targets Data/Thermal/YYYY-MM")
    parser.add_argument("--year", "-y", default="2025", help="Year (YYYY) used with --month. Defaults to 2025")
    parser.add_argument("--collect", "-c", action="store_true",
                        help="Collect files from Cam01..Cam10 subfolders into the source folder before organizing")
    group = parser.add_mutually_exclusive_group()
    group.add_argument("--dry-run", dest="move", action="store_false", help="Show planned moves (default)")
    group.add_argument("--move", dest="move", action="store_true", help="Actually move files")
    parser.set_defaults(move=False)

    args = parser.parse_args(argv)
    # If --month provided, construct path Data/Thermal/YYYY-MM and use that
    if args.month:
        month = args.month
        # normalize single-digit months
        if len(month) == 1:
            month = '0' + month
        if not re.match(r'^(0[1-9]|1[0-2])$', month):
            print(f"Invalid month: {args.month}. Expect two digits 01-12.")
            return 2
        year = args.year
        source = Path("Data") / "Thermal" / f"{year}-{month}"
    else:
        source = Path(args.source)

    # If requested, first collect files from Cam01..Cam10 subfolders into `source`.
    if args.collect:
        def collect_cam_folders(source: Path, move: bool = False):
            cam_re = re.compile(r"^Cam(0[1-9]|10)$", re.IGNORECASE)
            found = False
            for p in sorted([d for d in source.iterdir() if d.is_dir()], key=lambda x: x.name):
                if cam_re.match(p.name):
                    found = True
                    files = sorted([f for f in p.iterdir() if f.is_file()])
                    if not files:
                        continue
                    for f in files:
                        dest = source / f.name
                        if move:
                            try:
                                if dest.exists():
                                    print(f"Skipping (exists): {f} -> {dest}")
                                    continue
                                shutil.move(str(f), str(dest))
                                print(f"Moved: {f} -> {dest}")
                            except Exception as e:
                                print(f"Error moving {f}: {e}")
                        else:
                            print(f"Would move: {f} -> {dest}")
                    if move:
                        try:
                            next(p.iterdir())
                        except StopIteration:
                            try:
                                p.rmdir()
                                print(f"Removed empty directory: {p}")
                            except Exception:
                                pass
            if not found:
                print("No Cam01..Cam10 subfolders found to collect.")

        collect_cam_folders(source, move=args.move)

    return organize(source, move=args.move)


if __name__ == "__main__":
    sys.exit(main())

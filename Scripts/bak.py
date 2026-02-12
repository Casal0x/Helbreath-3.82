#!/usr/bin/env python3
"""Centralized .bak file manager with versioned checkpoints.

Uses .bak_<guid> files to track layered changes. Each `guard` creates
a new checkpoint; `revert` peels back one layer; `commit` accepts all.

Commands:
    guard <files...>    Create a new versioned checkpoint
    status              List all checkpoints with dirty/clean status
    revert              Peel back one checkpoint layer (most recent)
    revert --all        Revert to original state (oldest checkpoint)
    commit              Delete all .bak* files (accept current state)
"""

import argparse
import filecmp
import re
import shutil
import sys
import uuid
from collections import defaultdict
from datetime import datetime
from pathlib import Path

SOURCES = Path(r"Z:\Helbreath-3.82\Sources")
BAK_RE = re.compile(r"^(.+)\.bak_([a-f0-9]{8})$")


def find_all_baks() -> list[Path]:
    """Find all .bak_<guid> files under Sources/."""
    results = []
    for p in SOURCES.rglob("*.bak_*"):
        if p.is_file() and BAK_RE.match(p.name):
            results.append(p)
    return sorted(results)


def find_legacy_baks() -> list[Path]:
    """Find old-style .bak files (no guid) under Sources/."""
    results = []
    for p in SOURCES.rglob("*.bak"):
        if p.is_file() and p.suffix == ".bak" and not BAK_RE.match(p.name):
            results.append(p)
    return sorted(results)


def parse_bak(path: Path) -> tuple[Path, str] | None:
    """Parse .bak_<guid> path into (original_path, guid)."""
    m = BAK_RE.match(path.name)
    if m:
        return path.parent / m.group(1), m.group(2)
    return None


def get_checkpoints() -> list[tuple[str, float, list[Path]]]:
    """Return checkpoints as (guid, mtime, [bak_paths]) ordered oldest-first."""
    baks = find_all_baks()
    guid_files: dict[str, list[Path]] = defaultdict(list)
    guid_mtime: dict[str, float] = {}

    for bak in baks:
        parsed = parse_bak(bak)
        if parsed:
            _, guid = parsed
            guid_files[guid].append(bak)
            mt = bak.stat().st_mtime
            if guid not in guid_mtime or mt > guid_mtime[guid]:
                guid_mtime[guid] = mt

    return [
        (g, guid_mtime[g], guid_files[g])
        for g in sorted(guid_files, key=lambda g: guid_mtime[g])
    ]


def rel(path: Path) -> str:
    """Display path relative to Sources/."""
    try:
        return str(path.relative_to(SOURCES))
    except ValueError:
        return str(path)


def resolve_path(raw: str) -> Path:
    """Resolve user-provided path to absolute."""
    p = Path(raw)
    if p.is_absolute():
        return p
    cwd_path = Path.cwd() / p
    if cwd_path.exists():
        return cwd_path.resolve()
    src_path = SOURCES / p
    if src_path.exists():
        return src_path.resolve()
    return cwd_path.resolve()


def cmd_guard(args: argparse.Namespace) -> int:
    """Create a new versioned checkpoint for the listed files."""
    files = [resolve_path(f) for f in args.files]

    for f in files:
        if not f.exists():
            print(f"ERROR: {rel(f)} does not exist.")
            return 1

    guid = uuid.uuid4().hex[:8]

    for f in files:
        bak = f.parent / f"{f.name}.bak_{guid}"
        shutil.copy2(str(f), str(bak))
        print(f"  Guarded: {rel(f)}")

    print(f"\nCheckpoint {guid} created ({len(files)} file(s)).")
    return 0


def cmd_status(args: argparse.Namespace) -> int:
    """List all checkpoints with dirty/clean status. Exit 1 if any exist."""
    checkpoints = get_checkpoints()
    legacy = find_legacy_baks()

    if not checkpoints and not legacy:
        print("No checkpoints in Sources/")
        return 0

    if checkpoints:
        total = sum(len(files) for _, _, files in checkpoints)
        print(f"{len(checkpoints)} checkpoint(s), {total} file(s) in Sources/:\n")

        for i, (guid, mtime, bak_paths) in enumerate(checkpoints):
            ts = datetime.fromtimestamp(mtime).strftime("%Y-%m-%d %H:%M:%S")
            label = ""
            if len(checkpoints) > 1:
                if i == 0:
                    label = "  <- original"
                elif i == len(checkpoints) - 1:
                    label = "  <- most recent"
            print(f"  [{guid}]  {ts}  ({len(bak_paths)} file(s)){label}")

            for bak in sorted(bak_paths):
                parsed = parse_bak(bak)
                if parsed:
                    orig, _ = parsed
                    if not orig.exists():
                        status = "MISSING"
                    elif filecmp.cmp(str(bak), str(orig), shallow=False):
                        status = "clean"
                    else:
                        status = "dirty"
                    print(f"    [{status:^8s}]  {rel(orig)}")
            print()

    if legacy:
        print(f"WARNING: {len(legacy)} legacy .bak file(s) (no guid):")
        for p in legacy:
            print(f"  {rel(p)}")
        print("Run 'bak.py commit' to clean these up.\n")

    return 1  # .bak files exist


def cmd_revert(args: argparse.Namespace) -> int:
    """Peel back one checkpoint layer, or revert to original."""
    if args.all:
        return _revert_all()
    return _revert_latest()


def _revert_latest() -> int:
    """Restore from the most recent checkpoint, then delete it."""
    checkpoints = get_checkpoints()
    if not checkpoints:
        print("Nothing to revert — no checkpoints found.")
        return 0

    guid, _, bak_paths = checkpoints[-1]
    print(f"Reverting checkpoint {guid}...")

    for bak in sorted(bak_paths):
        parsed = parse_bak(bak)
        if parsed:
            orig, _ = parsed
            shutil.copy2(str(bak), str(orig))
            bak.unlink()
            print(f"  Restored: {rel(orig)}")

    remaining = len(checkpoints) - 1
    print(f"\n{len(bak_paths)} file(s) restored. {remaining} checkpoint(s) remaining.")
    return 0


def _revert_all() -> int:
    """Restore every file to its oldest checkpoint, delete all .bak files."""
    checkpoints = get_checkpoints()
    if not checkpoints:
        print("Nothing to revert — no checkpoints found.")
        return 0

    # For each file, find its oldest .bak (the original state)
    oldest_for_file: dict[Path, Path] = {}
    all_baks: list[Path] = []

    for _, _, bak_paths in checkpoints:
        for bak in bak_paths:
            parsed = parse_bak(bak)
            if parsed:
                orig, _ = parsed
                if orig not in oldest_for_file:
                    oldest_for_file[orig] = bak
            all_baks.append(bak)

    print("Reverting to original state...")
    for orig in sorted(oldest_for_file, key=lambda p: rel(p)):
        bak = oldest_for_file[orig]
        shutil.copy2(str(bak), str(orig))
        print(f"  Restored: {rel(orig)}")

    for bak in all_baks:
        if bak.exists():
            bak.unlink()

    print(f"\n{len(oldest_for_file)} file(s) restored to original. "
          f"All {len(checkpoints)} checkpoint(s) deleted.")
    return 0


def cmd_commit(args: argparse.Namespace) -> int:
    """Delete all .bak* files (accept current state)."""
    baks = find_all_baks()
    legacy = find_legacy_baks()
    all_files = baks + legacy

    if not all_files:
        print("Nothing to commit — no .bak files found.")
        return 0

    checkpoints = get_checkpoints()
    for f in all_files:
        f.unlink()

    parts = []
    if baks:
        parts.append(f"{len(baks)} checkpoint file(s) across "
                      f"{len(checkpoints)} checkpoint(s)")
    if legacy:
        parts.append(f"{len(legacy)} legacy .bak file(s)")
    print(f"Deleted {', '.join(parts)}. Changes accepted.")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(
        prog="bak.py",
        description="Versioned .bak file manager for Sources/",
    )
    sub = parser.add_subparsers(dest="command", required=True)

    p_guard = sub.add_parser("guard", help="Create a new versioned checkpoint")
    p_guard.add_argument("files", nargs="+", help="Files to checkpoint")

    sub.add_parser("status", help="List all checkpoints with status")

    p_revert = sub.add_parser("revert", help="Peel back one checkpoint layer")
    p_revert.add_argument("--all", action="store_true",
                          help="Revert to original state (oldest checkpoint)")

    sub.add_parser("commit", help="Delete all .bak* files (accept changes)")

    args = parser.parse_args()

    handlers = {
        "guard": cmd_guard,
        "status": cmd_status,
        "revert": cmd_revert,
        "commit": cmd_commit,
    }
    return handlers[args.command](args)


if __name__ == "__main__":
    sys.exit(main())

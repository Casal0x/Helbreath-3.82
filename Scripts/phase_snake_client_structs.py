#!/usr/bin/env python3
"""Phase 4: Client struct member renames
Target structs: DialogBoxInfo (29 members), weather_particle (4), EventEntry (3)
Total: 36 renames across ~55 client files.
Mode 2 justified: DialogBoxInfo members used across 42+ DialogBox_*.cpp files.
"""

import re
import os
import sys

BASE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CLIENT_DIR = os.path.join(BASE, "Sources", "Client")

# ═══════════════════════════════════════════════════════════════════════════
# 1. HEADER DEFINITION REPLACEMENTS (exact string match within line)
#    These rename the struct member declarations in the header files.
# ═══════════════════════════════════════════════════════════════════════════
HEADER_DEFS = {
    os.path.join(CLIENT_DIR, "DialogBoxInfo.h"): [
        ("int sV1, sV2, sV3, sV4, sV5, sV6, sV7, sV8, sV9, sV10, sV11, sV12, sV13, sV14;",
         "int v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14;"),
        ("uint32_t dwV1, dwV2, dwT1;", "uint32_t dw_v1, dw_v2, dw_t1;"),
        ("bool bFlag;", "bool flag;"),
        ("short sX, sY;", "short x, y;"),
        ("short sSizeX, sSizeY;", "short size_x, size_y;"),
        ("short sView;", "short view;"),
        ("char cStr[32], cStr2[32], cStr3[32], cStr4[32];",
         "char str[32], str2[32], str3[32], str4[32];"),
        ("char cMode;", "char mode;"),
        ("bool bIsScrollSelected;", "bool is_scroll_selected;"),
    ],
    os.path.join(CLIENT_DIR, "WeatherManager.h"): [
        ("short sX = 0;", "short x = 0;"),
        ("short sY = 0;", "short y = 0;"),
        ("short sBX = 0;", "short bx = 0;"),
        ("char cStep = 0;", "char step = 0;"),
    ],
    os.path.join(CLIENT_DIR, "EventListManager.h"): [
        ("uint32_t dwTime = 0;", "uint32_t time = 0;"),
        ("char cColor = 0;", "char color = 0;"),
        ("std::string cTxt;", "std::string txt;"),
    ],
}

# ═══════════════════════════════════════════════════════════════════════════
# 2. SAFE RENAMES: Distinctive names using \b word-boundary replacement
#    These names are unique to the target structs — safe for global replace.
#    Applied to ALL .h and .cpp in Sources/Client/
# ═══════════════════════════════════════════════════════════════════════════
SAFE_RENAMES = [
    # DialogBoxInfo — sV* (longest first: sV10+ before sV1 to prevent prefix clobber)
    ("sV14", "v14"), ("sV13", "v13"), ("sV12", "v12"), ("sV11", "v11"), ("sV10", "v10"),
    ("sV9", "v9"), ("sV8", "v8"), ("sV7", "v7"), ("sV6", "v6"), ("sV5", "v5"),
    ("sV4", "v4"), ("sV3", "v3"), ("sV2", "v2"), ("sV1", "v1"),
    # DialogBoxInfo — dw* (distinctive DWORD values, no collision with sV*)
    ("dwV1", "dw_v1"), ("dwV2", "dw_v2"), ("dwT1", "dw_t1"),
    # DialogBoxInfo — distinctive compound names
    ("sSizeX", "size_x"), ("sSizeY", "size_y"),
    ("bIsScrollSelected", "is_scroll_selected"),
    # weather_particle — distinctive
    ("sBX", "bx"),
]

# ═══════════════════════════════════════════════════════════════════════════
# 3. CONTEXT RENAMES: Ambiguous short names using dot/arrow access pattern
#    Pattern: (?<=[.>])name\b  →  matches .name and ->name (> from ->)
#    This avoids renaming parameters/locals with the same name (Phase 9/10).
#    Applied to ALL .h and .cpp in Sources/Client/
# ═══════════════════════════════════════════════════════════════════════════
CONTEXT_RENAMES = [
    # Sort longest first to avoid prefix collision (cStr4 before cStr)
    ("cStr4", "str4"), ("cStr3", "str3"), ("cStr2", "str2"), ("cStr", "str"),
    ("dwTime", "time"),
    ("cColor", "color"),
    ("cMode", "mode"),
    ("bFlag", "flag"),
    ("sView", "view"),
    ("cStep", "step"),
    ("cTxt", "txt"),
    ("sX", "x"), ("sY", "y"),
]


def collect_files():
    """Collect all .h and .cpp files under Sources/Client/."""
    files = []
    for name in sorted(os.listdir(CLIENT_DIR)):
        if name.endswith((".h", ".cpp")):
            files.append(os.path.join(CLIENT_DIR, name))
    return files


def apply_header_defs(dry_run=False):
    """Apply exact string replacements to struct definitions in headers."""
    total = 0
    modified = []
    for fpath, replacements in HEADER_DEFS.items():
        with open(fpath, "r", encoding="utf-8") as f:
            content = f.read()
        original = content
        for old, new in replacements:
            if old in content:
                content = content.replace(old, new)
                total += 1
            else:
                print(f"  WARN: not found in {os.path.basename(fpath)}: {old[:60]}...")
        if content != original:
            if not dry_run:
                with open(fpath, "w", encoding="utf-8", newline="\n") as f:
                    f.write(content)
            modified.append(fpath)
            print(f"  DEF  {os.path.basename(fpath)}")
    return total, modified


def apply_renames(files, dry_run=False):
    """Apply SAFE and CONTEXT renames to all files."""
    # Pre-compile regexes
    safe_patterns = [(re.compile(r"\b" + re.escape(old) + r"\b"), new)
                     for old, new in SAFE_RENAMES]
    ctx_patterns = [(re.compile(r"(?<=[.>])" + re.escape(old) + r"\b"), new)
                    for old, new in CONTEXT_RENAMES]

    modified_files = []
    total_replacements = 0

    for fpath in files:
        with open(fpath, "r", encoding="utf-8") as f:
            lines = f.readlines()

        original_lines = lines[:]
        new_lines = []
        file_changes = 0

        for line in lines:
            original_line = line

            # Skip #include and #pragma lines
            stripped = line.lstrip()
            if stripped.startswith("#include") or stripped.startswith("#pragma"):
                new_lines.append(line)
                continue

            # Apply SAFE renames (word-boundary)
            for pattern, new in safe_patterns:
                line = pattern.sub(new, line)

            # Apply CONTEXT renames (dot/arrow access only)
            for pattern, new in ctx_patterns:
                line = pattern.sub(new, line)

            if line != original_line:
                file_changes += 1
            new_lines.append(line)

        if file_changes > 0:
            if not dry_run:
                with open(fpath, "w", encoding="utf-8", newline="\n") as f:
                    f.writelines(new_lines)
            total_replacements += file_changes
            modified_files.append(fpath)
            print(f"  MOD  {os.path.basename(fpath)} ({file_changes} lines)")

    return modified_files, total_replacements


def main():
    dry_run = "--dry-run" in sys.argv
    mode = "DRY RUN" if dry_run else "LIVE"
    print(f"Phase 4: Client struct member renames [{mode}]")
    print(f"  Base: {CLIENT_DIR}")
    print()

    files = collect_files()
    print(f"Scanning {len(files)} client files...")
    print()

    # Step 1: Header definitions
    print("-- Header definitions --")
    def_count, def_files = apply_header_defs(dry_run)
    print(f"  {def_count} definition replacements in {len(def_files)} files")
    print()

    # Step 2: Access sites (safe + context)
    print("-- Access site renames --")
    mod_files, rep_count = apply_renames(files, dry_run)
    print()

    # Summary
    all_modified = sorted(set(f for f in def_files + mod_files))
    print(f"Summary: {len(all_modified)} files modified, ~{def_count + rep_count} changes")

    if dry_run:
        print("\n  Files that would be modified:")
        for f in all_modified:
            print(f"    {os.path.relpath(f, BASE)}")

    return 0 if all_modified else 1


if __name__ == "__main__":
    sys.exit(main())

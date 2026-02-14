#!/usr/bin/env python3
"""
Phase D1: Client Windows Type Cleanup

Replaces unguarded Windows types with C++ standard equivalents:
  - DWORD -> uint32_t (33 occurrences in 8 files)
  - WORD  -> uint16_t (21 occurrences in 9 files)
  - Remove #include <windows.h> from 23 files
  - Remove #include <mmsystem.h> from Tile.h
  - Remove #include <io.h> from MapData.h
  - Add #include <cstdint> to CharInfo.h and Tile.h

Usage:
  python Scripts/phase_d1_client_win_types.py --dry-run   # Preview changes
  python Scripts/phase_d1_client_win_types.py --verify     # Check for collisions
  python Scripts/phase_d1_client_win_types.py              # Apply changes
"""

import re
import sys
import os

BASE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
SOURCES = os.path.join(BASE, "Sources")
OUTPUT_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "output")

# ============================================================
# FILE LISTS
# ============================================================

# Files where DWORD -> uint32_t
DWORD_FILES = [
    "Client/CharInfo.h",
    "Client/Tile.h",
    "Client/BuildItemManager.cpp",
    "Client/MapData.cpp",
    "Client/NetworkMessages_Combat.cpp",
    "Client/NetworkMessages_Bank.cpp",
    "Client/NetworkMessages_Items.cpp",
    "Client/NetworkMessages_Stats.cpp",
]

# Files where WORD -> uint16_t
WORD_FILES = [
    "Client/Tile.h",
    "Client/FishingManager.cpp",
    "Client/Game.cpp",
    "Client/MapData.cpp",
    "Client/NetworkMessages_Bank.cpp",
    "Client/NetworkMessages_Items.cpp",
    "Client/NetworkMessages_Map.cpp",
    "Client/NetworkMessages_Player.cpp",
    "Client/NetworkMessages_Skills.cpp",
]

# Files to remove #include <windows.h> from (all unguarded)
REMOVE_WINDOWS_H = [
    # Headers
    "Client/BuildItem.h",
    "Client/CharInfo.h",
    "Client/Effect.h",
    "Client/Misc.h",
    "Client/Player.h",
    "Client/Tile.h",
    "Client/TileSpr.h",
    # .cpp files
    "Client/CraftingManager.cpp",
    "Client/FishingManager.cpp",
    "Client/GuildManager.cpp",
    "Client/NetworkMessageManager.cpp",
    "Client/NetworkMessages_Admin.cpp",
    "Client/NetworkMessages_Agriculture.cpp",
    "Client/NetworkMessages_Angels.cpp",
    "Client/NetworkMessages_Combat.cpp",
    "Client/NetworkMessages_Crusade.cpp",
    "Client/NetworkMessages_Items.cpp",
    "Client/NetworkMessages_Map.cpp",
    "Client/NetworkMessages_Player.cpp",
    "Client/NetworkMessages_Skills.cpp",
    "Client/NetworkMessages_Stats.cpp",
    "Client/NetworkMessages_System.cpp",
    "Client/QuestManager.cpp",
]

# Files to add #include <cstdint> (headers that use uint32_t/uint16_t directly)
ADD_CSTDINT = [
    "Client/CharInfo.h",
    "Client/Tile.h",
]

# ============================================================
# HELPERS
# ============================================================

def read_file(path):
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        return f.readlines()

def write_file(path, lines):
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.writelines(lines)

def full_path(rel):
    return os.path.join(SOURCES, rel.replace("/", os.sep))

# ============================================================
# TRANSFORMS
# ============================================================

def replace_dword(lines, filename):
    """Replace \bDWORD\b -> uint32_t, skipping comments."""
    changes = []
    pat = re.compile(r'\bDWORD\b')
    for i, line in enumerate(lines):
        stripped = line.lstrip()
        # Skip pure comment lines
        if stripped.startswith("//"):
            continue
        new_line = pat.sub("uint32_t", line)
        if new_line != line:
            changes.append((i + 1, line.rstrip(), new_line.rstrip()))
            lines[i] = new_line
    return changes

def replace_word(lines, filename):
    """Replace \bWORD\b -> uint16_t, skipping comments and DWORD."""
    changes = []
    # \bWORD\b naturally won't match inside DWORD (D is a word char before W)
    pat = re.compile(r'\bWORD\b')
    for i, line in enumerate(lines):
        stripped = line.lstrip()
        if stripped.startswith("//"):
            continue
        new_line = pat.sub("uint16_t", line)
        if new_line != line:
            changes.append((i + 1, line.rstrip(), new_line.rstrip()))
            lines[i] = new_line
    return changes

def remove_include(lines, include_str):
    """Remove a specific #include line. Returns list of (line_num, old_line) removed."""
    changes = []
    new_lines = []
    for i, line in enumerate(lines):
        if include_str in line and line.lstrip().startswith("#include"):
            changes.append((i + 1, line.rstrip()))
        else:
            new_lines.append(line)
    return new_lines, changes

def add_cstdint(lines, filename):
    """Add #include <cstdint> after #pragma once if not already present."""
    # Check if already included
    for line in lines:
        if "<cstdint>" in line:
            return lines, []

    # Find #pragma once line and insert after it
    for i, line in enumerate(lines):
        if line.strip() == "#pragma once":
            # Insert after #pragma once (and any blank line after it)
            insert_pos = i + 1
            # Skip blank line after #pragma once if present
            if insert_pos < len(lines) and lines[insert_pos].strip() == "":
                insert_pos += 1
            # Find first #include block and insert before it, or after #pragma once
            # Actually, insert at the end of the existing include block
            # Find the last #include in the header section
            last_include = insert_pos
            for j in range(insert_pos, min(insert_pos + 20, len(lines))):
                if lines[j].strip().startswith("#include"):
                    last_include = j + 1
                elif lines[j].strip() == "":
                    continue
                else:
                    break
            lines.insert(last_include, "#include <cstdint>\n")
            return lines, [(last_include + 1, "#include <cstdint>")]
    return lines, []

# ============================================================
# VERIFY MODE
# ============================================================

def verify():
    """Check for potential collisions."""
    print("=== VERIFY MODE ===\n")
    issues = 0

    # Check that DWORD files don't have DWORD inside #ifdef _WIN32 blocks
    for rel in DWORD_FILES:
        path = full_path(rel)
        if not os.path.exists(path):
            print(f"  ERROR: File not found: {rel}")
            issues += 1
            continue
        lines = read_file(path)
        in_ifdef = 0
        for i, line in enumerate(lines):
            if re.match(r'\s*#\s*ifdef\s+_WIN32', line):
                in_ifdef += 1
            elif re.match(r'\s*#\s*endif', line) and in_ifdef > 0:
                in_ifdef -= 1
            if in_ifdef > 0 and re.search(r'\bDWORD\b', line):
                print(f"  WARNING: {rel}:{i+1} has DWORD inside #ifdef _WIN32 block")
                issues += 1

    # Check that WORD files don't have WORD inside #ifdef _WIN32 blocks
    for rel in WORD_FILES:
        path = full_path(rel)
        if not os.path.exists(path):
            print(f"  ERROR: File not found: {rel}")
            issues += 1
            continue
        lines = read_file(path)
        in_ifdef = 0
        for i, line in enumerate(lines):
            if re.match(r'\s*#\s*ifdef\s+_WIN32', line):
                in_ifdef += 1
            elif re.match(r'\s*#\s*endif', line) and in_ifdef > 0:
                in_ifdef -= 1
            if in_ifdef > 0 and re.search(r'\bWORD\b', line) and not line.lstrip().startswith("//"):
                print(f"  WARNING: {rel}:{i+1} has WORD inside #ifdef _WIN32 block")
                issues += 1

    # Check all target files exist
    all_files = set(DWORD_FILES + WORD_FILES + REMOVE_WINDOWS_H + ADD_CSTDINT)
    for rel in sorted(all_files):
        path = full_path(rel)
        if not os.path.exists(path):
            print(f"  ERROR: File not found: {rel}")
            issues += 1

    # Check for DWORD/WORD in Shared or SFMLEngine (collision risk)
    for subdir in ["Dependencies/Shared", "SFMLEngine"]:
        dirpath = os.path.join(SOURCES, subdir)
        for root, dirs, files in os.walk(dirpath):
            for fname in files:
                if not (fname.endswith(".h") or fname.endswith(".cpp")):
                    continue
                fpath = os.path.join(root, fname)
                for i, line in enumerate(read_file(fpath)):
                    if re.search(r'\bDWORD\b', line) and not line.lstrip().startswith("//"):
                        rel = os.path.relpath(fpath, SOURCES)
                        # Skip third-party (ASIO, json)
                        if "ASIO" in rel or "json" in rel:
                            continue
                        print(f"  INFO: {rel}:{i+1} has DWORD (not in script scope)")
                    if re.search(r'(?<![A-Z])\bWORD\b', line) and not line.lstrip().startswith("//"):
                        rel = os.path.relpath(fpath, SOURCES)
                        if "ASIO" in rel or "json" in rel:
                            continue
                        print(f"  INFO: {rel}:{i+1} has WORD (not in script scope)")

    if issues == 0:
        print("  No issues found. Safe to apply.")
    else:
        print(f"\n  {issues} issue(s) found. Review before applying.")
    return issues

# ============================================================
# DRY RUN / APPLY
# ============================================================

def process(dry_run=True):
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    log_path = os.path.join(OUTPUT_DIR, "phase_d1_changes.log")

    total_changes = 0
    file_changes = {}

    # Collect all unique files
    all_files = {}
    for rel in set(DWORD_FILES + WORD_FILES + REMOVE_WINDOWS_H + ADD_CSTDINT):
        path = full_path(rel)
        if not os.path.exists(path):
            print(f"  ERROR: File not found: {rel}")
            continue
        all_files[rel] = read_file(path)

    log_lines = []
    log_lines.append(f"Phase D1: Client Windows Type Cleanup {'(DRY RUN)' if dry_run else '(APPLIED)'}\n")
    log_lines.append("=" * 70 + "\n\n")

    # Process each file
    for rel in sorted(all_files.keys()):
        lines = all_files[rel]
        changes_for_file = []

        # Remove includes first (before type replacements, since removing windows.h
        # changes line count)
        if rel in REMOVE_WINDOWS_H:
            lines, removed = remove_include(lines, "<windows.h>")
            for line_num, old in removed:
                changes_for_file.append(f"  L{line_num}: REMOVE {old}")

        # Remove <mmsystem.h> from Tile.h
        if rel == "Client/Tile.h":
            lines, removed = remove_include(lines, "<mmsystem.h>")
            for line_num, old in removed:
                changes_for_file.append(f"  L{line_num}: REMOVE {old}")

        # Remove <io.h> from MapData.h
        if rel == "Client/MapData.h":
            lines, removed = remove_include(lines, "<io.h>")
            for line_num, old in removed:
                changes_for_file.append(f"  L{line_num}: REMOVE {old}")

        # Add <cstdint> to headers that need it
        if rel in ADD_CSTDINT:
            lines, added = add_cstdint(lines, rel)
            for line_num, new in added:
                changes_for_file.append(f"  L{line_num}: ADD    {new}")

        # Replace DWORD -> uint32_t
        if rel in DWORD_FILES:
            dw_changes = replace_dword(lines, rel)
            for line_num, old, new in dw_changes:
                changes_for_file.append(f"  L{line_num}: DWORD->uint32_t")
                changes_for_file.append(f"    OLD: {old}")
                changes_for_file.append(f"    NEW: {new}")

        # Replace WORD -> uint16_t
        if rel in WORD_FILES:
            w_changes = replace_word(lines, rel)
            for line_num, old, new in w_changes:
                changes_for_file.append(f"  L{line_num}: WORD->uint16_t")
                changes_for_file.append(f"    OLD: {old}")
                changes_for_file.append(f"    NEW: {new}")

        if changes_for_file:
            count = sum(1 for c in changes_for_file if not c.startswith("    "))
            total_changes += count
            file_changes[rel] = count
            log_lines.append(f"--- {rel} ({count} changes) ---\n")
            for c in changes_for_file:
                log_lines.append(c + "\n")
            log_lines.append("\n")

            if not dry_run:
                write_file(full_path(rel), lines)

    # Also handle MapData.h (only include removal, not in type replacement lists)
    if "Client/MapData.h" not in all_files:
        path = full_path("Client/MapData.h")
        if os.path.exists(path):
            lines = read_file(path)
            lines, removed = remove_include(lines, "<io.h>")
            if removed:
                total_changes += len(removed)
                file_changes["Client/MapData.h"] = len(removed)
                log_lines.append(f"--- Client/MapData.h ({len(removed)} changes) ---\n")
                for line_num, old in removed:
                    log_lines.append(f"  L{line_num}: REMOVE {old}\n")
                log_lines.append("\n")
                if not dry_run:
                    write_file(path, lines)

    # Summary
    log_lines.append("=" * 70 + "\n")
    log_lines.append(f"TOTAL: {total_changes} changes across {len(file_changes)} files\n\n")
    log_lines.append("Files changed:\n")
    for rel, count in sorted(file_changes.items()):
        log_lines.append(f"  {rel}: {count} changes\n")

    with open(log_path, "w", encoding="utf-8") as f:
        f.writelines(log_lines)

    print(f"{'DRY RUN' if dry_run else 'APPLIED'}: {total_changes} changes across {len(file_changes)} files")
    print(f"Full log: {log_path}")

    if dry_run:
        # Also print summary to stdout
        for line in log_lines:
            print(line, end="")

# ============================================================
# MAIN
# ============================================================

if __name__ == "__main__":
    if "--verify" in sys.argv:
        sys.exit(verify())
    elif "--dry-run" in sys.argv:
        process(dry_run=True)
    else:
        process(dry_run=False)

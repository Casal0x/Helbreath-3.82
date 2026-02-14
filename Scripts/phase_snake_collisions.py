#!/usr/bin/env python3
"""Phase 11: Resolve collision-group Hungarian names.

Phase 9+10 skipped ~300 names across ~122 collision groups because multiple
Hungarian prefixes map to the same snake_case target (e.g., iCount, sCount,
dwCount all -> count).

This script resolves them by:
1. Building all collision groups from the same logic as phase_snake_params.py
2. Applying semantic overrides for ambiguous groups (msX/szX/poX -> mouse_x/size_x/offset_x)
3. Checking co-occurrence using function-body tracking (not proximity heuristic)
4. Renaming all collision-group names to their (possibly overridden) target
5. Reporting true same-scope conflicts for manual post-fix

Mode 2 justified: ~300 names across 200+ files, mechanical pattern.

Usage:
  --dry-run   Preview changes without modifying files (log to Scripts/output/)
  --verify    Analyze co-occurrence and report conflicts
  --list      Print collision groups only
"""

import re
import os
import sys
import argparse
from collections import defaultdict

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from phase_snake_params import (
    collect_hungarian_names,
    hungarian_to_snake,
    EXCLUDE_NAMES,
    OVERRIDE_RENAMES,
    CPP_KEYWORDS,
    collect_files,
)

BASE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUTPUT_DIR = os.path.join(BASE, "Scripts", "output")


# ═══════════════════════════════════════════════════════════════════════════
# COLLISION OVERRIDES — names that need a different target than the default
# ═══════════════════════════════════════════════════════════════════════════
# These break ambiguous collision groups where the default target (e.g. 'x')
# would lose semantic meaning or cause guaranteed same-scope conflicts.

COLLISION_OVERRIDES = {
    # x/y groups: msX=mouse, szX=size, poX=position offset — semantically distinct
    'msX': 'mouse_x',
    'msY': 'mouse_y',
    'szX': 'size_x',
    'szY': 'size_y',
    'poX': 'offset_x',
    'poY': 'offset_y',
    # x1/y1: pX1=previous, iX1=current — same-scope conflict in WeatherManager.cpp
    'pX1': 'prev_x1',
    'pY1': 'prev_y1',
}


# ═══════════════════════════════════════════════════════════════════════════
# COLLISION GROUP ANALYSIS
# ═══════════════════════════════════════════════════════════════════════════

def _get_snake_target(name):
    """Get the snake_case target for a Hungarian name, checking all override maps."""
    if name in COLLISION_OVERRIDES:
        return COLLISION_OVERRIDES[name]
    if name in OVERRIDE_RENAMES:
        return OVERRIDE_RENAMES[name]
    return hungarian_to_snake(name)


def _default_snake_target(name):
    """Get the DEFAULT snake_case target (ignoring COLLISION_OVERRIDES)."""
    if name in OVERRIDE_RENAMES:
        return OVERRIDE_RENAMES[name]
    return hungarian_to_snake(name)


def build_collision_groups_and_renames(names):
    """Build collision groups and the full rename table.

    Returns:
        groups: {target: [(name, count), ...]} — remaining collision groups (2+ members)
        renames: [(old, new), ...] — ALL renames (groups + overridden singletons)
    """
    # Step 1: Find names that were in original collision groups (using default targets)
    default_target_to_sources = defaultdict(list)
    for name in sorted(names.keys()):
        if name in EXCLUDE_NAMES:
            continue
        snake = _default_snake_target(name)
        if snake is None or snake == name or snake in CPP_KEYWORDS or len(name) <= 2:
            continue
        default_target_to_sources[snake].append((name, names[name]))

    original_collision_names = set()
    for target, sources in default_target_to_sources.items():
        if len(sources) > 1:
            for name, _ in sources:
                original_collision_names.add(name)

    # Step 2: Build final targets with COLLISION_OVERRIDES applied
    final_target_to_sources = defaultdict(list)
    renames = []

    for name in sorted(original_collision_names):
        snake = _get_snake_target(name)
        if snake and snake != name and snake not in CPP_KEYWORDS:
            renames.append((name, snake))
            final_target_to_sources[snake].append((name, names[name]))

    # Groups are entries with 2+ members after overrides
    groups = {t: s for t, s in final_target_to_sources.items() if len(s) > 1}

    return groups, renames


def analyze_cooccurrence(groups, files):
    """Check file-level co-occurrence within each collision group.

    Returns:
        cooccurrences: {target: {filepath: {name: [line_numbers]}}}
    """
    all_names = set()
    for sources in groups.values():
        for name, _ in sources:
            all_names.add(name)

    if not all_names:
        return {}

    sorted_names = sorted(all_names, key=len, reverse=True)
    combined_re = re.compile(
        r'\b(' + '|'.join(re.escape(n) for n in sorted_names) + r')\b')

    file_names = defaultdict(set)
    name_file_lines = defaultdict(lambda: defaultdict(list))

    for fpath in files:
        with open(fpath, "r", encoding="utf-8") as f:
            lines = f.readlines()

        in_pragma_pack = False
        for lineno, line in enumerate(lines, 1):
            stripped = line.lstrip()
            if 'HB_PACKED' in stripped or '#pragma pack' in stripped:
                in_pragma_pack = True
            if in_pragma_pack and stripped.startswith('};'):
                in_pragma_pack = False
                continue
            if in_pragma_pack:
                continue
            if stripped.startswith(('#include', '#pragma')):
                continue

            for m in combined_re.finditer(line):
                name = m.group(1)
                if name in all_names:
                    file_names[fpath].add(name)
                    name_file_lines[name][fpath].append(lineno)

    cooccurrences = {}
    for target, sources in groups.items():
        source_names = {name for name, _ in sources}
        for fpath, names_found in file_names.items():
            overlap = names_found & source_names
            if len(overlap) > 1:
                if target not in cooccurrences:
                    cooccurrences[target] = {}
                details = {}
                for name in sorted(overlap):
                    details[name] = name_file_lines[name][fpath][:10]
                cooccurrences[target][fpath] = details

    return cooccurrences


# ═══════════════════════════════════════════════════════════════════════════
# SCOPE DETECTION — function-body tracking via brace depth
# ═══════════════════════════════════════════════════════════════════════════

def _is_func_signature(line):
    """Check if a line looks like it ends with a function signature."""
    s = line.rstrip()
    return (s.endswith(')') or
            s.endswith(') const') or
            s.endswith(') override') or
            s.endswith(') noexcept') or
            s.endswith(') const override') or
            s.endswith(') const noexcept'))


def find_function_bodies(file_lines):
    """Find function body ranges using brace tracking.

    Identifies blocks where '{' follows a function signature (Allman or K&R style).
    Returns list of (start_line, end_line) tuples (1-indexed, inclusive).
    """
    bodies = []
    depth = 0
    # Stack: (depth_when_opened, start_line_1indexed, is_function_body)
    brace_stack = []
    prev_meaningful = ""

    for i, line in enumerate(file_lines):
        stripped = line.strip()

        # Skip pure comment lines for brace tracking reliability
        if stripped.startswith('//'):
            continue

        # Process characters left-to-right for correct brace ordering
        in_string = False
        in_char = False
        j = 0
        while j < len(stripped):
            ch = stripped[j]

            # Skip string literals
            if ch == '"' and not in_char:
                in_string = not in_string
                j += 1
                continue
            if ch == "'" and not in_string:
                in_char = not in_char
                j += 1
                continue
            if in_string or in_char:
                if ch == '\\':
                    j += 2
                    continue
                j += 1
                continue

            # Skip // comments
            if ch == '/' and j + 1 < len(stripped) and stripped[j + 1] == '/':
                break

            if ch == '{':
                depth += 1
                is_func = False
                # K&R: check text before { on same line
                before = stripped[:j].strip()
                if _is_func_signature(before):
                    is_func = True
                # Allman: { on its own line, check previous meaningful line
                elif j == 0 and _is_func_signature(prev_meaningful):
                    is_func = True
                # Constructor initializer list: prev line has ': member(val)'
                elif j == 0 and prev_meaningful.endswith(')') and ':' in prev_meaningful:
                    is_func = True

                brace_stack.append((depth, i + 1, is_func))

            elif ch == '}':
                if brace_stack and brace_stack[-1][0] == depth:
                    _, start, is_func = brace_stack.pop()
                    if is_func:
                        bodies.append((start, i + 1))
                depth -= 1

            j += 1

        if stripped and not stripped.startswith('//'):
            prev_meaningful = stripped

    return bodies


def check_same_scope(file_lines, lines_a, lines_b):
    """Check if two sets of line numbers share the same function body.

    Uses function-body tracking (brace parsing) instead of proximity heuristic.
    Returns True only if both names appear inside the same function definition.
    """
    bodies = find_function_bodies(file_lines)

    def get_body_index(lineno):
        """Find which function body contains this line, or -1."""
        for idx, (start, end) in enumerate(bodies):
            if start <= lineno <= end:
                return idx
        return -1

    for la in lines_a:
        for lb in lines_b:
            body_a = get_body_index(la)
            body_b = get_body_index(lb)
            # Both must be inside a function body AND the same one
            if body_a >= 0 and body_b >= 0 and body_a == body_b:
                return True
    return False


# ═══════════════════════════════════════════════════════════════════════════
# RENAME APPLICATION
# ═══════════════════════════════════════════════════════════════════════════

def apply_renames(files, renames, dry_run=False, log_file=None):
    """Apply word-boundary renames across all files.

    Skips #include, #pragma, and HB_PACKED/pragma-pack sections.
    Sorts longest-first to avoid partial matches.
    """
    renames_sorted = sorted(renames, key=lambda x: -len(x[0]))
    patterns = [(re.compile(r"\b" + re.escape(old) + r"\b"), new, old)
                for old, new in renames_sorted]

    modified_files = []
    total_changes = 0

    for fpath in files:
        with open(fpath, "r", encoding="utf-8") as f:
            lines = f.readlines()

        new_lines = []
        file_changes = 0
        in_pragma_pack = False

        for lineno, line in enumerate(lines, 1):
            original = line
            stripped = line.lstrip()

            # Skip #include and #pragma lines
            if stripped.startswith("#include") or stripped.startswith("#pragma"):
                if 'HB_PACKED' in stripped or '#pragma pack' in stripped:
                    in_pragma_pack = True
                new_lines.append(line)
                continue

            # Skip wire protocol struct contents
            if in_pragma_pack:
                if stripped.startswith('};'):
                    in_pragma_pack = False
                new_lines.append(line)
                continue

            # Apply renames
            for pat, new, old in patterns:
                line = pat.sub(new, line)

            if line != original:
                file_changes += 1
                if log_file:
                    rel = os.path.relpath(fpath, BASE)
                    log_file.write(f"  {rel}:{lineno}\n")
                    log_file.write(f"    - {original.rstrip()}\n")
                    log_file.write(f"    + {line.rstrip()}\n")
            new_lines.append(line)

        if file_changes > 0:
            if not dry_run:
                with open(fpath, "w", encoding="utf-8", newline="\n") as f:
                    f.writelines(new_lines)
            total_changes += file_changes
            modified_files.append(fpath)
            rel = os.path.relpath(fpath, BASE)
            print(f"  MOD  {rel} ({file_changes} lines)")

    return modified_files, total_changes


# ═══════════════════════════════════════════════════════════════════════════
# MAIN
# ═══════════════════════════════════════════════════════════════════════════

def main():
    parser = argparse.ArgumentParser(
        description="Phase 11: Resolve collision-group Hungarian renames")
    parser.add_argument("--dry-run", action="store_true",
                        help="Preview changes without modifying files")
    parser.add_argument("--verify", action="store_true",
                        help="Analyze co-occurrence and report conflicts")
    parser.add_argument("--list", action="store_true",
                        help="Print collision groups only")
    args = parser.parse_args()

    mode = ("DRY RUN" if args.dry_run else
            "VERIFY" if args.verify else
            "LIST" if args.list else "LIVE")
    print(f"Phase 11: Collision-group Hungarian renames [{mode}]")
    print()

    # ── Step 1: Collect names and build collision groups ──────────────
    print("Scanning for Hungarian-prefixed identifiers...")
    names = collect_hungarian_names()
    print(f"  Found {len(names)} unique Hungarian names "
          f"({sum(names.values())} total occurrences)")
    print()

    print("Building collision groups (with overrides)...")
    groups, renames = build_collision_groups_and_renames(names)
    total_group_names = sum(len(s) for s in groups.values())
    total_renames = len(renames)
    singleton_count = total_renames - total_group_names
    print(f"  {len(groups)} collision groups ({total_group_names} names)")
    if singleton_count > 0:
        print(f"  {singleton_count} overridden singletons (broken out of groups)")
    print(f"  {total_renames} total renames to apply")
    print()

    if args.list:
        print("  === COLLISION GROUPS (2+ members -> same target) ===")
        for target in sorted(groups.keys()):
            sources = groups[target]
            source_str = ", ".join(
                f"{n}({c})" for n, c in sorted(sources, key=lambda x: -x[1]))
            print(f"  {target:30s} <- {source_str}")
        if COLLISION_OVERRIDES:
            print()
            print("  === OVERRIDDEN SINGLETONS ===")
            for old, new in sorted(COLLISION_OVERRIDES.items()):
                if old in names:
                    print(f"  {old:30s} -> {new}")
        return 0

    # ── Step 2: Analyze co-occurrence for remaining groups ────────────
    files = collect_files()
    print(f"Analyzing co-occurrence across {len(files)} files...")
    cooccurrences = analyze_cooccurrence(groups, files)

    safe_count = len(groups) - len(cooccurrences)
    print(f"  {safe_count} groups: no file-level co-occurrence (fully safe)")
    print(f"  {len(cooccurrences)} groups: file-level co-occurrence detected")
    print()

    # ── Step 3: Scope-level check using function-body tracking ────────
    same_scope_conflicts = {}
    diff_scope_groups = {}

    for target, fpaths in cooccurrences.items():
        target_has_same_scope = False
        conflict_details = []

        for fpath, name_lines in fpaths.items():
            names_list = sorted(name_lines.keys())
            with open(fpath, "r", encoding="utf-8") as f:
                file_content = f.readlines()

            for i, name_a in enumerate(names_list):
                for name_b in names_list[i + 1:]:
                    if check_same_scope(file_content,
                                        name_lines[name_a],
                                        name_lines[name_b]):
                        target_has_same_scope = True
                        rel = os.path.relpath(fpath, BASE)
                        conflict_details.append(
                            f"    {rel}: {name_a} (L{name_lines[name_a][:3]})"
                            f" vs {name_b} (L{name_lines[name_b][:3]})")

        if target_has_same_scope:
            same_scope_conflicts[target] = conflict_details
        else:
            diff_scope_groups[target] = fpaths

    print(f"  {len(diff_scope_groups)} groups: co-occurrence in "
          f"different scopes (safe)")
    print(f"  {len(same_scope_conflicts)} groups: SAME-SCOPE conflicts")

    if same_scope_conflicts:
        print()
        print("  Same-scope conflicts (will need manual post-fix):")
        for target, details in sorted(same_scope_conflicts.items()):
            sources = [n for n, _ in groups[target]]
            print(f"    '{target}' <- {sources}")
            for d in details:
                print(d)
    print()

    if args.verify:
        os.makedirs(OUTPUT_DIR, exist_ok=True)
        log_path = os.path.join(OUTPUT_DIR,
                                "phase_snake_collisions_verify.log")
        with open(log_path, "w", encoding="utf-8") as f:
            f.write("=== Phase 11: Collision-group co-occurrence "
                    "analysis ===\n\n")
            f.write(f"Total collision groups: {len(groups)}\n")
            f.write(f"Total group names: {total_group_names}\n")
            f.write(f"Overridden singletons: {singleton_count}\n")
            f.write(f"Total renames: {total_renames}\n\n")
            f.write(f"No co-occurrence (fully safe): {safe_count}\n")
            f.write(f"Different-scope co-occurrence (safe): "
                    f"{len(diff_scope_groups)}\n")
            f.write(f"Same-scope conflicts: "
                    f"{len(same_scope_conflicts)}\n\n")

            f.write("=" * 70 + "\n")
            f.write("SAME-SCOPE CONFLICTS (need manual post-fix)\n")
            f.write("=" * 70 + "\n\n")
            if same_scope_conflicts:
                for target, details in sorted(same_scope_conflicts.items()):
                    sources = [n for n, _ in groups[target]]
                    f.write(f"  '{target}' <- {sources}\n")
                    for d in details:
                        f.write(f"{d}\n")
                    f.write("\n")
            else:
                f.write("  (none)\n\n")

            f.write("=" * 70 + "\n")
            f.write("ALL CO-OCCURRENCE DETAILS\n")
            f.write("=" * 70 + "\n\n")
            for target, fpaths in sorted(cooccurrences.items()):
                sources = [n for n, _ in groups[target]]
                scope_tag = ("CONFLICT" if target in same_scope_conflicts
                             else "safe (different scopes)")
                f.write(f"  '{target}' <- {sources}  [{scope_tag}]\n")
                for fpath, name_lines in sorted(fpaths.items()):
                    rel = os.path.relpath(fpath, BASE)
                    f.write(f"    {rel}:\n")
                    for name, linenos in sorted(name_lines.items()):
                        f.write(f"      {name}: lines {linenos}\n")
                f.write("\n")

            f.write("=" * 70 + "\n")
            f.write("ALL COLLISION GROUPS (after overrides)\n")
            f.write("=" * 70 + "\n\n")
            for target in sorted(groups.keys()):
                sources = groups[target]
                source_str = ", ".join(
                    f"{n}({c})"
                    for n, c in sorted(sources, key=lambda x: -x[1]))
                f.write(f"  {target:30s} <- {source_str}\n")

            if COLLISION_OVERRIDES:
                f.write("\n\nCOLLISION OVERRIDES APPLIED:\n\n")
                for old, new in sorted(COLLISION_OVERRIDES.items()):
                    if old in names:
                        f.write(f"  {old:30s} -> {new}\n")

        print(f"  Verify log: {os.path.relpath(log_path, BASE)}")
        return 0

    # ── Step 4: Build and apply renames ───────────────────────────────
    print(f"Building rename table: {len(renames)} entries")
    print()

    log_file = None
    log_path = None
    if args.dry_run:
        os.makedirs(OUTPUT_DIR, exist_ok=True)
        log_path = os.path.join(OUTPUT_DIR,
                                "phase_snake_collisions_dryrun.log")
        log_file = open(log_path, "w", encoding="utf-8")
        log_file.write("=== Phase 11: Collision-group renames "
                       "(DRY RUN) ===\n\n")
        log_file.write(f"Renames: {len(renames)}\n\n")

    try:
        action = "Previewing" if args.dry_run else "Applying"
        print(f"{action} renames across {len(files)} files...")
        mod_files, changes = apply_renames(
            files, renames, args.dry_run, log_file)
    finally:
        if log_file:
            log_file.write(f"\n\nSummary: {len(mod_files)} files, "
                           f"~{changes} line changes\n")
            log_file.close()

    print()
    print(f"Summary: {len(mod_files)} files modified, "
          f"~{changes} line changes")

    if args.dry_run and log_path:
        print(f"  Dry-run log: {os.path.relpath(log_path, BASE)}")

    if same_scope_conflicts:
        print()
        print(f"  NOTE: {len(same_scope_conflicts)} same-scope conflict(s) "
              f"may cause build errors.")
        print(f"  After build, fix with: bak.py guard <file> + Edit tool")
        for target, details in sorted(same_scope_conflicts.items()):
            sources = [n for n, _ in groups[target]]
            print(f"    '{target}' <- {sources}")

    return 0


if __name__ == "__main__":
    sys.exit(main())

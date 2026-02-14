#!/usr/bin/env python3
"""
Phase: Shield Glare Bug Fix + DrawParams/BitmapTextParams snake_case rename.

Bug: AdditiveColored(0,0,0) flashes white because renderer treats tint (0,0,0) as
"no tint set" and leaves sprite color at White under additive blending.

Fix: Add `has_tint` bool to DrawParams; factory methods that set tint values also
set has_tint=true. Renderer checks has_tint instead of tintR!=0||tintG!=0||tintB!=0.
Remove SFMLBitmapFont.cpp (1,1,1) clamp workaround.

Also renames DrawParams and BitmapTextParams fields+factories to snake_case.
"""

import re
import sys
import os
import copy

BASE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..")
CLIENT = os.path.join(BASE, "Sources", "Client")
SHARED = os.path.join(BASE, "Sources", "Dependencies", "Shared", "Render")
ENGINE = os.path.join(BASE, "Sources", "SFMLEngine")

# ============================================================
# RENAME TABLES
# ============================================================

# DrawParams factory methods (static): old -> new
DRAWPARAMS_FACTORIES = [
    # Order: longest first to prevent partial matches
    ("AdditiveNoColorKey", "additive_no_color_key"),
    ("AdditiveColored", "additive_colored"),
    ("AdditiveTinted", "additive_tinted"),
    ("TintedAlpha", "tinted_alpha"),
    ("NoColorKey", "no_color_key"),
    ("Additive", "additive"),
    ("Average", "average"),
    ("Opaque", "opaque"),
    ("Shadow", "shadow"),
    ("Alpha", "alpha_blend"),
    ("Tint", "tint"),
    ("Fade", "fade"),
]

# DrawParams fields: old -> new (all get m_ prefix per coding standards)
DRAWPARAMS_FIELDS = [
    # Longest first to prevent partial matches
    ("useColorKey", "m_use_color_key"),
    ("blendMode", "m_blend_mode"),
    ("additive", "m_additive"),
    ("reverse", "m_reverse"),
    ("shadow", "m_shadow"),
    ("tintR", "m_tint_r"),
    ("tintG", "m_tint_g"),
    ("tintB", "m_tint_b"),
    ("alpha", "m_alpha"),
    ("fade", "m_fade"),
]

# BitmapTextParams factory methods: old -> new
BITMAP_FACTORIES = [
    ("ColorReplaceWithShadow", "color_replace_with_shadow"),
    ("ColorReplaceWithAlpha", "color_replace_with_alpha"),
    ("TintedWithShadow", "tinted_with_shadow"),
    ("TintedWithAlpha", "tinted_with_alpha"),
    ("ColorReplace", "color_replace"),
    ("Default", "make_default"),
    ("Tinted", "tinted"),
]

# BitmapTextParams fields: old -> new (all get m_ prefix per coding standards)
BITMAP_FIELDS = [
    ("color_replace", "m_color_replace"),
    ("useAdditive", "m_use_additive"),
    ("shadow", "m_shadow"),
    ("tintR", "m_tint_r"),
    ("tintG", "m_tint_g"),
    ("tintB", "m_tint_b"),
    ("alpha", "m_alpha"),
]

# IBitmapFont methods: old -> new
BITMAP_METHODS = [
    ("DrawTextCentered", "draw_text_centered"),
    ("GetCharWidth", "get_char_width"),
]

# ============================================================
# FILE LISTS (from grep results)
# ============================================================

# Files that use DrawParams:: factory methods
DRAWPARAMS_FACTORY_FILES = [
    # Client
    os.path.join(CLIENT, f) for f in [
        "Screen_OnGame.cpp", "Game.cpp", "DialogBox_Manufacture.cpp",
        "DialogBox_Exchange.cpp", "DialogBox_Bank.cpp", "RenderHelpers.cpp",
        "PlayerRenderer.cpp", "NpcRenderer.cpp", "Game.DrawObjects.cpp",
        "DialogBox_Skill.cpp", "DialogBox_SellOrRepair.cpp", "DialogBox_Map.cpp",
        "DialogBox_ItemUpgrade.cpp", "DialogBox_Inventory.cpp",
        "DialogBox_GuideMap.cpp", "DialogBox_Character.cpp", "Screen_Login.cpp",
        "WeatherManager.cpp", "Screen_SelectCharacter.cpp", "Effect_Lights.cpp",
        "Effect_Draw.cpp", "Screen_Quit.cpp",
    ]
] + [
    # Shared
    os.path.join(SHARED, "ISprite.h"),
]

# Files that use DrawParams fields directly (not via factory)
DRAWPARAMS_FIELD_FILES = [
    os.path.join(SHARED, "SpriteTypes.h"),
    os.path.join(SHARED, "ISprite.h"),
    os.path.join(ENGINE, "SFMLSprite.cpp"),
    os.path.join(ENGINE, "SFMLBitmapFont.cpp"),
]

# Files that use BitmapTextParams:: factory methods
BITMAP_FACTORY_FILES = [
    os.path.join(SHARED, "IBitmapFont.h"),
    os.path.join(SHARED, "TextLib.cpp"),
]

# Files that use BitmapTextParams fields directly
BITMAP_FIELD_FILES = [
    os.path.join(SHARED, "IBitmapFont.h"),
    os.path.join(ENGINE, "SFMLBitmapFont.cpp"),
]

# Files that use IBitmapFont methods
BITMAP_METHOD_FILES = [
    os.path.join(SHARED, "IBitmapFont.h"),
    os.path.join(ENGINE, "SFMLBitmapFont.h"),
    os.path.join(ENGINE, "SFMLBitmapFont.cpp"),
]

# ============================================================
# HELPERS
# ============================================================

def read_file(path):
    with open(path, 'r', encoding='utf-8', errors='replace') as f:
        return f.readlines()

def write_file(path, lines):
    with open(path, 'w', encoding='utf-8', newline='') as f:
        f.writelines(lines)

def apply_renames(lines, renames, pattern_fn):
    """Apply a list of (old, new) renames using pattern_fn to build regex."""
    changes = 0
    for i, line in enumerate(lines):
        original = line
        for old, new in renames:
            pat = pattern_fn(old)
            line = re.sub(pat, lambda m, n=new, o=old: m.group(0).replace(o, n), line)
        if line != original:
            lines[i] = line
            changes += 1
    return changes

# ============================================================
# PHASE 1: DrawParams factory methods (DrawParams::Old -> DrawParams::new)
# ============================================================

def phase1_drawparams_factories(dry_run):
    print("Phase 1: DrawParams factory methods")
    total = 0
    all_files = list(set(DRAWPARAMS_FACTORY_FILES + [
        os.path.join(SHARED, "SpriteTypes.h"),
    ]))

    for path in sorted(all_files):
        if not os.path.exists(path):
            continue
        lines = read_file(path)
        original = list(lines)

        basename = os.path.basename(path)
        for old, new in DRAWPARAMS_FACTORIES:
            for i, line in enumerate(lines):
                # Consumer calls: DrawParams::Old(
                pat = rf'DrawParams::{re.escape(old)}\b'
                lines[i] = re.sub(pat, f'DrawParams::{new}', lines[i])
                # Declarations inside struct: static DrawParams Old(
                if basename == "SpriteTypes.h":
                    pat2 = rf'static DrawParams {re.escape(old)}\b'
                    lines[i] = re.sub(pat2, f'static DrawParams {new}', lines[i])

        changes = sum(1 for a, b in zip(original, lines) if a != b)
        if changes > 0:
            total += changes
            fname = os.path.relpath(path, BASE)
            print(f"  {fname}: {changes} lines")
            if dry_run:
                for ln, (a, b) in enumerate(zip(original, lines)):
                    if a != b:
                        print(f"    L{ln+1}: {a.rstrip()}")
                        print(f"      -> {b.rstrip()}")
            else:
                write_file(path, lines)

    print(f"  Subtotal: {total} lines")
    return total

# ============================================================
# PHASE 2: DrawParams fields (.tintR -> .tint_r etc.)
# ============================================================

def phase2_drawparams_fields(dry_run):
    print("Phase 2: DrawParams fields")
    total = 0
    all_files = list(set(DRAWPARAMS_FIELD_FILES + DRAWPARAMS_FACTORY_FILES))

    for path in sorted(all_files):
        if not os.path.exists(path):
            continue
        lines = read_file(path)
        original = list(lines)

        for old, new in DRAWPARAMS_FIELDS:
            basename = os.path.basename(path)
            for i, line in enumerate(lines):
                if basename == "SpriteTypes.h":
                    # Declaration file: rename in code portion, not in // comments
                    comment_idx = line.find('//')
                    if comment_idx >= 0:
                        code_part = line[:comment_idx]
                        comment_part = line[comment_idx:]
                    else:
                        code_part = line
                        comment_part = ''
                    pat = rf'\b{re.escape(old)}\b'
                    new_code = re.sub(pat, new, code_part)
                    lines[i] = new_code + comment_part
                else:
                    # Consumer files: only rename after . or ->
                    pat = rf'(?<=\.){re.escape(old)}\b|(?<=->){re.escape(old)}\b'
                    lines[i] = re.sub(pat, new, lines[i])

        changes = sum(1 for a, b in zip(original, lines) if a != b)
        if changes > 0:
            total += changes
            fname = os.path.relpath(path, BASE)
            print(f"  {fname}: {changes} lines")
            if dry_run:
                for ln, (a, b) in enumerate(zip(original, lines)):
                    if a != b:
                        print(f"    L{ln+1}: {a.rstrip()}")
                        print(f"      -> {b.rstrip()}")
            else:
                write_file(path, lines)

    print(f"  Subtotal: {total} lines")
    return total

# ============================================================
# PHASE 3: BitmapTextParams factory methods
# ============================================================

def phase3_bitmap_factories(dry_run):
    print("Phase 3: BitmapTextParams factory methods")
    total = 0
    all_files = list(set(BITMAP_FACTORY_FILES))

    for path in sorted(all_files):
        if not os.path.exists(path):
            continue
        lines = read_file(path)
        original = list(lines)

        basename = os.path.basename(path)
        for old, new in BITMAP_FACTORIES:
            for i, line in enumerate(lines):
                # Consumer calls: BitmapTextParams::Old(
                pat = rf'BitmapTextParams::{re.escape(old)}\b'
                lines[i] = re.sub(pat, f'BitmapTextParams::{new}', lines[i])
                # Declarations inside struct: static BitmapTextParams Old(
                if basename == "IBitmapFont.h":
                    pat2 = rf'static BitmapTextParams {re.escape(old)}\b'
                    lines[i] = re.sub(pat2, f'static BitmapTextParams {new}', lines[i])

        changes = sum(1 for a, b in zip(original, lines) if a != b)
        if changes > 0:
            total += changes
            fname = os.path.relpath(path, BASE)
            print(f"  {fname}: {changes} lines")
            if dry_run:
                for ln, (a, b) in enumerate(zip(original, lines)):
                    if a != b:
                        print(f"    L{ln+1}: {a.rstrip()}")
                        print(f"      -> {b.rstrip()}")
            else:
                write_file(path, lines)

    print(f"  Subtotal: {total} lines")
    return total

# ============================================================
# PHASE 4: BitmapTextParams fields
# ============================================================

def phase4_bitmap_fields(dry_run):
    print("Phase 4: BitmapTextParams fields")
    total = 0
    all_files = list(set(BITMAP_FIELD_FILES))

    for path in sorted(all_files):
        if not os.path.exists(path):
            continue
        lines = read_file(path)
        original = list(lines)

        for old, new in BITMAP_FIELDS:
            basename = os.path.basename(path)
            for i, line in enumerate(lines):
                if basename == "IBitmapFont.h":
                    # Declaration file: rename in code portion, not in // comments
                    comment_idx = line.find('//')
                    if comment_idx >= 0:
                        code_part = line[:comment_idx]
                        comment_part = line[comment_idx:]
                    else:
                        code_part = line
                        comment_part = ''
                    pat = rf'\b{re.escape(old)}\b'
                    new_code = re.sub(pat, new, code_part)
                    lines[i] = new_code + comment_part
                else:
                    # Consumer: after . or ->
                    pat = rf'(?<=\.){re.escape(old)}\b|(?<=->){re.escape(old)}\b'
                    lines[i] = re.sub(pat, new, lines[i])

        changes = sum(1 for a, b in zip(original, lines) if a != b)
        if changes > 0:
            total += changes
            fname = os.path.relpath(path, BASE)
            print(f"  {fname}: {changes} lines")
            if dry_run:
                for ln, (a, b) in enumerate(zip(original, lines)):
                    if a != b:
                        print(f"    L{ln+1}: {a.rstrip()}")
                        print(f"      -> {b.rstrip()}")
            else:
                write_file(path, lines)

    print(f"  Subtotal: {total} lines")
    return total

# ============================================================
# PHASE 5: IBitmapFont methods
# ============================================================

def phase5_bitmap_methods(dry_run):
    print("Phase 5: IBitmapFont methods (GetCharWidth, DrawTextCentered)")
    total = 0
    all_files = list(set(BITMAP_METHOD_FILES))

    for path in sorted(all_files):
        if not os.path.exists(path):
            continue
        lines = read_file(path)
        original = list(lines)

        for old, new in BITMAP_METHODS:
            for i, line in enumerate(lines):
                pat = rf'\b{re.escape(old)}\b'
                lines[i] = re.sub(pat, new, lines[i])

        changes = sum(1 for a, b in zip(original, lines) if a != b)
        if changes > 0:
            total += changes
            fname = os.path.relpath(path, BASE)
            print(f"  {fname}: {changes} lines")
            if dry_run:
                for ln, (a, b) in enumerate(zip(original, lines)):
                    if a != b:
                        print(f"    L{ln+1}: {a.rstrip()}")
                        print(f"      -> {b.rstrip()}")
            else:
                write_file(path, lines)

    print(f"  Subtotal: {total} lines")
    return total

# ============================================================
# PHASE 6: Bug fix - add has_tint, update renderer, remove workaround
# ============================================================

def phase6_bug_fix(dry_run):
    print("Phase 6: Bug fix (has_tint + renderer update + remove workaround)")
    total = 0
    changes_detail = []

    # --- 6a: Add has_tint field to DrawParams in SpriteTypes.h ---
    path = os.path.join(SHARED, "SpriteTypes.h")
    lines = read_file(path)
    original = list(lines)

    for i, line in enumerate(lines):
        # Add m_has_tint after tintB/m_tint_b declaration (handle pre/post rename)
        if ('m_tint_b' in line or 'tintB' in line) and '= 0;' in line and 'int16_t' in line:
            indent = line[:len(line) - len(line.lstrip())]
            lines.insert(i + 1, f"{indent}bool m_has_tint = false;\n")
            break

    # Set m_has_tint = true in the 4 factory methods that set tint values
    # Handle both pre-rename (PascalCase) and post-rename (snake_case) names
    tint_factories_post = ['tint', 'tinted_alpha', 'additive_tinted', 'additive_colored']
    tint_factories_pre = ['Tint', 'TintedAlpha', 'AdditiveTinted', 'AdditiveColored']
    all_tint_factories = tint_factories_post + tint_factories_pre
    j = 0
    while j < len(lines):
        line = lines[j]
        for factory in all_tint_factories:
            if f'static DrawParams {factory}(' in line:
                # Find the "return p;" in this factory and add m_has_tint before it
                for k in range(j + 1, min(j + 15, len(lines))):
                    if 'return p;' in lines[k]:
                        indent = lines[k][:len(lines[k]) - len(lines[k].lstrip())]
                        lines.insert(k, f"{indent}p.m_has_tint = true;\n")
                        break
                break
        j += 1

    changes = sum(1 for a, b in zip(original, lines) if a != b) + (len(lines) - len(original))
    if changes > 0:
        total += changes
        fname = os.path.relpath(path, BASE)
        print(f"  {fname}: {changes} lines (added has_tint)")
        if not dry_run:
            write_file(path, lines)

    # --- 6b: Update SFMLSprite.cpp tint checks ---
    path = os.path.join(ENGINE, "SFMLSprite.cpp")
    lines = read_file(path)
    original = list(lines)

    for i, line in enumerate(lines):
        # Replace the two tint checks — handle "if (...)" vs expression context
        # After phase 2: fields are m_tint_r etc. Before phase 2: tintR etc.
        post_check = '(params.m_tint_r != 0 || params.m_tint_g != 0 || params.m_tint_b != 0)'
        pre_check = '(params.tintR != 0 || params.tintG != 0 || params.tintB != 0)'
        if post_check in line or pre_check in line:
            if re.match(r'\s*if\s*\(params\.', line):
                # if-statement: replace whole "if (...)" with "if (params.m_has_tint)"
                lines[i] = re.sub(
                    r'if\s*\(params\.m_tint_r != 0 \|\| params\.m_tint_g != 0 \|\| params\.m_tint_b != 0\)',
                    'if (params.m_has_tint)', lines[i])
                lines[i] = re.sub(
                    r'if\s*\(params\.tintR != 0 \|\| params\.tintG != 0 \|\| params\.tintB != 0\)',
                    'if (params.m_has_tint)', lines[i])
            else:
                # Expression context: drop the grouping parens
                lines[i] = lines[i].replace(post_check, 'params.m_has_tint')
                lines[i] = lines[i].replace(pre_check, 'params.m_has_tint')
        # Snake_case local variables in the draw function
        if 'needsColorChange' in line:
            lines[i] = lines[i].replace('needsColorChange', 'needs_color_change')

    changes = sum(1 for a, b in zip(original, lines) if a != b)
    if changes > 0:
        total += changes
        fname = os.path.relpath(path, BASE)
        print(f"  {fname}: {changes} lines")
        if dry_run:
            for ln, (a, b) in enumerate(zip(original, lines)):
                if a != b:
                    print(f"    L{ln+1}: {a.rstrip()}")
                    print(f"      -> {b.rstrip()}")
        if not dry_run:
            write_file(path, lines)

    # --- 6c: Remove BitmapFont (1,1,1) workaround, set has_tint ---
    path = os.path.join(ENGINE, "SFMLBitmapFont.cpp")
    lines = read_file(path)
    original = list(lines)

    new_lines = []
    skip_clamp = False
    for i, line in enumerate(lines):
        # Remove the 4-line clamp block: if (params.color_replace && drawParams.tintR == 0 ...) { ... }
        # Handle both pre-rename (color_replace, tintR) and post-rename (m_color_replace, m_tint_r)
        stripped = line.strip()
        if ('color_replace' in stripped and
            ('m_tint_r == 0' in stripped or 'tintR == 0' in stripped) and
            ('m_tint_g == 0' in stripped or 'tintG == 0' in stripped)):
            skip_clamp = True
            continue
        if skip_clamp:
            if stripped == '{':
                continue
            if ('m_tint_r = 1' in stripped or 'tintR = 1' in stripped or
                'm_tint_g = 1' in stripped or 'tintG = 1' in stripped or
                'm_tint_b = 1' in stripped or 'tintB = 1' in stripped):
                continue
            if stripped == '}':
                skip_clamp = False
                continue
            skip_clamp = False  # Something unexpected, stop skipping

        # Update outdated comment about the (1,1,1) clamp workaround
        if 'clamp to (1,1,1)' in stripped:
            new_lines.append(line.replace(
                'But (0,0,0) skips the tint block, so clamp to (1,1,1).',
                'm_has_tint flag ensures tint block runs even for (0,0,0).'))
            continue

        # After the tintB assignment, add m_has_tint logic
        # Look for the line that sets drawParams.m_tint_b (or tintB)
        if ('drawParams.m_tint_b = params.m_tint_b' in stripped or
            'drawParams.tintB = params.tintB' in stripped):
            new_lines.append(line)
            indent = line[:len(line) - len(line.lstrip())]
            # Use whichever field names match the current file state
            if 'm_tint_b' in stripped:
                new_lines.append(f"{indent}drawParams.m_has_tint = params.m_color_replace ||\n")
                new_lines.append(f"{indent}                        (params.m_tint_r != 0 || params.m_tint_g != 0 || params.m_tint_b != 0);\n")
            else:
                new_lines.append(f"{indent}drawParams.m_has_tint = params.color_replace ||\n")
                new_lines.append(f"{indent}                        (params.tintR != 0 || params.tintG != 0 || params.tintB != 0);\n")
            continue

        new_lines.append(line)

    lines = new_lines
    changes = 0
    # Count differences
    max_len = max(len(original), len(lines))
    for idx in range(max_len):
        a = original[idx] if idx < len(original) else ""
        b = lines[idx] if idx < len(lines) else ""
        if a != b:
            changes += 1
    if changes > 0:
        total += changes
        fname = os.path.relpath(path, BASE)
        print(f"  {fname}: {changes} lines (removed workaround, added has_tint)")
        if not dry_run:
            write_file(path, lines)

    print(f"  Subtotal: {total} lines")
    return total

# ============================================================
# MAIN
# ============================================================

def get_all_affected_files():
    """Return sorted list of all unique files affected."""
    files = set()
    for f in DRAWPARAMS_FACTORY_FILES:
        files.add(f)
    for f in DRAWPARAMS_FIELD_FILES:
        files.add(f)
    for f in BITMAP_FACTORY_FILES:
        files.add(f)
    for f in BITMAP_FIELD_FILES:
        files.add(f)
    for f in BITMAP_METHOD_FILES:
        files.add(f)
    files.add(os.path.join(SHARED, "SpriteTypes.h"))
    return sorted(files)

def main():
    dry_run = "--dry-run" in sys.argv
    verify = "--verify" in sys.argv
    list_files = "--list-files" in sys.argv

    if list_files:
        files = get_all_affected_files()
        for f in files:
            if os.path.exists(f):
                print(os.path.relpath(f, BASE))
        print(f"\nTotal: {len([f for f in files if os.path.exists(f)])} files")
        return

    if verify:
        print("=== VERIFY MODE ===")
        # Check for potential collisions
        issues = 0

        # Check that alpha_blend won't collide with alpha field
        path = os.path.join(SHARED, "SpriteTypes.h")
        lines = read_file(path)
        for i, line in enumerate(lines):
            if 'float alpha' in line and 'alpha_blend' not in line:
                pass  # This is the field declaration, expected
            if 'alpha_blend' in line and 'Alpha' not in line:
                print(f"  WARNING: alpha_blend already exists at {path}:{i+1}")
                issues += 1

        # Check that color_replace factory won't collide with is_color_replace field
        path = os.path.join(SHARED, "IBitmapFont.h")
        lines = read_file(path)
        for i, line in enumerate(lines):
            if 'is_color_replace' in line and 'color_replace' in line:
                pass  # After rename, field and factory are distinct

        if issues == 0:
            print("  No collisions found.")
        else:
            print(f"  {issues} issue(s) found!")
        return

    if dry_run:
        print("=== DRY RUN MODE ===\n")

    total = 0
    # IMPORTANT: Field renames run BEFORE factory renames to avoid collision.
    # e.g. `shadow` field → m_shadow first, then `Shadow` factory → shadow().
    # If factory rename ran first, Shadow→shadow would then match \bshadow\b in phase 2.
    total += phase2_drawparams_fields(dry_run)
    print()
    total += phase1_drawparams_factories(dry_run)
    print()
    total += phase4_bitmap_fields(dry_run)
    print()
    total += phase3_bitmap_factories(dry_run)
    print()
    total += phase5_bitmap_methods(dry_run)
    print()
    total += phase6_bug_fix(dry_run)

    print(f"\n{'='*50}")
    print(f"Total: {total} lines changed")
    if dry_run:
        print("\nRe-run without --dry-run to apply.")

if __name__ == "__main__":
    main()

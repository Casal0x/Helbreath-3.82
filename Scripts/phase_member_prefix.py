#!/usr/bin/env python3
"""
phase_member_prefix.py - Add m_ prefix to client struct data members.

Renames ~121 struct members across 11 structs, ~500+ references in ~20 files.
Uses context-aware accessor patterns to avoid false positives on common names
like 'action', 'direction', 'type', 'x', 'y', etc.

Three phases per file:
  Phase C: Rename member DECLARATIONS in struct bodies (headers only)
  Phase A: Rename accessor.member dot-qualified accesses (all files)
  Phase B: Rename bare member access in struct's own .cpp files

Usage:
    python phase_member_prefix.py --dry-run    # Preview all changes
    python phase_member_prefix.py --verify     # Check for conflicts
    python phase_member_prefix.py              # Apply changes
"""

import re
import sys
from pathlib import Path
from collections import defaultdict

BASE = Path("Z:/Helbreath-3.82/Sources/Client")
OUTPUT_DIR = Path("Z:/Helbreath-3.82/Scripts/output")
DRY_RUN = "--dry-run" in sys.argv
VERIFY = "--verify" in sys.argv

file_changes = defaultdict(list)

# ============================================================
# Rename Tables (old_name, new_name)
# ============================================================

ANIM_STATE = [
    ("action", "m_action"),
    ("dir", "m_dir"),
    ("max_frame", "m_max_frame"),
    ("frame_time", "m_frame_time"),
    ("loop", "m_loop"),
    ("current_frame", "m_current_frame"),
    ("previous_frame", "m_previous_frame"),
    ("last_frame_time", "m_last_frame_time"),
    ("finished", "m_finished"),
]

ENTITY_MOTION = [
    ("bIsMoving", "m_is_moving"),
    ("direction", "m_direction"),
    ("progress", "m_progress"),
    ("start_time", "m_start_time"),
    ("duration", "m_duration"),
    ("start_offset_x", "m_start_offset_x"),
    ("start_offset_y", "m_start_offset_y"),
    ("current_offset_x", "m_current_offset_x"),
    ("current_offset_y", "m_current_offset_y"),
    ("bHasPending", "m_has_pending"),
    ("pending_direction", "m_pending_direction"),
    ("pending_duration", "m_pending_duration"),
]

FOCUSED_OBJECT = [
    ("valid", "m_valid"),
    ("objectID", "m_object_id"),
    ("mapX", "m_map_x"),
    ("mapY", "m_map_y"),
    ("screenX", "m_screen_x"),
    ("screenY", "m_screen_y"),
    ("dataX", "m_data_x"),
    ("dataY", "m_data_y"),
    ("type", "m_type"),
    ("ownerType", "m_owner_type"),
    ("action", "m_action"),
    ("direction", "m_direction"),
    ("frame", "m_frame"),
    ("name", "m_name"),
    ("appearance", "m_appearance"),
    ("status", "m_status"),
]

TARGET_OBJECT = list(FOCUSED_OBJECT)

DIALOG_BOX = [
    ("dw_v1", "m_dw_v1"), ("dw_v2", "m_dw_v2"), ("dw_t1", "m_dw_t1"),
    ("v10", "m_v10"), ("v11", "m_v11"), ("v12", "m_v12"),
    ("v13", "m_v13"), ("v14", "m_v14"),
    ("v1", "m_v1"), ("v2", "m_v2"), ("v3", "m_v3"), ("v4", "m_v4"),
    ("v5", "m_v5"), ("v6", "m_v6"), ("v7", "m_v7"), ("v8", "m_v8"),
    ("v9", "m_v9"),
    ("can_close_on_right_click", "m_can_close_on_right_click"),
    ("is_scroll_selected", "m_is_scroll_selected"),
    ("size_x", "m_size_x"), ("size_y", "m_size_y"),
    ("flag", "m_flag"),
    ("view", "m_view"),
    ("str4", "m_str4"), ("str3", "m_str3"), ("str2", "m_str2"), ("str", "m_str"),
    ("mode", "m_mode"),
    ("x", "m_x"), ("y", "m_y"),
]

EQUIPMENT = [
    ("body_armor_index", "m_body_armor_index"),
    ("arm_armor_index", "m_arm_armor_index"),
    ("body_index", "m_body_index"),
    ("undies_index", "m_undies_index"),
    ("hair_index", "m_hair_index"),
    ("pants_index", "m_pants_index"),
    ("boots_index", "m_boots_index"),
    ("weapon_index", "m_weapon_index"),
    ("shield_index", "m_shield_index"),
    ("mantle_index", "m_mantle_index"),
    ("helm_index", "m_helm_index"),
    ("weapon_color", "m_weapon_color"),
    ("shield_color", "m_shield_color"),
    ("armor_color", "m_armor_color"),
    ("mantle_color", "m_mantle_color"),
    ("arm_color", "m_arm_color"),
    ("pants_color", "m_pants_color"),
    ("boots_color", "m_boots_color"),
    ("helm_color", "m_helm_color"),
    ("weapon_glare", "m_weapon_glare"),
    ("shield_glare", "m_shield_glare"),
    ("skirt_draw", "m_skirt_draw"),
]

ANIM_DEF = [
    ("max_frame", "m_max_frame"),
    ("frame_time", "m_frame_time"),
    ("loop", "m_loop"),
]

SPELL_TILE = [("x", "m_x"), ("y", "m_y")]

SPELL_PARAMS = [
    ("magicType", "m_magic_type"),
    ("aoeRadiusX", "m_aoe_radius_x"),
    ("aoeRadiusY", "m_aoe_radius_y"),
    ("dynamicPattern", "m_dynamic_pattern"),
    ("dynamicRadius", "m_dynamic_radius"),
]

ANIM_PARAMS = [
    ("lifetime_ms", "m_lifetime_ms"),
    ("show_delay_ms", "m_show_delay_ms"),
    ("start_offset_y", "m_start_offset_y"),
    ("rise_pixels", "m_rise_pixels"),
    ("rise_duration_ms", "m_rise_duration_ms"),
    ("font_offset", "m_font_offset"),
    ("color", "m_color"),
    ("use_sprite_font", "m_use_sprite_font"),
]

BENCHMARK = [
    ("name", "m_name"),
    ("accumulated_ns", "m_accumulated_ns"),
    ("call_count", "m_call_count"),
    ("lastOutputTime", "m_last_output_time"),
]

# ============================================================
# Phase A: Accessor patterns for dot-qualified replacement
# (accessor_regex, rename_list, file_scope_set_or_None)
# ============================================================

ACCESSOR_RULES = [
    (r'm_animation', ANIM_STATE, None),
    (r'm_motion', ENTITY_MOTION, None),
    (r'\bmotion', ENTITY_MOTION, None),
    (r's_focusedObject', FOCUSED_OBJECT, None),
    (r'\binfo', TARGET_OBJECT, {'CursorTarget.cpp', 'Game.DrawObjects.cpp'}),
    (r'Info\([^)]*\)', DIALOG_BOX, None),
    (r'm_info\[[^\]]*\]', DIALOG_BOX, None),
    (r'\beq', EQUIPMENT, None),
    (r'PlayerAnim::\w+', ANIM_DEF, None),
    (r'from_action\([^)]*\)', ANIM_DEF, None),
    (r'(?:tiles|outTiles)\[[^\]]*\]', SPELL_TILE, None),
    (r'\bparams', SPELL_PARAMS, {'SpellAoE.cpp', 'Screen_OnGame.cpp'}),
    (r'\bparams', ANIM_PARAMS, {'FloatingTextManager.cpp'}),
    (r'get_params\(\)', ANIM_PARAMS, None),
    (r's_slots\[[^\]]*\]', BENCHMARK, None),
]

# ============================================================
# Phase B: Self-file definitions (struct's own .cpp/.h files)
# filename -> (struct_pattern_for_func_detect, renames, param_collisions)
# ============================================================

SELF_FILE_DEFS = {
    'AnimationState.h': ('animation_state', ANIM_STATE, {
        'set_action': {'action', 'dir', 'loop'},
        'set_direction': {'dir'},
    }),
    'AnimationState.cpp': ('animation_state', ANIM_STATE, {
        'set_action': {'action', 'dir', 'loop'},
        'set_direction': {'dir'},
    }),
    'EntityMotion.h': ('EntityMotion', ENTITY_MOTION, {
        'start_move': {'direction', 'duration'},
        'start_move_with_offset': {'direction', 'duration'},
        'queue_move': {'direction', 'duration'},
        'get_direction_start_offset': {'direction'},
    }),
    'EntityMotion.cpp': ('EntityMotion', ENTITY_MOTION, {
        'start_move': {'direction', 'duration'},
        'start_move_with_offset': {'direction', 'duration'},
        'queue_move': {'direction', 'duration'},
        'get_direction_start_offset': {'direction'},
    }),
    'EquipmentIndices.cpp': ('EquipmentIndices', EQUIPMENT, {}),
}

# ============================================================
# Phase C: Header struct body definitions
# filename -> [(struct_name, renames)]
# ============================================================

HEADER_STRUCT_DEFS = {
    'AnimationState.h': [('animation_state', ANIM_STATE)],
    'EntityMotion.h': [('EntityMotion', ENTITY_MOTION)],
    'CursorTarget.h': [('FocusedObject', FOCUSED_OBJECT), ('TargetObjectInfo', TARGET_OBJECT)],
    'DialogBoxInfo.h': [('DialogBoxInfo', DIALOG_BOX)],
    'EquipmentIndices.h': [('EquipmentIndices', EQUIPMENT)],
    'Player.h': [('AnimDef', ANIM_DEF)],
    'SpellAoE.h': [('spell_aoe_tile', SPELL_TILE), ('spell_aoe_params', SPELL_PARAMS)],
    'FloatingTextTypes.h': [('AnimParams', ANIM_PARAMS)],
    'Benchmark.h': [('BenchmarkSlot', BENCHMARK)],
}

# ============================================================
# File I/O
# ============================================================

def read_file(path):
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        return f.read()

def write_file(path, content):
    with open(path, "w", encoding="utf-8", newline="") as f:
        f.write(content)

# ============================================================
# Phase C: Rename member declarations in struct bodies
# ============================================================

def phase_c_header_decls(content, filename, filepath):
    """Rename member declarations inside struct bodies in header files."""
    if filename not in HEADER_STRUCT_DEFS:
        return content
    for struct_name, renames in HEADER_STRUCT_DEFS[filename]:
        content = _rename_in_struct_body(content, struct_name, renames, filepath)
    return content

def _rename_in_struct_body(content, struct_name, renames, filepath):
    """Find struct body and rename bare member names on declaration lines."""
    lines = content.split('\n')
    result = []
    in_struct = False
    brace_depth = 0
    struct_start_depth = 0
    in_signature = False  # Tracks if we're between ( and ) in a function sig

    for i, line in enumerate(lines):
        original = line

        # Detect struct/class start
        if not in_struct and re.search(rf'\bstruct\s+{re.escape(struct_name)}\b', line):
            in_struct = True
            struct_start_depth = brace_depth

        if in_struct:
            # Split code from comment FIRST (parens in comments don't count)
            comment_pos = line.find('//')
            if comment_pos >= 0:
                code_part = line[:comment_pos]
                comment_part = line[comment_pos:]
            else:
                code_part = line
                comment_part = ''

            # Track signature state using CODE portion only
            for ch in code_part:
                if ch == '(':
                    in_signature = True
                elif ch == ')':
                    in_signature = False

            # Count braces AFTER signature tracking
            opens = code_part.count('{')
            closes = code_part.count('}')
            brace_depth += opens - closes

            # Check struct end
            if brace_depth <= struct_start_depth and closes > 0:
                in_struct = False
                result.append(line)
                continue

            # Only rename on lines that are NOT part of function signatures
            # and do NOT contain ( or ) in CODE portion
            if not in_signature and '(' not in code_part and ')' not in code_part:
                for old, new in renames:
                    code_part = re.sub(rf'\b{re.escape(old)}\b', new, code_part)

                line = code_part + comment_part

            if line != original:
                file_changes[str(filepath)].append((i + 1, original.rstrip(), line.rstrip()))
        else:
            # Count braces outside struct too (for nested structs)
            brace_depth += line.count('{') - line.count('}')

        result.append(line)

    return '\n'.join(result)

# ============================================================
# Phase A: Accessor-qualified dot replacement
# ============================================================

def phase_a_accessors(content, filename, filepath):
    """Replace accessor.old_member -> accessor.new_member patterns."""
    lines = content.split('\n')
    result = []

    for i, line in enumerate(lines):
        original = line

        for accessor_re, renames, file_scope in ACCESSOR_RULES:
            if file_scope is not None and filename not in file_scope:
                continue
            for old, new in renames:
                pattern = rf'({accessor_re})\.{re.escape(old)}\b'
                line = re.sub(pattern, rf'\g<1>.{new}', line)

        if line != original:
            file_changes[str(filepath)].append((i + 1, original.rstrip(), line.rstrip()))
        result.append(line)

    return '\n'.join(result)

# ============================================================
# Phase B: Self-file bare member replacement
# ============================================================

def phase_b_self_file(content, filename, filepath):
    """Replace this->member and bare member access in struct method files."""
    if filename not in SELF_FILE_DEFS:
        return content

    struct_name, renames, param_collisions = SELF_FILE_DEFS[filename]
    is_header = filename.endswith('.h')
    lines = content.split('\n')

    # Build function scope map: for each line, which function is it in?
    func_map = _build_function_map(lines, struct_name, is_header)

    result = []
    for i, line in enumerate(lines):
        original = line
        current_func = func_map[i]

        if current_func is not None:
            skip = param_collisions.get(current_func, set())

            # Step 1: Replace this->member -> m_member
            for old, new in renames:
                line = re.sub(rf'this->{re.escape(old)}\b', new, line)

            # Step 2: Bare member replacement
            # Skip members that are parameters in the current function
            # Lookbehind prevents matching after . -> :: or word chars
            for old, new in renames:
                if old in skip:
                    continue
                line = re.sub(rf'(?<![.\w>:]){re.escape(old)}\b', new, line)

        if line != original:
            file_changes[str(filepath)].append((i + 1, original.rstrip(), line.rstrip()))
        result.append(line)

    return '\n'.join(result)


def _build_function_map(lines, struct_name, is_header):
    """Map each line index to the function name it's inside (or None)."""
    func_map = [None] * len(lines)
    current_func = None
    brace_depth = 0
    func_body_depth = -1  # Brace depth where function body starts

    for i, line in enumerate(lines):
        # Detect function definitions
        if current_func is None:
            if is_header:
                # In headers: inline methods inside struct body
                # Match: word(stuff) [const] {  on one line
                m = re.search(r'\b(\w+)\s*\([^)]*\)\s*(?:const\s*)?(?:->.*?)?\s*\{', line)
                if m and brace_depth >= 1:
                    candidate = m.group(1)
                    if candidate not in ('if', 'while', 'for', 'switch', 'return',
                                         'struct', 'class', 'sizeof', 'decltype',
                                         'static_cast', 'dynamic_cast', 'reinterpret_cast'):
                        current_func = candidate
                        func_body_depth = brace_depth + 1
            else:
                # In .cpp: struct_name::method(
                m = re.search(rf'{re.escape(struct_name)}::(\w+)\s*\(', line)
                if m and brace_depth == 0:
                    current_func = m.group(1)
                    func_body_depth = 1

        # Set function map BEFORE counting braces
        # This includes the function definition line itself
        func_map[i] = current_func

        # Count braces
        for ch in line:
            if ch == '{':
                brace_depth += 1
            elif ch == '}':
                brace_depth -= 1
                if current_func is not None and brace_depth < func_body_depth:
                    current_func = None
                    func_body_depth = -1

    return func_map

# ============================================================
# Main processing
# ============================================================

def collect_client_files():
    """Collect all .h and .cpp files under Sources/Client/."""
    files = []
    for ext in ('*.h', '*.cpp'):
        files.extend(BASE.glob(ext))
    return sorted(files)


def process_file(filepath):
    """Process a single file with all three phases."""
    filename = filepath.name
    content = read_file(filepath)
    original = content

    # Phase C: Header struct declarations
    content = phase_c_header_decls(content, filename, filepath)

    # Phase A: Accessor-qualified renames
    content = phase_a_accessors(content, filename, filepath)

    # Phase B: Self-file bare renames
    content = phase_b_self_file(content, filename, filepath)

    if content != original:
        if not DRY_RUN and not VERIFY:
            write_file(filepath, content)
        return True
    return False


def verify_mode():
    """Check for potential conflicts and issues."""
    print("=== VERIFICATION MODE ===\n")
    issues = []

    # Check: 'info.' outside scoped files
    print("Checking accessor pattern scoping...")
    for filepath in collect_client_files():
        fn = filepath.name
        content = read_file(filepath)

        if fn not in {'CursorTarget.cpp', 'Game.DrawObjects.cpp'}:
            for old, new in TARGET_OBJECT:
                if re.search(rf'\binfo\.{re.escape(old)}\b', content):
                    issues.append(f"  {fn}: 'info.{old}' outside TargetObjectInfo scope")

        if fn not in {'SpellAoE.cpp', 'Screen_OnGame.cpp', 'FloatingTextManager.cpp'}:
            for old, new in SPELL_PARAMS:
                if re.search(rf'\bparams\.{re.escape(old)}\b', content):
                    issues.append(f"  {fn}: 'params.{old}' outside spell_aoe_params scope")
            for old, new in ANIM_PARAMS:
                if re.search(rf'\bparams\.{re.escape(old)}\b', content):
                    issues.append(f"  {fn}: 'params.{old}' outside AnimParams scope")

        # Check 'eq.' outside expected files
        if fn not in {'EquipmentIndices.cpp', 'PlayerRenderer.cpp', 'NpcRenderer.cpp',
                      'RenderHelpers.cpp', 'Game.DrawObjects.cpp', 'EquipmentIndices.h'}:
            for old, new in EQUIPMENT:
                if re.search(rf'\beq\.{re.escape(old)}\b', content):
                    issues.append(f"  {fn}: 'eq.{old}' in unexpected file")

    if issues:
        print(f"\n{len(issues)} potential issues:")
        for iss in issues:
            print(iss)
    else:
        print("No issues found. Safe to proceed.")
    return len(issues) == 0


def report():
    """Print report of all changes."""
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    log_path = OUTPUT_DIR / "member_prefix_dry_run.log"

    total = 0
    with open(log_path, "w", encoding="utf-8") as f:
        f.write("=" * 80 + "\n")
        f.write("phase_member_prefix.py DRY RUN REPORT\n")
        f.write("=" * 80 + "\n\n")

        for fpath in sorted(file_changes.keys()):
            clist = file_changes[fpath]
            if not clist:
                continue
            try:
                rel = Path(fpath).relative_to(BASE)
            except ValueError:
                rel = fpath
            f.write(f"\n--- {rel} ({len(clist)} changes) ---\n")
            for lnum, old, new in clist:
                f.write(f"  L{lnum}:\n")
                f.write(f"    - {old}\n")
                f.write(f"    + {new}\n")
                total += 1

        f.write(f"\n{'=' * 80}\n")
        f.write(f"TOTAL: {total} changes across {len(file_changes)} files\n")

    print(f"\nDry-run log: {log_path}")
    print(f"Files: {len(file_changes)}, Changes: {total}")
    print()
    for fpath in sorted(file_changes.keys()):
        clist = file_changes[fpath]
        if clist:
            try:
                rel = Path(fpath).relative_to(BASE)
            except ValueError:
                rel = fpath
            print(f"  {rel}: {len(clist)}")


def main():
    print("phase_member_prefix.py - Add m_ prefix to client struct data members")
    print(f"Base: {BASE}")

    if VERIFY:
        ok = verify_mode()
        sys.exit(0 if ok else 1)

    mode = "DRY RUN" if DRY_RUN else "APPLY"
    print(f"Mode: {mode}\n")

    files = collect_client_files()
    print(f"Scanning {len(files)} files...")

    changed = 0
    for fp in files:
        if process_file(fp):
            changed += 1

    print(f"\n{changed} files {'would be' if DRY_RUN else ''} modified.")
    report()


if __name__ == "__main__":
    main()

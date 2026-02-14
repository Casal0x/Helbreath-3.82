#!/usr/bin/env python3
"""
Fixup script for phase_member_prefix.py missed access patterns.

The main script missed these accessor patterns:
  - DialogBoxInfo: info., dropInfo., giveInfo., mfg., m_info[...].
  - FocusedObject: focused. (local const ref)
  - AnimDef: def. (local const ref from PlayerAnim::from_action())
  - Game.cpp: m_dialog_box_manager.Info(CursorTarget::get_selected_id()).
"""

import re
import sys
import os

BASE = os.path.join(os.path.dirname(__file__), "..", "Sources", "Client")

# DialogBoxInfo member renames (old -> new)
DBI_RENAMES = {
    "v1": "m_v1", "v2": "m_v2", "v3": "m_v3", "v4": "m_v4",
    "v5": "m_v5", "v6": "m_v6", "v7": "m_v7", "v8": "m_v8",
    "v9": "m_v9", "v10": "m_v10", "v11": "m_v11", "v12": "m_v12",
    "v13": "m_v13", "v14": "m_v14",
    "dw_v1": "m_dw_v1", "dw_v2": "m_dw_v2", "dw_t1": "m_dw_t1",
    "flag": "m_flag",
    "x": "m_x", "y": "m_y",
    "size_x": "m_size_x", "size_y": "m_size_y",
    "view": "m_view",
    "str": "m_str", "str2": "m_str2", "str3": "m_str3", "str4": "m_str4",
    "mode": "m_mode",
    "is_scroll_selected": "m_is_scroll_selected",
    "can_close_on_right_click": "m_can_close_on_right_click",
}

# FocusedObject member renames
FOCUSED_RENAMES = {
    "valid": "m_valid", "objectID": "m_object_id",
    "mapX": "m_map_x", "mapY": "m_map_y",
    "screenX": "m_screen_x", "screenY": "m_screen_y",
    "dataX": "m_data_x", "dataY": "m_data_y",
    "type": "m_type", "ownerType": "m_owner_type",
    "action": "m_action", "direction": "m_direction",
    "frame": "m_frame", "name": "m_name",
    "appearance": "m_appearance", "status": "m_status",
}

# AnimDef member renames
ANIMDEF_RENAMES = {
    "max_frame": "m_max_frame",
    "frame_time": "m_frame_time",
    "loop": "m_loop",
}

def apply_accessor_renames(lines, accessor_regex, renames, file_label=""):
    """Apply renames for a specific accessor pattern."""
    changes = 0
    # Sort by length descending to prevent partial matches (e.g., str2 before str)
    sorted_renames = sorted(renames.items(), key=lambda x: len(x[0]), reverse=True)

    for i, line in enumerate(lines):
        original = line
        for old, new in sorted_renames:
            # Pattern: accessor.old_member (with word boundary after)
            pattern = rf'({accessor_regex}\.){re.escape(old)}\b'
            # Make sure we're not already renamed (don't match m_xxx)
            line = re.sub(pattern, lambda m: m.group(1) + new, line)
        if line != original:
            lines[i] = line
            changes += 1
    return changes

def process_file(filepath, rules, dry_run=False):
    """Process a single file with the given rules."""
    if not os.path.exists(filepath):
        print(f"  SKIP (not found): {filepath}")
        return 0

    with open(filepath, 'r', encoding='utf-8', errors='replace') as f:
        lines = f.readlines()

    original_lines = list(lines)
    total_changes = 0

    for accessor_regex, renames, label in rules:
        changes = apply_accessor_renames(lines, accessor_regex, renames, label)
        total_changes += changes

    if total_changes > 0:
        if dry_run:
            print(f"  {os.path.basename(filepath)}: {total_changes} lines changed")
            for i, (orig, new) in enumerate(zip(original_lines, lines)):
                if orig != new:
                    print(f"    L{i+1}: {orig.rstrip()}")
                    print(f"      -> {new.rstrip()}")
        else:
            with open(filepath, 'w', encoding='utf-8', newline='') as f:
                f.writelines(lines)
            print(f"  {os.path.basename(filepath)}: {total_changes} lines changed")

    return total_changes

def main():
    dry_run = "--dry-run" in sys.argv

    if dry_run:
        print("=== DRY RUN MODE ===\n")

    total = 0

    # --- DialogBox_ChatHistory.cpp: info.member ---
    total += process_file(
        os.path.join(BASE, "DialogBox_ChatHistory.cpp"),
        [(r'info', DBI_RENAMES, "info.")],
        dry_run
    )

    # --- DialogBox_Bank.cpp: giveInfo.member, dropInfo.member ---
    total += process_file(
        os.path.join(BASE, "DialogBox_Bank.cpp"),
        [
            (r'giveInfo', DBI_RENAMES, "giveInfo."),
            (r'dropInfo', DBI_RENAMES, "dropInfo."),
        ],
        dry_run
    )

    # --- DialogBox_Exchange.cpp: dropInfo.member ---
    total += process_file(
        os.path.join(BASE, "DialogBox_Exchange.cpp"),
        [(r'dropInfo', DBI_RENAMES, "dropInfo.")],
        dry_run
    )

    # --- DialogBox_Manufacture.cpp: info.member ---
    total += process_file(
        os.path.join(BASE, "DialogBox_Manufacture.cpp"),
        [(r'info', DBI_RENAMES, "info.")],
        dry_run
    )

    # --- DialogBox_SellList.cpp: dropInfo.member ---
    total += process_file(
        os.path.join(BASE, "DialogBox_SellList.cpp"),
        [(r'dropInfo', DBI_RENAMES, "dropInfo.")],
        dry_run
    )

    # --- IDialogBox.cpp: info.member ---
    total += process_file(
        os.path.join(BASE, "IDialogBox.cpp"),
        [(r'info', DBI_RENAMES, "info.")],
        dry_run
    )

    # --- DialogBoxManager.cpp: mfg.member, info.member (local ref to m_info[]) ---
    total += process_file(
        os.path.join(BASE, "DialogBoxManager.cpp"),
        [
            (r'mfg', DBI_RENAMES, "mfg."),
            (r'info', DBI_RENAMES, "info."),
        ],
        dry_run
    )

    # --- Game.cpp: focused.member, m_dialog_box_manager.Info(...).member ---
    total += process_file(
        os.path.join(BASE, "Game.cpp"),
        [
            (r'focused', FOCUSED_RENAMES, "focused."),
            # Info(CursorTarget::get_selected_id()).x/y — nested parens
            (r'm_dialog_box_manager\.Info\([^()]*(?:\([^()]*\)[^()]*)*\)', DBI_RENAMES, "Info()."),
        ],
        dry_run
    )

    # --- MapData.cpp: def.member (AnimDef) ---
    total += process_file(
        os.path.join(BASE, "MapData.cpp"),
        [(r'def', ANIMDEF_RENAMES, "def.")],
        dry_run
    )

    print(f"\nTotal: {total} lines changed across files")
    if dry_run:
        print("\nRe-run without --dry-run to apply.")

if __name__ == "__main__":
    main()

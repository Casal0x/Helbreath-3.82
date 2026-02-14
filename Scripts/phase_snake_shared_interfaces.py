#!/usr/bin/env python3
"""Phase 7: Shared interface method renames
Target: IRenderer (~41), ISpriteFactory (~15), ResolutionConfig (~23),
        TextLib (~24), ConcurrentMsgQueue (~7), IOServicePool (~2) = ~112 methods.
Atomic: shared declarations + SFMLEngine implementations + client consumers.
Mode 2 justified: ~112 methods across ~80+ files in 3 modules.
"""

import re
import os
import sys

BASE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# ═══════════════════════════════════════════════════════════════════════════
# SCOPED RENAMES — class-qualified to avoid collisions
# Pattern: ExactOld → ExactNew (simple string replace)
# ═══════════════════════════════════════════════════════════════════════════
SCOPED_RENAMES = [
    # TextStyle::Color → from_color (NOT color — collides with data member 'color')
    ("TextStyle::Color", "TextStyle::from_color"),
    ("TextStyle::Transparent", "TextStyle::transparent"),
    # Sprites — Create/Destroy are too generic for word-boundary
    ("Sprites::Create", "Sprites::create"),
    ("Sprites::Destroy", "Sprites::destroy"),
]

# ═══════════════════════════════════════════════════════════════════════════
# DEFINITION FIXES — unqualified declarations inside class bodies
# These are exact string replacements applied only to matching filenames.
# Needed because SCOPED_RENAMES only match "Class::Method" (qualified form),
# but inside the class body the declaration is just "Method" (unqualified).
# ═══════════════════════════════════════════════════════════════════════════
DEFINITION_FIXES = [
    ("ISpriteFactory.h", "static ISprite* Create(", "static ISprite* create("),
    ("ISpriteFactory.h", "static void Destroy(", "static void destroy("),
    ("TextLib.h", "static constexpr TextStyle Color(", "static constexpr TextStyle from_color("),
    ("TextLib.h", "static constexpr TextStyle Transparent(", "static constexpr TextStyle transparent("),
]

# ═══════════════════════════════════════════════════════════════════════════
# SAFE RENAMES — distinctive PascalCase method names (word-boundary)
# All other interface methods. Sorted longest-first.
# ═══════════════════════════════════════════════════════════════════════════
SAFE_RENAMES = sorted([
    # ── IRenderer ──
    ("EndFrameCheckLostSurface", "end_frame_check_lost_surface"),
    ("DrawRoundedRectOutline", "draw_rounded_rect_outline"),
    ("DrawRoundedRectFilled", "draw_rounded_rect_filled"),
    ("SetFullscreenStretch", "set_fullscreen_stretch"),
    ("IsFullscreenStretch", "is_fullscreen_stretch"),
    ("GetAmbientLightLevel", "get_ambient_light_level"),
    ("SetAmbientLightLevel", "set_ambient_light_level"),
    ("GetBackBufferNative", "get_back_buffer_native"),
    ("ChangeDisplayMode", "change_display_mode"),
    ("WasFramePresented", "was_frame_presented"),
    ("GetNativeRenderer", "get_native_renderer"),
    ("ColorTransferRGB", "color_transfer_rgb"),
    ("ResizeBackBuffer", "resize_back_buffer"),
    ("DrawRectOutline", "draw_rect_outline"),
    ("DrawRectFilled", "draw_rect_filled"),
    ("DestroyTexture", "destroy_texture"),
    ("CreateTexture", "create_texture"),
    ("BeginTextBatch", "begin_text_batch"),
    ("EndTextBatch", "end_text_batch"),
    ("SetFullscreen", "set_fullscreen"),
    ("IsFullscreen", "is_fullscreen"),
    ("GetTextLength", "get_text_length"),
    ("GetTextWidth", "get_text_width"),
    ("DrawTextRect", "draw_text_rect"),
    ("GetDeltaTimeMS", "get_delta_time_ms"),
    ("GetDeltaTime", "get_delta_time"),
    ("SetClipArea", "set_clip_area"),
    ("GetClipArea", "get_clip_area"),
    ("GetWidthMid", "get_width_mid"),
    ("GetHeightMid", "get_height_mid"),
    ("BeginFrame", "begin_frame"),
    ("Screenshot", "screenshot"),
    ("GetHeight", "get_height"),
    ("EndFrame", "end_frame"),
    ("DrawPixel", "draw_pixel"),
    ("DrawLine", "draw_line"),
    ("GetWidth", "get_width"),
    ("GetFPS", "get_fps"),

    # ── ISpriteFactory + Sprites static ──
    ("CreateSpriteFromData", "create_sprite_from_data"),
    ("CreateSprite", "create_sprite"),
    ("DestroySprite", "destroy_sprite"),
    ("GetSpriteCount", "get_sprite_count"),
    ("GetSpritePath", "get_sprite_path"),
    ("SetFactory", "set_factory"),
    ("GetFactory", "get_factory"),

    # ── ResolutionConfig ──
    ("RecalculateScreenOffset", "recalculate_screen_offset"),
    ("ViewCenterTileX", "view_center_tile_x"),
    ("ViewCenterTileY", "view_center_tile_y"),
    ("IsHighResolution", "is_high_resolution"),
    ("IconPanelOffsetX", "icon_panel_offset_x"),
    ("EventList2BaseY", "event_list2_base_y"),
    ("IconPanelHeight", "icon_panel_height"),
    ("IconPanelWidth", "icon_panel_width"),
    ("ViewTileHeight", "view_tile_height"),
    ("ViewTileWidth", "view_tile_width"),
    ("LevelUpTextX", "level_up_text_x"),
    ("LevelUpTextY", "level_up_text_y"),
    ("SetWindowSize", "set_window_size"),
    ("LogicalHeight", "logical_height"),
    ("LogicalWidth", "logical_width"),
    ("LogicalMaxX", "logical_max_x"),
    ("LogicalMaxY", "logical_max_y"),
    ("MenuOffsetX", "menu_offset_x"),
    ("MenuOffsetY", "menu_offset_y"),
    ("ChatInputX", "chat_input_x"),
    ("ChatInputY", "chat_input_y"),
    ("ScreenX", "screen_x"),
    ("ScreenY", "screen_y"),

    # ── TextLib — TextStyle builder methods ──
    ("WithIntegratedShadow", "with_integrated_shadow"),
    ("WithTwoPointShadow", "with_two_point_shadow"),
    ("WithShadowStyle", "with_shadow_style"),
    ("WithDropShadow", "with_drop_shadow"),
    ("WithHighlight", "with_highlight"),
    ("WithFontSize", "with_font_size"),
    ("WithAdditive", "with_additive"),
    ("WithShadow", "with_shadow"),
    ("WithAlpha", "with_alpha"),

    # ── TextLib — free functions ──
    ("MeasureWrappedTextHeight", "measure_wrapped_text_height"),
    ("LoadBitmapFontDynamic", "load_bitmap_font_dynamic"),
    ("GetFittingCharCount", "get_fitting_char_count"),
    ("IsBitmapFontLoaded", "is_bitmap_font_loaded"),
    ("DrawTextAligned", "draw_text_aligned"),
    ("DrawTextWrapped", "draw_text_wrapped"),
    ("LoadBitmapFont", "load_bitmap_font"),
    ("GetBitmapFont", "get_bitmap_font"),
    ("GetLineHeight", "get_line_height"),
    ("MeasureText", "measure_text"),
    ("BeginBatch", "begin_batch"),
    ("EndBatch", "end_batch"),

    # ── ConcurrentMsgQueue + ConcurrentQueue ──
    ("Push", "push"),
    ("Pop", "pop"),
    ("Size", "size"),
    ("Empty", "empty"),

    # ── IOServicePool ──
    ("Start", "start"),
    ("Stop", "stop"),

    # ── Common singleton/lifecycle ──
    ("Init", "init"),
    ("Shutdown", "shutdown"),
    ("Get", "get"),
    ("DrawText", "draw_text"),
    ("Draw", "draw"),
], key=lambda x: -len(x[0]))


def collect_files():
    """Collect all source files across all modules."""
    files = []
    for d in ["Sources/Dependencies/Shared", "Sources/SFMLEngine",
              "Sources/Client", "Sources/Server"]:
        dirpath = os.path.join(BASE, d)
        if not os.path.isdir(dirpath):
            continue
        for root, dirs, names in os.walk(dirpath):
            for name in sorted(names):
                if name.endswith((".h", ".cpp")):
                    files.append(os.path.join(root, name))
    return sorted(files)


def apply_renames(files, dry_run=False):
    """Apply all renames to files."""
    # Pre-compile word-boundary patterns
    safe_patterns = [(re.compile(r"\b" + re.escape(old) + r"\b"), new)
                     for old, new in SAFE_RENAMES]

    modified_files = []
    total_changes = 0

    for fpath in files:
        with open(fpath, "r", encoding="utf-8") as f:
            lines = f.readlines()

        # Pre-filter DEFINITION_FIXES for this file
        fname = os.path.basename(fpath)
        file_def_fixes = [(old, new) for (fn, old, new) in DEFINITION_FIXES
                          if fn == fname]

        new_lines = []
        file_changes = 0

        for line in lines:
            original_line = line
            stripped = line.lstrip()

            # Skip #include and #pragma lines
            if stripped.startswith("#include") or stripped.startswith("#pragma"):
                new_lines.append(line)
                continue

            # Apply DEFINITION_FIXES first (exact string, file-specific)
            for old, new in file_def_fixes:
                if old in line:
                    line = line.replace(old, new)

            # Apply SCOPED renames (exact string match)
            for old, new in SCOPED_RENAMES:
                if old in line:
                    line = line.replace(old, new)

            # Apply SAFE renames (word-boundary)
            for pattern, new in safe_patterns:
                line = pattern.sub(new, line)

            if line != original_line:
                file_changes += 1
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


def main():
    dry_run = "--dry-run" in sys.argv
    mode = "DRY RUN" if dry_run else "LIVE"
    print(f"Phase 7: Shared interface method renames [{mode}]")
    print(f"  SCOPED renames: {len(SCOPED_RENAMES)}")
    print(f"  DEFINITION fixes: {len(DEFINITION_FIXES)}")
    print(f"  SAFE renames: {len(SAFE_RENAMES)}")
    print()

    files = collect_files()
    print(f"Scanning {len(files)} source files...")
    print()

    mod_files, changes = apply_renames(files, dry_run)
    print()
    print(f"Summary: {len(mod_files)} files modified, ~{changes} changes")

    return 0


if __name__ == "__main__":
    sys.exit(main())

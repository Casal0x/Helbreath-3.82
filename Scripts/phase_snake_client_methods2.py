#!/usr/bin/env python3
"""Phase 8: Client PascalCase method renames (remaining after Phase 7).
Target: ~206 unique methods across ~49 header files + their .cpp consumers.
Mode 2 justified: 206 methods across ~100+ files.

Covers: CursorTarget, ChatManager, FrameTiming, Benchmark, DialogBox_*,
        EntityRenderState, EffectManager, ConfigManager, LocalCacheManager,
        Screen_*, Overlay_*, Version, Misc, and misc small classes.
"""

import re
import os
import sys

BASE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# ═══════════════════════════════════════════════════════════════════════════
# SAFE RENAMES — distinctive PascalCase method names (word-boundary)
# Sorted longest-first to prevent partial matches.
# ═══════════════════════════════════════════════════════════════════════════
SAFE_RENAMES = sorted([
    # ── CursorTarget (namespace functions) ──
    ("TestObject", "test_object"),
    ("TestGroundItem", "test_ground_item"),
    ("TestDynamicObject", "test_dynamic_object"),
    ("HasFocusedObject", "has_focused_object"),
    ("GetCursorFrame", "get_cursor_frame"),
    ("GetFocusedMapX", "get_focused_map_x"),
    ("GetFocusedMapY", "get_focused_map_y"),
    ("GetFocusedName", "get_focused_name"),
    ("IsOverGroundItem", "is_over_ground_item"),
    ("GetFocusHighlightData", "get_focus_highlight_data"),
    ("PointInRect", "point_in_rect"),
    ("PointInCircle", "point_in_circle"),
    ("SetSelection", "set_selection"),
    ("ClearSelection", "clear_selection"),
    ("GetSelectedID", "get_selected_id"),
    ("GetDragDistX", "get_drag_dist_x"),
    ("GetDragDistY", "get_drag_dist_y"),
    ("HasSelection", "has_selection"),
    ("RecordSelectionClick", "record_selection_click"),
    ("ResetSelectionClickTime", "reset_selection_click_time"),
    ("GetSelectionClickTime", "get_selection_click_time"),
    ("GetSelectionClickX", "get_selection_click_x"),
    ("GetSelectionClickY", "get_selection_click_y"),
    ("SetPrevPosition", "set_prev_position"),
    ("GetPrevX", "get_prev_x"),
    ("GetPrevY", "get_prev_y"),
    ("SetCursorStatus", "set_cursor_status"),

    # ── ChatManager ──
    ("AddMessage", "add_message"),
    ("ClearMessages", "clear_messages"),
    ("AddWhisperTarget", "add_whisper_target"),
    ("ClearWhispers", "clear_whispers"),
    ("GetWhisperTargetName", "get_whisper_target_name"),
    ("HasWhisperTarget", "has_whisper_target"),
    ("GetWhisperIndex", "get_whisper_index"),
    ("SetWhisperIndex", "set_whisper_index"),
    ("CycleWhisperUp", "cycle_whisper_up"),
    ("CycleWhisperDown", "cycle_whisper_down"),
    ("IsWhisperEnabled", "is_whisper_enabled"),
    ("SetWhisperEnabled", "set_whisper_enabled"),
    ("IsShoutEnabled", "is_shout_enabled"),
    ("SetShoutEnabled", "set_shout_enabled"),

    # ── FrameTiming ──
    ("SetProfilingEnabled", "set_profiling_enabled"),
    ("IsProfilingEnabled", "is_profiling_enabled"),
    ("SetFrameRendered", "set_frame_rendered"),
    ("BeginProfile", "begin_profile"),
    ("EndProfile", "end_profile"),
    ("GetProfileTimeMS", "get_profile_time_ms"),
    ("GetProfileAvgTimeMS", "get_profile_avg_time_ms"),
    ("GetStageName", "get_stage_name"),

    # ── Benchmark / DebugConsole ──
    ("Allocate", "allocate"),
    ("Deallocate", "deallocate"),
    ("IsAllocated", "is_allocated"),
    ("PrintMemory", "print_memory"),
    ("ResetMemoryDelta", "reset_memory_delta"),
    ("FindOrCreateSlot", "find_or_create_slot"),

    # ── EntityRenderState ──
    ("IsPlayer", "is_player"),
    ("IsNPC", "is_npc"),
    ("IsMale", "is_male"),
    ("IsFemale", "is_female"),

    # ── EffectManager ──
    ("AddEffect", "add_effect"),

    # ── ConfigManager ──
    ("SetDefaults", "set_defaults"),

    # ── CommonTypes ──
    ("GetTimeMS", "get_time_ms"),

    # ── HotkeyManager ──
    ("Register", "register_hotkey"),

    # ── InventoryManager ──
    ("EquipItem", "equip_item"),

    # ── FloatingTextManager ──
    # Clear handled below in generic section

    # ── LocalCacheManager ──
    ("HasCache", "has_cache"),
    ("GetHash", "get_hash"),
    ("AccumulatePacket", "accumulate_packet"),
    ("FinalizeAndSave", "finalize_and_save"),
    ("ReplayFromCache", "replay_from_cache"),
    ("ResetAccumulator", "reset_accumulator"),
    ("IsReplaying", "is_replaying"),

    # ── Misc.h ──
    ("GetPoint", "get_point"),
    ("ReplaceString", "replace_string"),

    # ── EquipmentIndices ──
    ("CalcColors", "calc_colors"),

    # ── Version ──
    ("GetSemVer", "get_sem_ver"),
    ("GetDisplayString", "get_display_string"),
    ("GetFullString", "get_full_string"),

    # ── Game.h (previously skipped) ──
    ("CannotConstruct", "cannot_construct"),
    ("GrandMagicResult", "grand_magic_result"),
    ("MeteorStrikeComing", "meteor_strike_coming"),

    # ── DialogBox_Bank ──
    ("DrawItemList", "draw_item_list"),
    ("DrawItemDetails", "draw_item_details"),
    ("DrawScrollbar", "draw_scrollbar"),

    # ── DialogBox_ChangeStatsMajestic ──
    ("DrawStatRow", "draw_stat_row"),

    # ── DialogBox_Character ──
    ("DrawStat", "draw_stat"),
    ("DrawEquippedItem", "draw_equipped_item"),
    ("DrawHoverButton", "draw_hover_button"),
    ("DrawMaleCharacter", "draw_male_character"),
    ("DrawFemaleCharacter", "draw_female_character"),
    ("BuildEquipStatusArray", "build_equip_status_array"),
    ("FindEquipItemAtPoint", "find_equip_item_at_point"),

    # ── DialogBox_ChatHistory ──
    ("DrawScrollBar", "draw_scroll_bar"),
    ("DrawChatMessages", "draw_chat_messages"),
    ("HandleScrollInput", "handle_scroll_input"),

    # ── DialogBox_CityHallMenu / GuildMenu (shared names) ──
    ("OnClickMode0", "on_click_mode0"),
    ("OnClickMode1", "on_click_mode1"),
    ("OnClickMode5", "on_click_mode5"),
    ("OnClickMode7", "on_click_mode7"),
    ("OnClickMode8", "on_click_mode8"),
    ("OnClickMode9", "on_click_mode9"),
    ("OnClickMode10", "on_click_mode10"),
    ("OnClickMode11", "on_click_mode11"),
    ("OnClickMode13", "on_click_mode13"),
    ("OnClickModeOkOnly", "on_click_mode_ok_only"),

    # ── DialogBox_CrusadeJob ──
    ("DrawModeSelectJob", "draw_mode_select_job"),
    ("DrawModeConfirm", "draw_mode_confirm"),

    # ── DialogBox_Exchange ──
    ("DrawItems", "draw_items"),
    ("DrawItemInfo", "draw_item_info"),

    # ── DialogBox_GuideMap ──
    ("DrawBorder", "draw_border"),
    ("DrawZoomedMap", "draw_zoomed_map"),
    ("DrawFullMap", "draw_full_map"),
    ("DrawLocationTooltip", "draw_location_tooltip"),

    # ── DialogBox_GuildMenu ──
    ("DrawSimpleMessage", "draw_simple_message"),

    # ── DialogBox_GuildOperation ──
    ("DrawJoinRequest", "draw_join_request"),
    ("DrawDismissRequest", "draw_dismiss_request"),
    ("DrawInfoMessage", "draw_info_message"),

    # ── DialogBox_Help ──
    ("IsMouseOverItem", "is_mouse_over_item"),
    ("DrawHelpItem", "draw_help_item"),

    # ── DialogBox_HudPanel ──
    ("HudYOffset", "hud_y_offset"),
    ("HudXOffset", "hud_x_offset"),
    ("DrawGaugeBars", "draw_gauge_bars"),
    ("DrawIconButtons", "draw_icon_buttons"),
    ("DrawStatusIcons", "draw_status_icons"),
    ("IsInButton", "is_in_button"),
    ("ToggleDialogWithSound", "toggle_dialog_with_sound"),

    # ── DialogBox_Inventory ──
    ("DrawInventoryItem", "draw_inventory_item"),

    # ── DialogBox_ItemUpgrade ──
    ("DrawItemPreview", "draw_item_preview"),
    ("CalculateUpgradeCost", "calculate_upgrade_cost"),

    # ── DialogBox_LevelUpSetting ──
    ("HandleStatClick", "handle_stat_click"),

    # ── DialogBox_MagicShop ──
    ("DrawSpellList", "draw_spell_list"),
    ("DrawPageIndicator", "draw_page_indicator"),
    ("HandleSpellClick", "handle_spell_click"),
    ("HandlePageClick", "handle_page_click"),

    # ── DialogBox_Manufacture ──
    ("DrawAlchemyWaiting", "draw_alchemy_waiting"),
    ("DrawAlchemyCreating", "draw_alchemy_creating"),
    ("DrawManufactureList", "draw_manufacture_list"),
    ("DrawManufactureWaiting", "draw_manufacture_waiting"),
    ("DrawManufactureInProgress", "draw_manufacture_in_progress"),
    ("DrawManufactureDone", "draw_manufacture_done"),
    ("DrawCraftingWaiting", "draw_crafting_waiting"),
    ("DrawCraftingInProgress", "draw_crafting_in_progress"),
    ("ResetItemSlots", "reset_item_slots"),
    ("CheckSlotItemClick", "check_slot_item_click"),
    ("TryAddItemToSlot", "try_add_item_to_slot"),

    # ── DialogBox_NpcActionQuery ──
    ("DrawHighlightedText", "draw_highlighted_text"),

    # ── DialogBox_NpcTalk ──
    ("GetTotalLines", "get_total_lines"),
    ("DrawButtons", "draw_buttons"),
    ("DrawTextContent", "draw_text_content"),
    ("HandleScrollBarDrag", "handle_scroll_bar_drag"),

    # ── DialogBox_SellList ──
    ("DrawEmptyListMessage", "draw_empty_list_message"),

    # ── DialogBox_Shop ──
    ("DrawWeaponStats", "draw_weapon_stats"),
    ("DrawShieldStats", "draw_shield_stats"),
    ("DrawArmorStats", "draw_armor_stats"),
    ("DrawLevelRequirement", "draw_level_requirement"),
    ("DrawQuantitySelector", "draw_quantity_selector"),
    ("CalculateDiscountedPrice", "calculate_discounted_price"),
    ("OnClickItemList", "on_click_item_list"),
    ("OnClickItemDetails", "on_click_item_details"),

    # ── DialogBox_SysMenu ──
    ("GetCurrentResolutionIndex", "get_current_resolution_index"),
    ("GetNearestResolutionIndex", "get_nearest_resolution_index"),
    ("CycleResolution", "cycle_resolution"),
    ("ApplyResolution", "apply_resolution"),
    ("DrawTabs", "draw_tabs"),
    ("DrawTabContent", "draw_tab_content"),
    ("DrawGeneralTab", "draw_general_tab"),
    ("DrawGraphicsTab", "draw_graphics_tab"),
    ("DrawAudioTab", "draw_audio_tab"),
    ("DrawSystemTab", "draw_system_tab"),
    ("OnClickGeneral", "on_click_general"),
    ("OnClickGraphics", "on_click_graphics"),
    ("OnClickAudio", "on_click_audio"),
    ("OnClickSystem", "on_click_system"),
    ("DrawToggle", "draw_toggle"),
    ("IsInToggleArea", "is_in_toggle_area"),

    # ── Overlay_ChangePassword ──
    ("UpdateFocusedInput", "update_focused_input"),
    ("HandleSubmit", "handle_submit"),
    ("ValidateInputs", "validate_inputs"),

    # ── Overlay_LogResMsg ──
    ("HandleDismiss", "handle_dismiss"),
    ("RenderMessage", "render_message"),

    # ── Screen_Loading ──
    ("GetProgress", "get_progress"),
    ("MakeSprite", "make_sprite"),
    ("MakeTileSpr", "make_tile_spr"),
    ("MakeEffectSpr", "make_effect_spr"),

    # ── Screen_Login ──
    ("AttemptLogin", "attempt_login"),
    ("DrawLoginWindow", "draw_login_window"),

    # ── Screen_OnGame ──
    ("RenderItemTooltip", "render_item_tooltip"),
    ("DrawTileGrid", "draw_tile_grid"),
    ("DrawPatchingGrid", "draw_patching_grid"),
    ("DrawSpellTargetOverlay", "draw_spell_target_overlay"),

    # ── Screen_SelectCharacter ──
    ("EnterGame", "enter_game"),

    # ── Screen_Splash ──
    ("GetContributorAlpha", "get_contributor_alpha"),

    # ── Screen_Test ──
    ("LoadBitmapFonts", "load_bitmap_fonts"),
    ("RenderHeader", "render_header"),
    ("RenderTestRow", "render_test_row"),
    ("RenderAlignmentShowcase", "render_alignment_showcase"),

    # ── Screen_TestPrimitives ──
    ("RenderPixelTests", "render_pixel_tests"),
    ("RenderLineAlphaTests", "render_line_alpha_tests"),
    ("RenderLineAdditiveTests", "render_line_additive_tests"),
    ("RenderRectFilledTests", "render_rect_filled_tests"),
    ("RenderRectOutlineTests", "render_rect_outline_tests"),
    ("RenderRoundedRectFilledTests", "render_rounded_rect_filled_tests"),
    ("RenderRoundedRectOutlineTests", "render_rounded_rect_outline_tests"),

    # ── Generic short names (distinctive enough with word-boundary) ──
    ("Clear", "clear"),
    ("Load", "load"),
    ("Save", "save"),
    ("Clamp", "clamp"),
], key=lambda x: -len(x[0]))


def collect_files():
    """Collect source files from Client directory only."""
    files = []
    for d in ["Sources/Client"]:
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

        new_lines = []
        file_changes = 0

        for line in lines:
            original_line = line
            stripped = line.lstrip()

            # Skip #include and #pragma lines
            if stripped.startswith("#include") or stripped.startswith("#pragma"):
                new_lines.append(line)
                continue

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
    print(f"Phase 8: Client PascalCase method renames [{mode}]")
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

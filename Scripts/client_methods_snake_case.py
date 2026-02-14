#!/usr/bin/env python3
"""
Client method snake_case conversion: PascalCase methods → snake_case.

Mode 2 script — justified: ~300 method renames across ~160 client source files.

Companion to client_snake_case.py which handled types, variables, and member data.
This script handles METHOD names that were excluded from the first script.

Deliberately SKIPPED (SFMLEngine / Shared / keyword collisions):
- Draw (bare)    — ISprite::Draw used 617 times from client code
- Init (bare)    — IRenderer::Init called from client code
- Clear (bare)   — Shared structs PlayerStatus::Clear(), PlayerAppearance::Clear()
- Get (bare)     — ResolutionConfig::Get() in Shared code
- Stop (bare)    — hb::shared::action::Type::Stop (Shared enum value)
- Info (bare)    — local variable `info` shadowing in DialogBoxManager.cpp
- Register       — `register` is a C++ reserved keyword
- BeginFrame     — IRenderer::BeginFrame called from client code
- EndFrame       — IRenderer::EndFrame called from client code
- EquipItem      — CommonType::EquipItem (Shared enum value)
- AddEffect      — ItemEffectType::AddEffect (Shared enum value)
- IsPlayer/IsNPC/IsMale/IsFemale — delegate to hb::shared::owner:: functions

CursorTarget and FrameTiming are DEFERRED to separate Mode 1 manual edits because
header declarations are bare (inside namespace/class blocks) and qualified patterns
can't match them.

Usage:
    python Scripts/client_methods_snake_case.py --dry-run    # Preview
    python Scripts/client_methods_snake_case.py              # Apply
"""

import os
import re
import sys
from pathlib import Path

# ============================================================================
# Master rename dictionary: old_name → new_name
# ============================================================================

RENAMES = {
    # ========================================================================
    # Generic single-word methods (shared across multiple client classes)
    # These were excluded from client_snake_case.py but are safe now that
    # we've verified no SFMLEngine collision (no ->Method( calls from client)
    # ========================================================================
    "Reset": "reset",
    "Update": "update",
    "Initialize": "initialize",
    "Shutdown": "shutdown",
    "IsActive": "is_active",
    "Format": "format",
    # "Get" — SKIP: ResolutionConfig::Get() in Shared code
    "Render": "render",
    # "Info" — SKIP: local variable `info` shadowing in DialogBoxManager.cpp
    "IsEnabled": "is_enabled",
    "Enable": "enable",
    "Disable": "disable",
    "Toggle": "toggle",
    # "Register" — SKIP: `register` is a C++ reserved keyword
    "IsValid": "is_valid",
    "ProcessMessage": "process_message",
    "GetName": "get_name",
    "Execute": "execute",
    # "Stop" — SKIP: hb::shared::action::Type::Stop (Shared enum)
    "Bump": "bump",
    "GetConfig": "get_config",
    "GetList": "get_list",
    "GetId": "get_id",

    # ========================================================================
    # CGame — Core Methods
    # ========================================================================
    "CheckActiveAura2": "check_active_aura2",
    "CheckActiveAura": "check_active_aura",
    "ReadSettings": "read_settings",
    "WriteSettings": "write_settings",
    "FindGuildName": "find_guild_name",
    "bItemDrop_ExternalScreen": "item_drop_external_screen",
    "CreateScreenShot": "create_screen_shot",
    "CrusadeWarResult": "crusade_war_result",
    "CrusadeContributionResult": "crusade_contribution_result",
    # "CannotConstruct" — SKIP: hb::shared::net::Notify::CannotConstruct enum value
    "DrawTopMsg": "draw_top_msg",
    "SetTopMsg": "set_top_msg",
    "DrawObjectFOE": "draw_object_foe",
    # "GrandMagicResult" — SKIP: hb::shared::net::Notify::GrandMagicResult enum value
    # "MeteorStrikeComing" — SKIP: hb::shared::net::Notify::MeteorStrikeComing enum value
    "DrawNewDialogBox": "draw_new_dialog_box",
    "AddMapStatusInfo": "add_map_status_info",
    "_RequestMapStatus": "request_map_status",
    "DrawDialogBoxs": "draw_dialog_boxs",
    "FormatCommaNumber": "format_comma_number",
    "ResponsePanningHandler": "response_panning_handler",
    "_SetIlusionEffect": "set_ilusion_effect",
    "NoticementHandler": "noticement_handler",
    "GetItemConfig": "get_item_config",
    "FindItemIdByName": "find_item_id_by_name",
    "_LoadGameMsgTextContents": "load_game_msg_text_contents",
    "GetNpcConfigNameById": "get_npc_config_name_by_id",
    "GetNpcConfigName": "get_npc_config_name",
    "ResolveNpcType": "resolve_npc_type",
    "UseShortCut": "use_shortcut",
    "DrawCursor": "draw_cursor",
    "NpcTalkHandler": "npc_talk_handler",
    "SetCameraShakingEffect": "set_camera_shaking_effect",
    "ClearSkillUsingStatus": "clear_skill_using_status",
    "bCheckExID": "check_ex_id",
    "bCheckLocalChatCommand": "check_local_chat_command",
    "GetOfficialMapName": "get_official_map_name",
    "iGetLevelExp": "get_level_exp",
    "DrawVersion": "draw_version",
    "_bIsItemOnHand": "is_item_on_hand",
    "DynamicObjectHandler": "dynamic_object_handler",
    "_bCheckItemByType": "check_item_by_type",
    "DrawNpcName": "draw_npc_name",
    "DrawObjectName": "draw_object_name",
    "_LoadTextDlgContents": "load_text_dlg_contents",
    "_iLoadTextDlgContents2": "load_text_dlg_contents2",
    "RequestFullObjectData": "request_full_object_data",
    "RetrieveItemHandler": "retrieve_item_handler",
    "CivilRightAdmissionHandler": "civil_right_admission_handler",
    "_Draw_CharacterBody": "draw_character_body",
    "RequestTeleportAndWaitData": "request_teleport_and_wait_data",
    "PointCommandHandler": "point_command_handler",
    "AddEventList": "add_event_list",
    "_ShiftGuildOperationList": "shift_guild_operation_list",
    "_PutGuildOperationList": "put_guild_operation_list",
    "DisbandGuildResponseHandler": "disband_guild_response_handler",
    "InitPlayerCharacteristics": "init_player_characteristics",
    "CreateNewGuildResponseHandler": "create_new_guild_response_handler",
    "InitGameSettings": "init_game_settings",
    "CommonEventHandler": "common_event_handler",
    "iGetTopDialogBoxIndex": "get_top_dialog_box_index",
    "DisableDialogBox": "disable_dialog_box",
    "EnableDialogBox": "enable_dialog_box",
    "InitItemList": "init_item_list",

    # CGame — DrawObject_On* methods (longest first to avoid partial matches)
    "DrawObject_OnAttackMove": "draw_object_on_attack_move",
    "DrawObject_OnDamageMove": "draw_object_on_damage_move",
    "DrawObject_OnMove_ForMenu": "draw_object_on_move_for_menu",
    "DrawObject_OnGetItem": "draw_object_on_get_item",
    "DrawObject_OnAttack": "draw_object_on_attack",
    "DrawObject_OnDamage": "draw_object_on_damage",
    "DrawObject_OnDying": "draw_object_on_dying",
    "DrawObject_OnMagic": "draw_object_on_magic",
    "DrawObject_OnDead": "draw_object_on_dead",
    "DrawObject_OnStop": "draw_object_on_stop",
    "DrawObject_OnMove": "draw_object_on_move",
    "DrawObject_OnRun": "draw_object_on_run",

    "ClearGuildNameList": "clear_guild_name_list",
    "DrawBackground": "draw_background",
    "ChatMsgHandler": "chat_msg_handler",
    "ReleaseUnusedSprites": "release_unused_sprites",
    "OnKeyUp": "on_key_up",
    "ChangeGameMode": "change_game_mode",
    "LogRecvMsgHandler": "log_recv_msg_handler",
    "LogResponseHandler": "log_response_handler",
    "OnLogSocketEvent": "on_log_socket_event",
    "OnTimer": "on_timer",
    "LogEventHandler": "log_event_handler",
    "_ReadMapData": "read_map_data",
    "MotionEventHandler": "motion_event_handler",
    "InitDataResponseHandler": "init_data_response_handler",
    "InitPlayerResponseHandler": "init_player_response_handler",
    "ConnectionEstablishHandler": "connection_establish_handler",
    "MotionResponseHandler": "motion_response_handler",
    "GameRecvMsgHandler": "game_recv_msg_handler",
    "DrawObjects": "draw_objects",
    "bSendCommand": "send_command",
    "RestoreSprites": "restore_sprites",
    "CommandProcessor": "command_processor",
    "ProcessLeftClick": "process_left_click",
    "ProcessRightClick": "process_right_click",
    "ProcessMotionCommands": "process_motion_commands",
    "OnGameSocketEvent": "on_game_socket_event",
    "OnKeyDown": "on_key_down",
    "RegisterHotkeys": "register_hotkeys",

    # CGame — Hotkey methods
    "Hotkey_ToggleDialogTransparency": "hotkey_toggle_dialog_transparency",
    "Hotkey_Simple_ToggleCharacterInfo": "hotkey_simple_toggle_character_info",
    "Hotkey_Simple_UseMagicShortcut": "hotkey_simple_use_magic_shortcut",
    "Hotkey_Simple_ToggleChatHistory": "hotkey_simple_toggle_chat_history",
    "Hotkey_Simple_ToggleSystemMenu": "hotkey_simple_toggle_system_menu",
    "Hotkey_Simple_WhisperCycleDown": "hotkey_simple_whisper_cycle_down",
    "Hotkey_Simple_ToggleSafeAttack": "hotkey_simple_toggle_safe_attack",
    "Hotkey_Simple_TabToggleCombat": "hotkey_simple_tab_toggle_combat",
    "Hotkey_Simple_LoadBackupChat": "hotkey_simple_load_backup_chat",
    "Hotkey_Simple_UseHealthPotion": "hotkey_simple_use_health_potion",
    "Hotkey_Simple_ToggleInventory": "hotkey_simple_toggle_inventory",
    "Hotkey_Simple_SpecialAbility": "hotkey_simple_special_ability",
    "Hotkey_Simple_WhisperCycleUp": "hotkey_simple_whisper_cycle_up",
    "Hotkey_Simple_UseManaPotion": "hotkey_simple_use_mana_potion",
    "Hotkey_Simple_UseShortcut1": "hotkey_simple_use_shortcut1",
    "Hotkey_Simple_UseShortcut2": "hotkey_simple_use_shortcut2",
    "Hotkey_Simple_UseShortcut3": "hotkey_simple_use_shortcut3",
    "Hotkey_Simple_ToggleMagic": "hotkey_simple_toggle_magic",
    "Hotkey_Simple_ToggleSkill": "hotkey_simple_toggle_skill",
    "Hotkey_ToggleSoundAndMusic": "hotkey_toggle_sound_and_music",
    "Hotkey_Simple_Screenshot": "hotkey_simple_screenshot",
    "Hotkey_Simple_ArrowRight": "hotkey_simple_arrow_right",
    "Hotkey_Simple_ArrowLeft": "hotkey_simple_arrow_left",
    "Hotkey_ToggleRunningMode": "hotkey_toggle_running_mode",
    "Hotkey_ToggleForceAttack": "hotkey_toggle_force_attack",
    "Hotkey_CycleDetailLevel": "hotkey_cycle_detail_level",
    "Hotkey_ToggleSystemMenu": "hotkey_toggle_system_menu",
    "Hotkey_Simple_Escape": "hotkey_simple_escape",
    "Hotkey_Simple_ZoomOut": "hotkey_simple_zoom_out",
    "Hotkey_Simple_ZoomIn": "hotkey_simple_zoom_in",
    "Hotkey_ToggleGuideMap": "hotkey_toggle_guide_map",
    "Hotkey_WhisperTarget": "hotkey_whisper_target",
    "Hotkey_ToggleHelp": "hotkey_toggle_help",

    # CGame — Config loading
    "_bDecodeItemConfigFileContents": "decode_item_config_file_contents",
    "_bDecodeMagicConfigFileContents": "decode_magic_config_file_contents",
    "_bDecodeSkillConfigFileContents": "decode_skill_config_file_contents",
    "_bDecodeNpcConfigFileContents": "decode_npc_config_file_contents",
    "_CheckConfigsReadyAndEnterGame": "check_configs_ready_and_enter_game",
    "_RequestConfigsFromServer": "request_configs_from_server",
    "_TryReplayCacheForConfig": "try_replay_cache_for_config",
    "EnsureItemConfigsLoaded": "ensure_item_configs_loaded",
    "EnsureMagicConfigsLoaded": "ensure_magic_configs_loaded",
    "EnsureSkillConfigsLoaded": "ensure_skill_configs_loaded",
    "EnsureNpcConfigsLoaded": "ensure_npc_configs_loaded",
    "_EnsureConfigLoaded": "ensure_config_loaded",

    # CGame — Remaining methods
    "ReserveFightzoneResponseHandler": "reserve_fightzone_response_handler",
    "ShowHeldenianVictory": "show_heldenian_victory",
    "StartBGM": "start_bgm",
    "bHasHeroSet": "has_hero_set",
    "DKGlare": "dk_glare",
    "Abaddon_corpse": "abaddon_corpse",
    "DrawAngel": "draw_angel",
    "_ItemDropHistory": "item_drop_history",

    # ========================================================================
    # IDialogBox — Virtual Methods and Helpers
    # ========================================================================
    "OnDoubleClick": "on_double_click",
    "OnItemDrop": "on_item_drop",
    "OnDisable": "on_disable",
    "OnEnable": "on_enable",
    "OnUpdate": "on_update",
    "OnPress": "on_press",
    "OnClick": "on_click",
    "OnDraw": "on_draw",
    "PutAlignedString": "put_aligned_string",
    "PlaySoundEffect": "play_sound_effect",
    "SetDefaultRect": "set_default_rect",
    "DisableThisDialog": "disable_this_dialog",
    "SetCanCloseOnRightClick": "set_can_close_on_right_click",
    "GetDialogBoxAs": "get_dialog_box_as",
    "GetDialogBox": "get_dialog_box",
    "PutString": "put_string",
    "InfoOf": "info_of",

    # ========================================================================
    # DialogBoxManager — Methods
    # ========================================================================
    "InitializeDialogBoxes": "initialize_dialog_boxes",
    "HandleDraggingItemRelease": "handle_dragging_item_release",
    "RegisterDialogBox": "register_dialog_box",
    "UpdateDialogBoxs": "update_dialog_boxs",
    "ToggleDialogBox": "toggle_dialog_box",
    "HandleDoubleClick": "handle_double_click",
    "HandleMouseDown": "handle_mouse_down",
    "HandleRightClick": "handle_right_click",
    "HandleItemDrop": "handle_item_drop",
    "HandleClick": "handle_click",
    "HandlePress": "handle_press",
    "InitDefaults": "init_defaults",
    "SetEnabled": "set_enabled",
    "SetOrderAt": "set_order_at",
    "GetTopId": "get_top_id",
    "OrderAt": "order_at",

    # ========================================================================
    # IGameScreen — Helper Methods
    # ========================================================================
    "GetCenteredDialogPos": "get_centered_dialog_pos",
    "PutString_SprFont": "put_string_spr_font",
    "StartInputString": "start_input_string",
    "ClearInputString": "clear_input_string",
    "ShowReceivedString": "show_received_string",
    "EndInputString": "end_input_string",

    # ========================================================================
    # GameModeManager — Static API and Impl Methods
    # ========================================================================
    "GetActiveOverlayAs": "get_active_overlay_as",
    "GetActiveScreenAs": "get_active_screen_as",
    "GetTransitionState": "get_transition_state",
    "GetActiveOverlay": "get_active_overlay",
    "GetActiveScreen": "get_active_screen",
    "GetModeStartTime": "get_mode_start_time",
    "IsTransitioning": "is_transitioning",
    "UpdateScreensImpl": "update_screens_impl",
    "GetFadeAlphaImpl": "get_fade_alpha_impl",
    "ApplyScreenChange": "apply_screen_change",
    "SetCurrentMode": "set_current_mode",
    "UpdateScreens": "update_screens",
    "InitializeImpl": "initialize_impl",
    "GetModeValue": "get_mode_value",
    "GetFadeAlpha": "get_fade_alpha",
    "ShutdownImpl": "shutdown_impl",
    "UpdateImpl": "update_impl",
    "RenderImpl": "render_impl",
    "GetGame": "get_game",
    "GetMode": "get_mode",

    # ========================================================================
    # ChatCommand / ChatCommandManager
    # ========================================================================
    "RegisterBuiltInCommands": "register_built_in_commands",
    "RegisterCommand": "register_command",
    "ProcessCommand": "process_command",

    # ========================================================================
    # CMapData — Methods (Init SKIPPED — IRenderer::Init collision)
    # ========================================================================
    "GetOwnerStatusByObjectID": "get_owner_status_by_object_id",
    "iObjectFrameCounter": "object_frame_counter",
    "OpenMapDataFile": "open_map_data_file",
    "bSetChatMsgOwner": "set_chat_msg_owner",
    "bSetDynamicObject": "set_dynamic_object",
    "_bDecodeMapInfo": "decode_map_info",
    "bGetIsLocateable": "get_is_locatable",
    "ClearDeadChatMsg": "clear_dead_chat_msg",
    "bIsTeleportLoc": "is_teleport_loc",
    "bSetDeadOwner": "set_dead_owner",
    "ShiftMapData": "shift_map_data",
    "ClearChatMsg": "clear_chat_msg",
    "bSetOwner": "set_owner",
    "bGetOwner": "get_owner",
    "bSetItem": "set_item",

    # ========================================================================
    # CCamera — Methods
    # ========================================================================
    "WorldToScreenX": "world_to_screen_x",
    "WorldToScreenY": "world_to_screen_y",
    "ScreenToWorldX": "screen_to_world_x",
    "ScreenToWorldY": "screen_to_world_y",
    "RestorePosition": "restore_position",
    "GetDestinationX": "get_destination_x",
    "GetDestinationY": "get_destination_y",
    "MoveDestination": "move_destination",
    "SetDestination": "set_destination",
    "GetShakeDegree": "get_shake_degree",
    "SavePosition": "save_position",
    "CenterOnTile": "center_on_tile",
    "SetPosition": "set_position",
    "ApplyShake": "apply_shake",
    "SetShake": "set_shake",
    "SnapTo": "snap_to",
    "GetX": "get_x",
    "GetY": "get_y",

    # CursorTarget & FrameTiming — DEFERRED to Mode 1 manual edits.
    # Qualified patterns don't work because header declarations are bare (inside
    # namespace/class block). Internal calls within the .cpp are also bare.
    # Will handle separately after this script completes.

    # ========================================================================
    # RenderHelpers — Namespace Functions
    # ========================================================================
    "DrawAbaddonEffects": "draw_abaddon_effects",
    "ApplyDirectionOverride": "apply_direction_override",
    "CheckInvisibility": "check_invisibility",
    "DrawPlayerLayers": "draw_player_layers",
    "DrawBerserkGlow": "draw_berserk_glow",
    "DrawEffectAuras": "draw_effect_auras",
    "DrawEquipLayer": "draw_equip_layer",
    "DrawAfkEffect": "draw_afk_effect",
    "DrawNpcLayers": "draw_npc_layers",
    "DrawGMEffect": "draw_gm_effect",
    "DrawNpcLight": "draw_npc_light",
    "UpdateChat": "update_chat",
    "DrawWeapon": "draw_weapon",
    "DrawShield": "draw_shield",
    "DrawShadow": "draw_shadow",
    "DrawBody": "draw_body",
    "DrawName": "draw_name",

    # ========================================================================
    # CPlayerRenderer / CNpcRenderer — Shared Method Names
    # ========================================================================
    "DrawAttackMove": "draw_attack_move",
    "DrawDamageMove": "draw_damage_move",
    "DrawGetItem": "draw_get_item",
    "DrawAttack": "draw_attack",
    "DrawDamage": "draw_damage",
    "DrawDying": "draw_dying",
    "DrawMagic": "draw_magic",
    "DrawStop": "draw_stop",
    "DrawMove": "draw_move",
    "DrawDead": "draw_dead",
    "DrawRun": "draw_run",

    # ========================================================================
    # HotkeyManager — Methods
    # ========================================================================
    "HandleKeyDown": "handle_key_down",
    "HandleKeyUp": "handle_key_up",
    "HandleKey": "handle_key",

    # ========================================================================
    # CPlayerController — Methods
    # ========================================================================
    "CalculatePlayerTurn": "calculate_player_turn",
    "IncrementCommandCount": "increment_command_count",
    "DecrementCommandCount": "decrement_command_count",
    "IsCommandAvailable": "is_command_available",
    "SetCommandAvailable": "set_command_available",
    "ClearPendingStopDir": "clear_pending_stop_dir",
    "IsPrevMoveBlocked": "is_prev_move_blocked",
    "SetPrevMoveBlocked": "set_prev_move_blocked",
    "ResetCommandCount": "reset_command_count",
    "GetPendingStopDir": "get_pending_stop_dir",
    "SetPendingStopDir": "set_pending_stop_dir",
    "ClearDestination": "clear_destination",
    "GetAttackEndTime": "get_attack_end_time",
    "SetAttackEndTime": "set_attack_end_time",
    "GetCommandCount": "get_command_count",
    "SetCommandCount": "set_command_count",
    "GetNextMoveDir": "get_next_move_dir",
    "GetCommandTime": "get_command_time",
    "SetCommandTime": "set_command_time",
    "GetPlayerTurn": "get_player_turn",
    "SetPlayerTurn": "set_player_turn",
    "GetPrevMoveX": "get_prev_move_x",
    "GetPrevMoveY": "get_prev_move_y",
    "SetPrevMove": "set_prev_move",
    "GetCommand": "get_command",
    "SetCommand": "set_command",

    # ========================================================================
    # EntityMotion — Methods
    # ========================================================================
    "GetDirectionStartOffset": "get_direction_start_offset",
    "GetDurationForAction": "get_duration_for_action",
    "StartMoveWithOffset": "start_move_with_offset",
    "StartMove": "start_move",
    "QueueMove": "queue_move",
    "IsComplete": "is_complete",
    "IsMoving": "is_moving",
    "HasPending": "has_pending",

    # ========================================================================
    # CEntityRenderState — Methods (skip IsPlayer/IsNPC — Shared delegation)
    # ========================================================================
    # GetName already included in generics above

    # ========================================================================
    # FocusedObject — Struct Method
    # ========================================================================
    "IsDead": "is_dead",

    # ========================================================================
    # PlayerAnim — Namespace Function
    # ========================================================================
    "FromAction": "from_action",
}

# ============================================================================
# File processing (same logic as client_snake_case.py)
# ============================================================================

CLIENT_DIR = Path("Z:/Helbreath-3.82/Sources/Client")


def get_source_files():
    """Get all .h and .cpp files in client directory (skip .bak files)."""
    files = []
    for ext in ("*.h", "*.cpp"):
        files.extend(CLIENT_DIR.glob(ext))
    return sorted(f for f in files if '.bak_' not in f.name)


def process_file(filepath, renames_sorted, dry_run=False):
    """Process a single file, applying renames line by line.
    Returns (changed_count, details_list)."""
    try:
        with open(filepath, 'r', encoding='utf-8', errors='replace') as f:
            original_lines = f.readlines()
    except Exception as e:
        return 0, [f"ERROR reading {filepath}: {e}"]

    new_lines = []
    file_changes = 0
    details = []

    for line_num, line in enumerate(original_lines, 1):
        # Skip #include lines to avoid corrupting include paths
        # Strip BOM (\ufeff) in addition to whitespace
        stripped = line.lstrip().lstrip('\ufeff')
        if stripped.startswith('#include'):
            new_lines.append(line)
            continue

        new_line = line
        for old, new, pattern in renames_sorted:
            if old not in new_line:
                continue  # Fast skip — substring check before regex
            result = pattern.sub(new, new_line)
            if result != new_line:
                count = len(pattern.findall(new_line))
                file_changes += count
                details.append(f"  L{line_num}: {old} -> {new} ({count}x)")
                new_line = result

        new_lines.append(new_line)

    if file_changes > 0 and not dry_run:
        with open(filepath, 'w', encoding='utf-8', newline='') as f:
            f.writelines(new_lines)

    return file_changes, details


def main():
    dry_run = "--dry-run" in sys.argv

    # Sort renames: longest first to avoid partial matches
    renames_sorted = []
    for old, new in sorted(RENAMES.items(), key=lambda x: -len(x[0])):
        pattern = re.compile(r'\b' + re.escape(old) + r'\b')
        renames_sorted.append((old, new, pattern))

    print(f"{'DRY RUN — ' if dry_run else ''}Client methods snake_case conversion")
    print(f"Total renames in dictionary: {len(RENAMES)}")
    print()

    source_files = get_source_files()
    print(f"Source files to process: {len(source_files)}")
    print()

    total_changes = 0
    files_changed = 0

    for filepath in source_files:
        changes, details = process_file(filepath, renames_sorted, dry_run)
        if changes > 0:
            rel = filepath.relative_to(Path("Z:/Helbreath-3.82"))
            print(f"  {rel}: {changes} replacements")
            if dry_run:
                for d in details[:20]:
                    print(d)
                if len(details) > 20:
                    print(f"  ... and {len(details) - 20} more")
            total_changes += changes
            files_changed += 1

    print()
    print(f"{'Would change' if dry_run else 'Changed'}: {total_changes} replacements across {files_changed} files")

    if dry_run:
        print("\nRun without --dry-run to apply changes.")


if __name__ == "__main__":
    main()

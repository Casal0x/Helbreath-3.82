#!/usr/bin/env python3
"""
Client coding standard conversion: PascalCase/Hungarian → snake_case.

Mode 2 script — justified: ~400 renames across ~220 client source files.

Scope: Extracted manager classes from Track C (WeatherManager, BuildItemManager,
ShopManager, InventoryManager, CraftingManager, FishingManager, ItemNameFormatter,
GuildManager, QuestManager, TeleportManager, EventListManager, TextInputManager,
CombatSystem, MagicCastingSystem, SpellAoE, AudioManager, ConfigManager,
FloatingTextManager, EffectManager, AnimationState, Effect, FloatingText).

Approach:
- Process each rename one at a time using word-boundary regex
- Sort by length (longest first) to avoid partial matches
- Skip #include lines to avoid path corruption
- Report per-file statistics

Deliberately excluded (unsafe for global regex):
- Virtual base class methods: Clear, Reset, Update, Get, Initialize, Shutdown, Draw,
  Format, Load, Save, IsActive (defined in SFMLEngine/Shared base classes)
- Parameters: pGame, pData, pItem, pCfg (too broad, collide with miniaudio/Shared)
- Local variables (collide across files)
- Non-m_ struct fields: sX, sY, cDir, iIndex, dwTime, cTxt, cColor (too common)
- CGame class name (too disruptive, referenced from Shared code)
- CItem/ASIOSocket/CSkill definitions are in Shared/ but their member ACCESSES in Client
  code are included here because server_snake_case.py already changed the definitions

Usage:
    python Scripts/client_snake_case.py --dry-run    # Preview changes
    python Scripts/client_snake_case.py              # Apply changes
"""

import os
import re
import sys
from pathlib import Path

# ============================================================================
# Master rename dictionary: old_name → new_name
# Combined from 8 analysis agents, dangerous patterns removed
# ============================================================================

RENAMES = {
    # ========================================================================
    # Qualified Enum Values — REMOVED: SoundType::Character/Monster/Effect,
    # DamageTextType::Small/Medium/Large, NotifyTextType::LevelUp
    # Reason: enum class values in declarations can't be safely renamed
    # because `small` is a Windows RPC macro (#define small char).
    # Enum values stay PascalCase for now — to be fixed in a targeted pass.
    # ========================================================================

    # ========================================================================
    # Bare Enum Values (unique enough for safe global rename)
    # ========================================================================
    "PlayerChat": "player_chat",
    "SkillChange": "skill_change",
    "MagicCastName": "magic_cast_name",
    "EnemyKill": "enemy_kill",

    # ========================================================================
    # Class/Struct/Enum Type Names (PascalCase → snake_case)
    # Filenames are preserved because #include lines are skipped.
    # ========================================================================
    # Managers
    "CFloatingTextManager": "floating_text_manager",
    "MagicCastingSystem": "magic_casting_system",
    "BuildItemManager": "build_item_manager",
    "EventListManager": "event_list_manager",
    "TextInputManager": "text_input_manager",
    "ItemNameFormatter": "item_name_formatter",
    "InventoryManager": "inventory_manager",
    "TeleportManager": "teleport_manager",
    "CraftingManager": "crafting_manager",
    "WeatherManager": "weather_manager",
    "FishingManager": "fishing_manager",
    "EffectManager": "effect_manager",
    "ConfigManager": "config_manager",
    "CombatSystem": "combat_system",
    "GuildManager": "guild_manager",
    "QuestManager": "quest_manager",
    "AudioManager": "audio_manager",
    "ShopManager": "shop_manager",

    # Structs and other types
    "AnimationState": "animation_state",
    "CFloatingText": "floating_text",
    "WeatherParticle": "weather_particle",
    "SpellAoEParams": "spell_aoe_params",
    "SpellAoETile": "spell_aoe_tile",
    "DecodedSound": "decoded_sound",
    "ActiveSound": "active_sound",
    "CBuildItem": "build_item",

    # Enum types
    "NotifyTextType": "notify_text_type",
    "DamageTextType": "damage_text_type",
    "ChatTextType": "chat_text_type",
    "SoundType": "sound_type",

    # ========================================================================
    # Shared Method: SetGame (used by all extracted managers, all defs in Client)
    # ========================================================================
    "SetGame": "set_game",

    # ========================================================================
    # Shared Member: m_pGame (all extracted managers + dialogs + screens)
    # ========================================================================
    "m_pGame": "m_game",

    # ========================================================================
    # CBuildItem (Client) — Member Variables
    # ========================================================================
    "m_cElementName1": "m_element_name_1",
    "m_cElementName2": "m_element_name_2",
    "m_cElementName3": "m_element_name_3",
    "m_cElementName4": "m_element_name_4",
    "m_cElementName5": "m_element_name_5",
    "m_cElementName6": "m_element_name_6",
    "m_bBuildEnabled": "m_build_enabled",
    "m_iElementCount": "m_element_count",
    "m_bElementFlag": "m_element_flag",
    "m_iSkillLimit": "m_skill_limit",
    "m_iSprFrame": "m_sprite_frame",
    "m_iMaxSkill": "m_max_skill",
    "m_iSprH": "m_sprite_handle",

    # ========================================================================
    # CGame — Struct Member Renames (m_ prefixed)
    # ========================================================================
    "m_stGuildName": "m_guild_name_cache",
    "m_stQuest": "m_quest",

    # ========================================================================
    # AudioManager — Member Variables
    # ========================================================================
    "m_bAmbientGroupInitialized": "m_ambient_group_initialized",
    "m_bSfxGroupInitialized": "m_sfx_group_initialized",
    "m_bUIGroupInitialized": "m_ui_group_initialized",
    "m_currentMusicTrack": "m_current_music_track",
    "m_characterSounds": "m_character_sounds",
    "m_bSoundAvailable": "m_sound_available",
    "m_bMasterEnabled": "m_master_enabled",
    "m_bAmbientEnabled": "m_ambient_enabled",
    "m_monsterSounds": "m_monster_sounds",
    "m_bSoundEnabled": "m_sound_enabled",
    "m_bMusicEnabled": "m_music_enabled",
    "m_effectSounds": "m_effect_sounds",
    "m_ambientVolume": "m_ambient_volume",
    "m_activeSounds": "m_active_sounds",
    "m_dwSoundOrder": "m_sound_order",
    "m_ambientGroup": "m_ambient_group",
    "m_masterVolume": "m_master_volume",
    "m_bBgmLoaded": "m_bgm_loaded",
    "m_soundVolume": "m_sound_volume",
    "m_musicVolume": "m_music_volume",
    "m_bUIEnabled": "m_ui_enabled",
    "m_listenerX": "m_listener_x",
    "m_listenerY": "m_listener_y",
    "m_sfxGroup": "m_sfx_group",
    "m_bgmSound": "m_bgm_sound",
    "m_uiVolume": "m_ui_volume",
    "m_uiGroup": "m_ui_group",

    # ========================================================================
    # ConfigManager — Member Variables
    # ========================================================================
    "m_bFullscreenStretch": "m_fullscreen_stretch",
    "m_recentShortcut": "m_recent_shortcut",
    "m_bCaptureMouse": "m_capture_mouse",
    "m_magicShortcut": "m_magic_shortcut",
    "m_bPatchingGrid": "m_patching_grid",
    "m_bShowLatency": "m_show_latency",
    "m_bRunningMode": "m_running_mode",
    "m_windowHeight": "m_window_height",
    "m_bDialogTrans": "m_dialog_trans",
    "m_cDetailLevel": "m_detail_level",
    "m_bFullscreen": "m_fullscreen",
    "m_bBorderless": "m_borderless",
    "m_windowWidth": "m_window_width",
    "m_bTileGrid": "m_tile_grid",
    "m_bShowFPS": "m_show_fps",
    "m_iFpsLimit": "m_fps_limit",
    "m_bZoomMap": "m_zoom_map",
    "m_bDirty": "m_dirty",
    "m_bVSync": "m_vsync",

    # ========================================================================
    # Shared across AudioManager/ConfigManager/ChatCommandManager
    # ========================================================================
    "m_bInitialized": "m_initialized",

    # ========================================================================
    # CFloatingText — Member Variables
    # ========================================================================
    "m_eCategory": "m_category",
    "m_eChatType": "m_chat_type",
    "m_eDamageType": "m_damage_type",
    "m_eNotifyType": "m_notify_type",
    "m_iObjectID": "m_object_id",
    "m_szText": "m_text",

    # ========================================================================
    # Shared across CFloatingText, CEffect, CMsg, Game, Screen_OnGame
    # ========================================================================
    "m_sX": "m_x",
    "m_sY": "m_y",
    "m_dwTime": "m_time",

    # ========================================================================
    # EffectManager — Member Variables
    # ========================================================================
    "m_pEffectList": "m_effect_list",
    "m_pEffectSpr": "m_effect_sprites",

    # ========================================================================
    # CEffect — Member Variables
    # ========================================================================
    "m_dwFrameTime": "m_frame_time",
    "m_cMaxFrame": "m_max_frame",
    "m_cFrame": "m_frame",
    "m_sType": "m_type",
    "m_cDir": "m_dir",
    "m_iErr": "m_error",
    "m_dX": "m_dest_x",
    "m_dY": "m_dest_y",
    "m_mX2": "m_move_x2",
    "m_mY2": "m_move_y2",
    "m_mX3": "m_move_x3",
    "m_mY3": "m_move_y3",
    "m_mX": "m_move_x",
    "m_mY": "m_move_y",
    "m_rX": "m_render_x",
    "m_rY": "m_render_y",
    "m_iV1": "m_value1",

    # ========================================================================
    # AnimParams struct fields (plain data, unique Hungarian names)
    # ========================================================================
    "dwLifetimeMs": "lifetime_ms",
    "dwShowDelayMs": "show_delay_ms",
    "iRiseDurationMs": "rise_duration_ms",
    "iStartOffsetY": "start_offset_y",
    "bUseSpriteFont": "use_sprite_font",
    "iRisePixels": "rise_pixels",
    "iFontOffset": "font_offset",

    # ========================================================================
    # WeatherManager — Methods
    # ========================================================================
    "DrawThunderEffect": "draw_thunder_effect",
    "SetWeatherStatus": "set_weather_status",
    "GetWeatherStatus": "get_weather_status",
    "SetAmbientLight": "set_ambient_light",
    "GetAmbientLight": "get_ambient_light",
    "ResetParticles": "reset_particles",
    "SetDependencies": "set_dependencies",
    "GetEffectType": "get_effect_type",
    "SetWeather": "set_weather",
    "SetMapData": "set_map_data",
    "IsRaining": "is_raining",
    "IsSnowing": "is_snowing",
    "SetXmas": "set_xmas",
    "IsNight": "is_night",

    # ========================================================================
    # BuildItemManager — Methods
    # ========================================================================
    "UpdateAvailableRecipes": "update_available_recipes",
    "ValidateCurrentRecipe": "validate_current_recipe",
    "ParseRecipeFile": "parse_recipe_file",
    "GetDisplayList": "get_display_list",
    "LoadRecipes": "load_recipes",

    # ========================================================================
    # ShopManager — Methods
    # ========================================================================
    "GetPendingShopType": "get_pending_shop_type",
    "SetPendingShopType": "set_pending_shop_type",
    "RequestShopMenu": "request_shop_menu",
    "HandleResponse": "handle_response",
    "SendRequest": "send_request",
    "GetItemList": "get_item_list",
    "ClearItems": "clear_items",
    "HasItems": "has_items",

    # ========================================================================
    # InventoryManager — Methods
    # ========================================================================
    "CheckItemOperationEnabled": "check_item_operation_enabled",
    "GetTotalItemCount": "get_total_item_count",
    "GetBankItemCount": "get_bank_item_count",
    "CalcTotalWeight": "calc_total_weight",
    "SetItemOrder": "set_item_order",
    "UnequipSlot": "unequip_slot",
    # "EquipItem" REMOVED — collides with CommonType::EquipItem (Shared enum)
    "EraseItem": "erase_item",

    # ========================================================================
    # CraftingManager — Methods
    # ========================================================================
    "HandleNoMatchingPortion": "handle_no_matching_portion",
    "HandleLowPortionSkill": "handle_low_portion_skill",
    "HandleBuildItemSuccess": "handle_build_item_success",
    "HandleCraftingSuccess": "handle_crafting_success",
    "HandleBuildItemFail": "handle_build_item_fail",
    "HandlePortionSuccess": "handle_portion_success",
    "HandleCraftingFail": "handle_crafting_fail",
    "HandlePortionFail": "handle_portion_fail",

    # ========================================================================
    # FishingManager — Methods
    # ========================================================================
    "HandleEventFishMode": "handle_event_fish_mode",
    "HandleFishCanceled": "handle_fish_canceled",
    "HandleFishSuccess": "handle_fish_success",
    "HandleFishChance": "handle_fish_chance",
    "HandleFishFail": "handle_fish_fail",

    # ========================================================================
    # ItemNameFormatter — Methods
    # ========================================================================
    "SetItemConfigs": "set_item_configs",

    # ========================================================================
    # GuildManager — Methods
    # ========================================================================
    "HandleQueryDismissGuildPermission": "handle_query_dismiss_guild_permission",
    "HandleQueryJoinGuildPermission": "handle_query_join_guild_permission",
    "HandleCannotJoinMoreGuildsMan": "handle_cannot_join_more_guilds_man",
    "HandleCreateNewGuildResponse": "handle_create_new_guild_response",
    "HandleDisbandGuildResponse": "handle_disband_guild_response",
    "HandleDismissGuildApprove": "handle_dismiss_guild_approve",
    "HandleNoGuildMasterLevel": "handle_no_guild_master_level",
    "HandleDismissGuildReject": "handle_dismiss_guild_reject",
    "HandleSuccessBanGuildMan": "handle_success_ban_guild_man",
    "HandleReqGuildNameAnswer": "handle_req_guild_name_answer",
    "HandleCannotBanGuildMan": "handle_cannot_ban_guild_man",
    "HandleJoinGuildApprove": "handle_join_guild_approve",
    "HandleJoinGuildReject": "handle_join_guild_reject",
    "HandleDismissGuildsMan": "handle_dismiss_guilds_man",
    "HandleGuildDisbanded": "handle_guild_disbanded",
    "HandleNewGuildsMan": "handle_new_guilds_man",
    "UpdateLocationFlags": "update_location_flags",

    # ========================================================================
    # QuestManager — Methods
    # ========================================================================
    "HandleQuestCompleted": "handle_quest_completed",
    "HandleQuestContents": "handle_quest_contents",
    "HandleQuestAborted": "handle_quest_aborted",
    "HandleQuestCounter": "handle_quest_counter",
    "HandleQuestReward": "handle_quest_reward",

    # ========================================================================
    # TeleportManager — Methods
    # ========================================================================
    "HandleHeldenianTeleportList": "handle_heldenian_teleport_list",
    "HandleChargedTeleport": "handle_charged_teleport",
    "HandleTeleportList": "handle_teleport_list",
    "SetRequested": "set_requested",
    "IsRequested": "is_requested",
    "SetLocation": "set_location",
    "GetMapCount": "get_map_count",
    "SetMapCount": "set_map_count",
    "GetMapName": "get_map_name",
    "SetMapName": "set_map_name",
    "GetLocX": "get_loc_x",
    "GetLocY": "get_loc_y",

    # ========================================================================
    # EventListManager — Methods
    # ========================================================================
    "AddEventTop": "add_event_top",
    "ShowEvents": "show_events",
    "AddEvent": "add_event",

    # ========================================================================
    # TextInputManager — Methods
    # ========================================================================
    "GetInputString": "get_input_string",
    "ClearInput": "clear_input",
    "StartInput": "start_input",
    "ShowInput": "show_input",
    "HandleChar": "handle_char",
    "EndInput": "end_input",

    # ========================================================================
    # CombatSystem — Methods
    # ========================================================================
    "GetWeaponSkillType": "get_weapon_skill_type",
    "CanSuperAttack": "can_super_attack",
    "GetAttackType": "get_attack_type",
    "SetPlayer": "set_player",

    # ========================================================================
    # MagicCastingSystem — Methods
    # ========================================================================
    "GetManaCost": "get_mana_cost",
    "BeginCast": "begin_cast",

    # ========================================================================
    # SpellAoE — Methods / Static Helpers
    # ========================================================================
    "CalculateTiles": "calculate_tiles",
    "GetWallOffset": "get_wall_offset",
    "GetMoveDir": "get_move_dir",
    "AddUnique": "add_unique",
    "AddArea": "add_area",

    # ========================================================================
    # AudioManager — Methods
    # ========================================================================
    "CleanupFinishedSounds": "cleanup_finished_sounds",
    "GetCurrentMusicTrack": "get_current_music_track",
    "SetListenerPosition": "set_listener_position",
    "FreeDecodedSound": "free_decoded_sound",
    "GetGroupForSound": "get_group_for_sound",
    "IsCategoryEnabled": "is_category_enabled",
    "GetDecodedSound": "get_decoded_sound",
    "SetAmbientVolume": "set_ambient_volume",
    "SetAmbientEnabled": "set_ambient_enabled",
    "GetAmbientVolume": "get_ambient_volume",
    "IsAmbientEnabled": "is_ambient_enabled",
    "IsSoundAvailable": "is_sound_available",
    "SetMasterVolume": "set_master_volume",
    "SetMasterEnabled": "set_master_enabled",
    "GetMasterVolume": "get_master_volume",
    "IsMasterEnabled": "is_master_enabled",
    "SetSoundVolume": "set_sound_volume",
    "SetSoundEnabled": "set_sound_enabled",
    "GetSoundVolume": "get_sound_volume",
    "IsSoundEnabled": "is_sound_enabled",
    "SetMusicVolume": "set_music_volume",
    "SetMusicEnabled": "set_music_enabled",
    "GetMusicVolume": "get_music_volume",
    "IsMusicEnabled": "is_music_enabled",
    "IsMusicPlaying": "is_music_playing",
    "PlayGameSound": "play_game_sound",
    "PlaySoundLoop": "play_sound_loop",
    "StopAllSounds": "stop_all_sounds",
    "VolumeToFloat": "volume_to_float",
    "UnloadSounds": "unload_sounds",
    "SetUIVolume": "set_ui_volume",
    "SetUIEnabled": "set_ui_enabled",
    "GetUIVolume": "get_ui_volume",
    "GetListenerX": "get_listener_x",
    "GetListenerY": "get_listener_y",
    "IsUIEnabled": "is_ui_enabled",
    "LoadSounds": "load_sounds",
    "DecodeFile": "decode_file",
    "StopSound": "stop_sound",
    "PlayMusic": "play_music",
    "StopMusic": "stop_music",

    # ========================================================================
    # ConfigManager — Methods
    # ========================================================================
    "IsFullscreenStretchEnabled": "is_fullscreen_stretch_enabled",
    "SetFullscreenStretchEnabled": "set_fullscreen_stretch_enabled",
    "IsDialogTransparencyEnabled": "is_dialog_transparency_enabled",
    "SetDialogTransparencyEnabled": "set_dialog_transparency_enabled",
    "IsQuickActionsEnabled": "is_quick_actions_enabled",
    "IsPatchingGridEnabled": "is_patching_grid_enabled",
    "SetPatchingGridEnabled": "set_patching_grid_enabled",
    "IsMouseCaptureEnabled": "is_mouse_capture_enabled",
    "SetMouseCaptureEnabled": "set_mouse_capture_enabled",
    "IsShowLatencyEnabled": "is_show_latency_enabled",
    "SetShowLatencyEnabled": "set_show_latency_enabled",
    "IsRunningModeEnabled": "is_running_mode_enabled",
    "SetRunningModeEnabled": "set_running_mode_enabled",
    "IsFullscreenEnabled": "is_fullscreen_enabled",
    "SetFullscreenEnabled": "set_fullscreen_enabled",
    "IsBorderlessEnabled": "is_borderless_enabled",
    "SetBorderlessEnabled": "set_borderless_enabled",
    "IsTileGridEnabled": "is_tile_grid_enabled",
    "SetTileGridEnabled": "set_tile_grid_enabled",
    "IsZoomMapEnabled": "is_zoom_map_enabled",
    "SetZoomMapEnabled": "set_zoom_map_enabled",
    "IsShowFpsEnabled": "is_show_fps_enabled",
    "SetShowFpsEnabled": "set_show_fps_enabled",
    "GetRecentShortcut": "get_recent_shortcut",
    "SetRecentShortcut": "set_recent_shortcut",
    "GetMagicShortcut": "get_magic_shortcut",
    "SetMagicShortcut": "set_magic_shortcut",
    "IsVSyncEnabled": "is_vsync_enabled",
    "SetVSyncEnabled": "set_vsync_enabled",
    "GetWindowHeight": "get_window_height",
    "GetWindowWidth": "get_window_width",
    "GetDetailLevel": "get_detail_level",
    "SetDetailLevel": "set_detail_level",
    "SetWindowSize": "set_window_size",
    "GetShortcut": "get_shortcut",
    "SetShortcut": "set_shortcut",
    "GetFpsLimit": "get_fps_limit",
    "SetFpsLimit": "set_fps_limit",
    "MarkClean": "mark_clean",
    "IsDirty": "is_dirty",

    # ========================================================================
    # CFloatingTextManager — Methods
    # ========================================================================
    "AddDamageFromValue": "add_damage_from_value",
    "RemoveByObjectID": "remove_by_object_id",
    "UpdatePosition": "update_position",
    "ReleaseExpired": "release_expired",
    "AddDamageText": "add_damage_text",
    "AddNotifyText": "add_notify_text",
    "FindFreeSlot": "find_free_slot",
    "DrawMessage": "draw_message",
    "MaxMessages": "max_messages",
    "AddChatText": "add_chat_text",
    "IsOccupied": "is_occupied",
    "DrawSingle": "draw_single",
    "BindToTile": "bind_to_tile",
    "ClearAll": "clear_all",
    "DrawAll": "draw_all",

    # ========================================================================
    # CFloatingText — Methods
    # ========================================================================
    "UsesSpriteFont": "uses_sprite_font",
    "GetParams": "get_params",

    # ========================================================================
    # EffectManager — Methods
    # ========================================================================
    "DrawEffectLightsImpl": "draw_effect_lights_impl",
    "UpdateEffectsImpl": "update_effects_impl",
    "DrawEffectsImpl": "draw_effects_impl",
    "DrawEffectLights": "draw_effect_lights",
    "ClearAllEffects": "clear_all_effects",
    "SetEffectSprites": "set_effect_sprites",
    "AddEffectImpl": "add_effect_impl",
    "DrawEffects": "draw_effects",
    # "AddEffect" REMOVED — collides with ItemEffectType::AddEffect (Shared enum)

    # ========================================================================
    # AnimationState — Methods
    # ========================================================================
    "SetDirection": "set_direction",
    "JustChangedTo": "just_changed_to",
    "FrameChanged": "frame_changed",
    "IsFinished": "is_finished",
    "SetAction": "set_action",

    # ========================================================================
    # GetCharKind — shared free function (FloatingTextManager + TextInputManager)
    # ========================================================================
    "GetCharKind": "get_char_kind",

    # ========================================================================
    # CItem members (Shared/Item/Item.h — definitions renamed by server script)
    # Client code accessing CItem members MUST match the new Shared definitions.
    # ========================================================================
    "m_sItemEffectValue1": "m_item_effect_value1",
    "m_sItemEffectValue2": "m_item_effect_value2",
    "m_sItemEffectValue3": "m_item_effect_value3",
    "m_sItemEffectValue4": "m_item_effect_value4",
    "m_sItemEffectValue5": "m_item_effect_value5",
    "m_sItemEffectValue6": "m_item_effect_value6",
    "m_sItemSpecEffectValue1": "m_item_special_effect_value1",
    "m_sItemSpecEffectValue2": "m_item_special_effect_value2",
    "m_sItemSpecEffectValue3": "m_item_special_effect_value3",
    "m_sSpecialEffectValue1": "m_special_effect_value1",
    "m_sSpecialEffectValue2": "m_special_effect_value2",
    "m_sTouchEffectValue1": "m_touch_effect_value1",
    "m_sTouchEffectValue2": "m_touch_effect_value2",
    "m_sTouchEffectValue3": "m_touch_effect_value3",
    "m_sItemEffectType": "m_item_effect_type",
    "m_sTouchEffectType": "m_touch_effect_type",
    "m_sSpecialEffect": "m_special_effect",
    "m_sSpriteFrame": "m_sprite_frame",
    "m_sRelatedSkill": "m_related_skill",
    "m_wMaxLifeSpan": "m_max_life_span",
    "m_wCurLifeSpan": "m_cur_life_span",
    "m_cGenderLimit": "m_gender_limit",
    "m_sLevelLimit": "m_level_limit",
    "m_cApprValue": "m_appearance_value",
    "m_dwAttribute": "m_attribute",
    "m_cItemColor": "m_item_color",
    "m_cItemType": "m_item_type",
    "m_cEquipPos": "m_equip_pos",
    "m_bIsForSale": "m_is_for_sale",
    "m_cCategory": "m_category",
    "m_dwCount": "m_count",
    "m_sIDnum": "m_id_num",
    "m_sSprite": "m_sprite",
    "m_wWeight": "m_weight",
    "m_wPrice": "m_price",
    "m_cSpeed": "m_speed",

    # ========================================================================
    # Shared name member (CItem, CBuildItem, CMagic, CSkill, CCharInfo)
    # All use m_cName for their name field — all become m_name
    # ========================================================================
    "m_cName": "m_name",

    # ========================================================================
    # ASIOSocket members (Shared/Net/ASIOSocket.h — definitions renamed)
    # Client accesses m_bIsAvailable through m_pGSock/m_pLSock
    # ========================================================================
    "m_bIsAvailable": "m_is_available",

    # ========================================================================
    # CItem methods (Shared/Item/Item.h — renamed by server script)
    # Client code calling these methods MUST match the new Shared definitions.
    # ========================================================================
    "GetItemEffectType": "get_item_effect_type",
    "GetTouchEffectType": "get_touch_effect_type",
    "SetItemEffectType": "set_item_effect_type",
    "SetTouchEffectType": "set_touch_effect_type",
    "GetAttributeValue": "get_attribute_value",
    "GetAttributeType": "get_attribute_type",
    "GetDisplayName": "get_display_name",
    "SetCustomMade": "set_custom_made",
    "IsCustomMade": "is_custom_made",
    "GetItemType": "get_item_type",
    "GetEquipPos": "get_equip_pos",
    "SetItemType": "set_item_type",
    "SetEquipPos": "set_equip_pos",
    "IsStackable": "is_stackable",
    "IsAccessory": "is_accessory",
    "IsWeapon": "is_weapon",
    "IsArmor": "is_armor",

    # ========================================================================
    # ItemEnums.h free functions (Shared — renamed by server script)
    # ========================================================================
    "IsConsumableEffectType": "is_consumable_effect_type",
    "IsAttackEffectType": "is_attack_effect_type",
    "ToTouchEffectType": "to_touch_effect_type",
    "ToItemEffectType": "to_item_effect_type",
    "IsTrueStackType": "is_true_stack_type",
    "IsStackableType": "is_stackable_type",
    "IsAccessorySlot": "is_accessory_slot",
    "IsSpecialItem": "is_special_item",
    "IsWeaponSlot": "is_weapon_slot",
    "IsArmorSlot": "is_armor_slot",
    "ToItemType": "to_item_type",
    "ToEquipPos": "to_equip_pos",
    "ToInt": "to_int",

    # ========================================================================
    # ASIOSocket methods (Shared/Net/ASIOSocket.h — renamed by server script)
    # ========================================================================
    "QueueCompletedPacket": "queue_completed_packet",
    "pGetRcvDataPointer": "get_rcv_data_pointer",
    "bInitBufferSize": "init_buffer_size",
    "DrainToQueue": "drain_to_queue",
    "PeekPacket": "peek_packet",
    "PopPacket": "pop_packet",
    "iSendMsg": "send_msg",
    "bConnect": "connect",

    # ========================================================================
    # IOServicePool method (Shared/Net/IOServicePool.h — renamed by server script)
    # ========================================================================
    "GetContext": "get_context",

    # ========================================================================
    # ResolutionConfig method (Shared/Render/ResolutionConfig.h — already snake_case)
    # Qualified to avoid renaming client-local Initialize() methods
    # ========================================================================
    "ResolutionConfig::Initialize": "ResolutionConfig::initialize",

    # ========================================================================
    # NetMessages enum values (Shared/Net/NetMessages.h — renamed by server script)
    # Qualified patterns to avoid renaming declarations (which are in Shared, not Client)
    # ========================================================================
    "CommonType::ConfirmExchangeItem": "CommonType::confirm_exchange_item",
    "CommonType::CancelExchangeItem": "CommonType::cancel_exchange_item",
    "MsgId::RequestFullObjectData": "MsgId::request_full_object_data",
    "MsgId::RequestCreateNewGuild": "MsgId::request_create_new_guild",
    "CommonType::SetExchangeItem": "CommonType::set_exchange_item",
    "MsgId::RequestDisbandGuild": "MsgId::request_disband_guild",
    "Notify::CancelExchangeItem": "Notify::cancel_exchange_item",
    "MsgId::RequestEnterGame": "MsgId::request_enter_game",
    "Notify::SetExchangeItem": "Notify::set_exchange_item",
    "Notify::SetItemCount": "Notify::set_item_count",
    "MsgId::RequestLogin": "MsgId::request_login",

    # ========================================================================
    # CTile members (Client/Tile.h — heavily accessed from rendering/network)
    # ========================================================================
    "m_cDynamicObjectData1": "m_dynamic_object_data_1",
    "m_cDynamicObjectData2": "m_dynamic_object_data_2",
    "m_cDynamicObjectData3": "m_dynamic_object_data_3",
    "m_cDynamicObjectData4": "m_dynamic_object_data_4",
    "m_dwDynamicObjectTime": "m_dynamic_object_time",
    "m_sDynamicObjectType": "m_dynamic_object_type",
    "m_cDynamicObjectFrame": "m_dynamic_object_frame",
    "m_iEffectTotalFrame": "m_effect_total_frame",
    "m_cDeadOwnerFrame": "m_dead_owner_frame",
    "m_dwDeadOwnerTime": "m_dead_owner_time",
    "m_cDeadOwnerName": "m_dead_owner_name",
    "m_wDeadObjectID": "m_dead_object_id",
    "m_sDeadOwnerType": "m_dead_owner_type",
    "m_iDeadChatMsg": "m_dead_chat_msg",
    "m_dwEffectTime": "m_effect_time",
    "m_iEffectFrame": "m_effect_frame",
    "m_iEffectType": "m_effect_type",
    "m_sOwnerType": "m_owner_type",
    "m_wObjectID": "m_object_id",
    "m_cOwnerName": "m_owner_name",
    "m_dwItemAttr": "m_item_attr",
    "m_iChatMsg": "m_chat_msg",
    "m_cDeadDir": "m_dead_dir",
    "m_sItemID": "m_item_id",
    "m_sV1": "m_v1",
    "m_sV2": "m_v2",
    "m_sV3": "m_v3",

    # ========================================================================
    # CMagic members (Client/Magic.h)
    # ========================================================================
    "m_sDynamicPattern": "m_dynamic_pattern",
    "m_sDynamicRadius": "m_dynamic_radius",
    "m_sAoERadiusX": "m_aoe_radius_x",
    "m_sAoERadiusY": "m_aoe_radius_y",
    "m_bIsVisible": "m_is_visible",
    "m_sValue1": "m_value_1",
    "m_sValue2": "m_value_2",
    "m_sValue3": "m_value_3",
    "m_sValue4": "m_value_4",

    # ========================================================================
    # CSkill members (Client/Skill.h — also in Shared, renamed by server script)
    # ========================================================================
    "m_bIsUseable": "m_is_useable",
    "m_cUseMethod": "m_use_method",
    "m_iLevel": "m_level",

    # ========================================================================
    # CCharInfo members (Client/CharInfo.h)
    # ========================================================================
    "m_cMapName": "m_map_name",
    "m_sSkinCol": "m_skin_color",
    "m_sSex": "m_sex",
    "m_sStr": "m_str",
    "m_sVit": "m_vit",
    "m_sDex": "m_dex",
    "m_sInt": "m_int",
    "m_sMag": "m_mag",
    "m_sChr": "m_chr",
    "m_sLevel": "m_level",
    "m_iExp": "m_exp",
    "m_iYear": "m_year",
    "m_iMonth": "m_month",
    "m_iDay": "m_day",
    "m_iHour": "m_hour",
    "m_iMinute": "m_minute",

    # ========================================================================
    # CEntityRenderState members (Client/EntityRenderState.h)
    # m_wObjectID, m_sOwnerType, m_iEffectType, m_iEffectFrame — shared with CTile
    # ========================================================================
    "m_iMoveOffsetX": "m_move_offset_x",
    "m_iMoveOffsetY": "m_move_offset_y",
    "m_iChatIndex": "m_chat_index",
    "m_iDataX": "m_data_x",
    "m_iDataY": "m_data_y",
    "m_iAction": "m_action",
    "m_iFrame": "m_frame",
    "m_iDir": "m_dir",

    # ========================================================================
    # CMapData members (Client/MapData.h — 868 m_pData hits + other members)
    # ========================================================================
    "m_iObjectIDcacheLocX": "m_object_id_cache_loc_x",
    "m_iObjectIDcacheLocY": "m_object_id_cache_loc_y",
    "m_dwFrameAdjustTime": "m_frame_adjust_time",
    "m_dwFrameCheckTime": "m_frame_check_time",
    "m_dwDOframeTime": "m_dynamic_object_frame_time",
    "m_sMapSizeX": "m_map_size_x",
    "m_sMapSizeY": "m_map_size_y",
    "m_pTmpData": "m_tmp_data",
    "m_sPivotX": "m_pivot_x",
    "m_sPivotY": "m_pivot_y",
    "m_sRectX": "m_rect_x",
    "m_sRectY": "m_rect_y",
    "m_pData": "m_data",
}

# ============================================================================
# File processing
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
        # Strip BOM (\ufeff) in addition to whitespace — some files have UTF-8 BOM
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
    # e.g., CFloatingTextManager before CFloatingText
    renames_sorted = []
    for old, new in sorted(RENAMES.items(), key=lambda x: -len(x[0])):
        pattern = re.compile(r'\b' + re.escape(old) + r'\b')
        renames_sorted.append((old, new, pattern))

    print(f"{'DRY RUN — ' if dry_run else ''}Client snake_case conversion")
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

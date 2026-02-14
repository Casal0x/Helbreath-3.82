#!/usr/bin/env python3
"""Phase 6: Client + server member variable renames
Target: Game.h (~160), Player.h (~72), Camera.h (8), PlayerController.h (12),
        Tile.h (3), ServerConsole.h (8) class members + Game.h struct members.
Mode 2 justified: ~260 renames across ~80 files.
"""

import re
import os
import sys

BASE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# ═══════════════════════════════════════════════════════════════════════════
# HEADER DEFINITION FIXES — exact string replacements in struct blocks
# ═══════════════════════════════════════════════════════════════════════════
HEADER_DEFS = {
    os.path.join(BASE, "Sources", "Client", "Game.h"): [
        # m_stDialogBoxExchangeInfo — remaining
        ("int   v1, v2, v3, v4, v5, v6, v7, sItemID;",
         "int   v1, v2, v3, v4, v5, v6, v7, item_id;"),
        ("std::string cStr1;", "std::string str1;"),
        # m_stSellItemList
        ("int iIndex;\n\t\tint iAmount;", "int index;\n\t\tint amount;"),
        # m_stGuildOpList
        ("std::string cName;\n\t\tchar cOpMode;", "std::string name;\n\t\tchar op_mode;"),
        # m_quest — remaining
        ("bool bIsQuestCompleted;", "bool is_quest_completed;"),
        ("short sWho, sQuestType, sContribution, sTargetType, sTargetCount, x, y, sRange;",
         "short who, quest_type, contribution, target_type, target_count, x, y, range;"),
        ("short sCurrentCount;", "short current_count;"),
        ("std::string cTargetName;", "std::string target_name;"),
        # m_stPartyMember
        ("char cStatus;\n\t\tstd::string cName;\n\t} m_stPartyMember",
         "char status;\n\t\tstd::string name;\n\t} m_stPartyMember"),
        # m_stCrusadeStructureInfo — remaining
        ("char cType;\n\t\tchar cSide;\n\t} m_stCrusadeStructureInfo",
         "char type;\n\t\tchar side;\n\t} m_stCrusadeStructureInfo"),
        # m_stPartyMemberNameList
        ("std::string cName;\n\t} m_stPartyMemberNameList",
         "std::string name;\n\t} m_stPartyMemberNameList"),
        # m_guild_name_cache
        ("uint32_t dwRefTime;", "uint32_t ref_time;"),
        ("int iGuildRank;", "int guild_rank;"),
        ("std::string cCharName;", "std::string char_name;"),
        ("std::string cGuildName;", "std::string guild_name;"),
    ],
}

# ═══════════════════════════════════════════════════════════════════════════
# SAFE RENAMES — m_-prefixed class members (word-boundary safe)
# Sorted longest-first to prevent prefix clobbering.
# ═══════════════════════════════════════════════════════════════════════════
SAFE_RENAMES = sorted([
    # ── Game.h m_dw* ──
    ("m_dwCommanderCommandRequestedTime", "m_commander_command_requested_time"),
    ("m_dwSpecialAbilitySettingTime", "m_special_ability_setting_time"),
    ("m_dwCheckConnectionTime", "m_check_connection_time"),
    ("m_dwRestartCountTime", "m_restart_count_time"),
    ("m_dwConfigRequestTime", "m_config_request_time"),
    ("m_dwOverlayStartTime", "m_overlay_start_time"),
    ("m_dwLastNetRecvTime", "m_last_net_recv_time"),
    ("m_dwLastNpcEventTime", "m_last_npc_event_time"),
    ("m_dwMonsterEventTime", "m_monster_event_time"),
    ("m_dwLastNetMsgTime", "m_last_net_msg_time"),
    ("m_dwLastNetMsgSize", "m_last_net_msg_size"),
    ("m_dwObserverCamTime", "m_observer_cam_time"),
    ("m_dwCheckConnTime", "m_check_conn_time"),
    ("m_dwCheckSprTime", "m_check_spr_time"),
    ("m_dwCheckChatTime", "m_check_chat_time"),
    ("m_dwMagicCastTime", "m_magic_cast_time"),
    ("m_dwLastNetMsgId", "m_last_net_msg_id"),
    ("m_dwEnvEffectTime", "m_env_effect_time"),
    ("m_dwConnectMode", "m_connect_mode"),
    ("m_dwDamagedTime", "m_damaged_time"),
    ("m_dwTopMsgTime", "m_top_msg_time"),
    ("m_dwCurTime", "m_cur_time"),
    ("m_dwFPStime", "m_fps_time"),
    # ── Game.h m_i* ──
    ("m_iHeldenianAresdenLeftTower", "m_heldenian_aresden_left_tower"),
    ("m_iHeldenianElvineLeftTower", "m_heldenian_elvine_left_tower"),
    ("m_iHeldenianAresdenFlags", "m_heldenian_aresden_flags"),
    ("m_iHeldenianElvineFlags", "m_heldenian_elvine_flags"),
    ("m_iGizonItemUpgradeLeft", "m_gizon_item_upgrade_left"),
    ("m_iFightzoneNumberTemp", "m_fightzone_number_temp"),
    ("m_iNpcConfigsReceived", "m_npc_configs_received"),
    ("m_iTimeLeftSecAccount", "m_time_left_sec_account"),
    ("m_iContributionPrice", "m_contribution_price"),
    ("m_iPointCommandType", "m_point_command_type"),
    ("m_iCastingMagicType", "m_casting_magic_type"),
    ("m_iTotalPartyMember", "m_total_party_member"),
    ("m_iFightzoneNumber", "m_fightzone_number"),
    ("m_iTimeLeftSecIP", "m_time_left_sec_ip"),
    ("m_iDownSkillIndex", "m_down_skill_index"),
    ("m_iIlusionOwnerH", "m_ilusion_owner_h"),
    ("m_iGameServerPort", "m_game_server_port"),
    ("m_iLogServerPort", "m_log_server_port"),
    ("m_iTopMsgLastSec", "m_top_msg_last_sec"),
    ("m_iItemDropCnt", "m_item_drop_cnt"),
    ("m_iGatePositX", "m_gate_posit_x"),
    ("m_iGatePositY", "m_gate_posit_y"),
    ("m_iNetLagCount", "m_net_lag_count"),
    ("m_iPartyStatus", "m_party_status"),
    ("m_iBlockMonth", "m_block_month"),
    ("m_iAccntMonth", "m_accnt_month"),
    ("m_iLatencyMs", "m_latency_ms"),
    ("m_iTotalChar", "m_total_char"),
    ("m_iBlockYear", "m_block_year"),
    ("m_iAccntYear", "m_accnt_year"),
    ("m_iBlockDay", "m_block_day"),
    ("m_iAccntDay", "m_accnt_day"),
    ("m_iDrawFlag", "m_draw_flag"),
    ("m_iIpMonth", "m_ip_month"),
    ("m_iIpYear", "m_ip_year"),
    ("m_iIpDay", "m_ip_day"),
    # ── Game.h m_s* ──
    ("m_sItemEquipmentStatus", "m_item_equipment_status"),
    ("m_sRecentShortCut", "m_recent_short_cut"),
    ("m_sMagicShortCut", "m_magic_short_cut"),
    ("m_sItemDropID", "m_item_drop_id"),
    ("m_sMonsterID", "m_monster_id"),
    ("m_sShortCut", "m_short_cut"),
    ("m_sEventX", "m_event_x"),
    ("m_sEventY", "m_event_y"),
    ("m_sVDL_X", "m_vdl_x"),
    ("m_sVDL_Y", "m_vdl_y"),
    ("m_sMCX", "m_mcx"),
    ("m_sMCY", "m_mcy"),
    # ── Game.h m_b* ──
    ("m_bIsF1HelpWindowEnabled", "m_is_f1_help_window_enabled"),
    ("m_bIsObserverCommanded", "m_is_observer_commanded"),
    ("m_bIsGetPointingMode", "m_is_get_pointing_mode"),
    ("m_bIsServerChanging", "m_is_server_changing"),
    ("m_bMouseInitialized", "m_mouse_initialized"),
    ("m_bSkillUsingStatus", "m_skill_using_status"),
    ("m_bItemUsingStatus", "m_item_using_status"),
    ("m_bIsItemEquipped", "m_is_item_equipped"),
    ("m_bIsItemDisabled", "m_is_item_disabled"),
    ("m_bWaitForNewClick", "m_wait_for_new_click"),
    ("m_bHideLocalCursor", "m_hide_local_cursor"),
    ("m_bIsObserverMode", "m_is_observer_mode"),
    ("m_bIsCrusadeMode", "m_is_crusade_mode"),
    ("m_bInitDataReady", "m_init_data_ready"),
    ("m_bForceDisconn", "m_force_disconn"),
    ("m_bConfigsReady", "m_configs_ready"),
    ("m_bIsFirstConn", "m_is_first_conn"),
    ("m_bIllusionMVT", "m_illusion_mvt"),
    ("m_bUsingSlate", "m_using_slate"),
    ("m_bItemDrop", "m_item_drop"),
    ("m_bIsXmas", "m_is_xmas"),
    # ── Game.h m_w* ──
    ("m_wLastAttackTargetID", "m_last_attack_target_id"),
    ("m_wEnterGameType", "m_enter_game_type"),
    ("m_wCommObjectID", "m_comm_object_id"),
    # ── Game.h m_c* ──
    ("m_cConstructMapName", "m_construct_map_name"),
    ("m_cIlusionOwnerType", "m_ilusion_owner_type"),
    ("m_cGameServerName", "m_game_server_name"),
    ("m_cWorldServerName", "m_world_server_name"),
    ("m_cBackupChatMsg", "m_backup_chat_msg"),
    ("m_cStatusMapName", "m_status_map_name"),
    ("m_cLogServerAddr", "m_log_server_addr"),
    ("m_cOverlayMessage", "m_overlay_message"),
    ("m_cOverlayContext", "m_overlay_context"),
    ("m_cNpcNameByType", "m_npc_name_by_type"),
    ("m_cAmountString", "m_amount_string"),
    ("m_cRestartCount", "m_restart_count"),
    ("m_cArrowPressed", "m_arrow_pressed"),
    ("m_cMapMessage", "m_map_message"),
    ("m_cGateMapName", "m_gate_map_name"),
    ("m_cCurLocation", "m_cur_location"),
    ("m_cMenuDirCnt", "m_menu_dir_cnt"),
    ("m_cMenuFrame", "m_menu_frame"),
    ("m_cItemOrder", "m_item_order"),
    ("m_cMaxFocus", "m_max_focus"),
    ("m_cCurFocus", "m_cur_focus"),
    ("m_cMapIndex", "m_map_index"),
    ("m_cLocation", "m_location"),
    ("m_cDiscount", "m_discount"),
    ("m_cMenuDir", "m_menu_dir"),
    ("m_cChatMsg", "m_chat_msg"),
    ("m_cLoading", "m_loading"),
    ("m_cName_IE", "m_name_ie"),
    ("m_cMCName", "m_mc_name"),
    ("m_cTopMsg", "m_top_msg"),
    ("m_cMsg", "m_msg"),
    # ── Game.h m_p* ──
    ("m_pNetworkMessageManager", "m_network_message_manager"),
    ("m_pAgreeMsgTextList", "m_agree_msg_text_list"),
    ("m_pSpriteFactory", "m_sprite_factory"),
    ("m_pEffectManager", "m_effect_manager"),
    ("m_pItemConfigList", "m_item_config_list"),
    ("m_pMagicCfgList", "m_magic_cfg_list"),
    ("m_pSkillCfgList", "m_skill_cfg_list"),
    ("m_pMsgTextList2", "m_msg_text_list2"),
    ("m_pGameMsgList", "m_game_msg_list"),
    ("m_pMsgTextList", "m_msg_text_list"),
    ("m_pBankList", "m_bank_list"),
    ("m_pItemList", "m_item_list"),
    ("m_pCharList", "m_char_list"),
    ("m_pMapData", "m_map_data"),
    ("m_pTileSpr", "m_tile_spr"),
    ("m_pPlayer", "m_player"),
    ("m_pIOPool", "m_io_pool"),
    ("m_pSprite", "m_sprite"),
    ("m_pGSock", "m_g_sock"),
    ("m_pLSock", "m_l_sock"),
    ("m_pExID", "m_ex_id"),
    # ── Game.h m_rc*, m_e*, camelCase ──
    ("m_stDialogBoxExchangeInfo", "m_dialog_box_exchange_info"),
    ("m_stCrusadeStructureInfo", "m_crusade_structure_info"),
    ("m_stPartyMemberNameList", "m_party_member_name_list"),
    ("m_pendingLoginPacket", "m_pending_login_packet"),
    ("m_dialogBoxManager", "m_dialog_box_manager"),
    ("m_stSellItemList", "m_sell_item_list"),
    ("m_fishingManager", "m_fishing_manager"),
    ("m_craftingManager", "m_crafting_manager"),
    ("m_stGuildOpList", "m_guild_op_list"),
    ("m_stPartyMember", "m_party_member"),
    ("m_questManager", "m_quest_manager"),
    ("m_guildManager", "m_guild_manager"),
    ("m_playerRenderer", "m_player_renderer"),
    ("m_npcConfigList", "m_npc_config_list"),
    ("m_activeOverlay", "m_active_overlay"),
    ("m_floatingText", "m_floating_text"),
    ("m_stRepairAll", "m_repair_all"),
    ("m_npcRenderer", "m_npc_renderer"),
    ("m_entityState", "m_entity_state"),
    ("m_rcPlayerRect", "m_player_rect"),
    ("m_eConfigRetry", "m_config_retry"),
    ("m_rcBodyRect", "m_body_rect"),
    # ── Game.h standalone (no m_ prefix) ──
    ("iMaxBankItems", "m_max_bank_items"),
    ("iMaxStats", "m_max_stats"),
    ("iMaxLevel", "m_max_level"),

    # ═══════════════════════════════════════════
    # Player.h members
    # ═══════════════════════════════════════════
    ("m_iSpecialAbilityTimeLeftSec", "m_special_ability_time_left_sec"),
    ("m_bIsSpecialAbilityEnabled", "m_is_special_ability_enabled"),
    ("m_iSpecialAbilityType", "m_special_ability_type"),
    ("m_sDamageMoveAmount", "m_damage_move_amount"),
    ("m_bIsSafeAttackMode", "m_is_safe_attack_mode"),
    ("m_iConstructionPoint", "m_construction_point"),
    ("m_sPlayerObjectID", "m_player_object_id"),
    ("m_bSuperAttackMode", "m_super_attack_mode"),
    ("m_iEnemyKillCount", "m_enemy_kill_count"),
    ("m_iWarContribution", "m_war_contribution"),
    ("m_iSuperAttackLeft", "m_super_attack_left"),
    ("m_cAccountPassword", "m_account_password"),
    ("m_iConstructLocX", "m_construct_loc_x"),
    ("m_iConstructLocY", "m_construct_loc_y"),
    ("m_bIsCombatMode", "m_is_combat_mode"),
    ("m_iMagicMastery", "m_magic_mastery"),
    ("m_iSkillMastery", "m_skill_mastery"),
    ("m_iHungerStatus", "m_hunger_status"),
    ("m_cAccountName", "m_account_name"),
    ("m_cPlayerName", "m_player_name"),
    ("m_iCrusadeDuty", "m_crusade_duty"),
    ("m_iContribution", "m_contribution"),
    ("m_bForceAttack", "m_force_attack"),
    ("m_iRewardGold", "m_reward_gold"),
    ("m_sPlayerType", "m_player_type"),
    ("m_bIsConfusion", "m_is_confusion"),
    ("m_bIsPoisoned", "m_is_poisoned"),
    ("m_iAngelicStr", "m_angelic_str"),
    ("m_iAngelicInt", "m_angelic_int"),
    ("m_iAngelicDex", "m_angelic_dex"),
    ("m_iAngelicMag", "m_angelic_mag"),
    ("m_sDamageMove", "m_damage_move"),
    ("m_iStatModStr", "m_stat_mod_str"),
    ("m_iStatModVit", "m_stat_mod_vit"),
    ("m_iStatModDex", "m_stat_mod_dex"),
    ("m_iStatModInt", "m_stat_mod_int"),
    ("m_iStatModMag", "m_stat_mod_mag"),
    ("m_iStatModChr", "m_stat_mod_chr"),
    ("m_cGuildName", "m_guild_name"),
    ("m_iPlayerDir", "m_player_dir"),
    ("m_iGuildRank", "m_guild_rank"),
    ("m_iHairStyle", "m_hair_style"),
    ("m_sPlayerX", "m_player_x"),
    ("m_sPlayerY", "m_player_y"),
    ("m_bParalyze", "m_paralyze"),
    ("m_iCharisma", "m_charisma"),
    ("m_iPKCount", "m_pk_count"),
    ("m_iUnderCol", "m_under_col"),
    ("m_bIsGMMode", "m_is_gm_mode"),
    ("m_iSkinCol", "m_skin_col"),
    ("m_iHairCol", "m_hair_col"),
    ("m_iLU_Point", "m_lu_point"),
    ("m_bAresden", "m_aresden"),
    ("m_bCitizen", "m_citizen"),
    ("m_bHunter", "m_hunter"),
    ("m_wLU_Str", "m_lu_str"),
    ("m_wLU_Vit", "m_lu_vit"),
    ("m_wLU_Dex", "m_lu_dex"),
    ("m_wLU_Int", "m_lu_int"),
    ("m_wLU_Mag", "m_lu_mag"),
    ("m_wLU_Char", "m_lu_char"),
    ("m_iGender", "m_gender"),
    ("m_iTHAC0", "m_thac0"),
    ("m_iStr", "m_str"),
    ("m_iVit", "m_vit"),
    ("m_iDex", "m_dex"),
    ("m_iInt", "m_int"),
    ("m_iMag", "m_mag"),
    ("m_iHP", "m_hp"),
    ("m_iMP", "m_mp"),
    ("m_iSP", "m_sp"),
    ("m_iAC", "m_ac"),

    # ═══════════════════════════════════════════
    # Camera.h members
    # ═══════════════════════════════════════════
    ("m_iSavedPositionX", "m_saved_position_x"),
    ("m_iSavedPositionY", "m_saved_position_y"),
    ("m_dwLastUpdateTime", "m_last_update_time"),
    ("m_iDestinationX", "m_destination_x"),
    ("m_iDestinationY", "m_destination_y"),
    ("m_iShakeDegree", "m_shake_degree"),
    ("m_iPositionX", "m_position_x"),
    ("m_iPositionY", "m_position_y"),

    # ═══════════════════════════════════════════
    # PlayerController.h members
    # ═══════════════════════════════════════════
    ("m_bIsPrevMoveBlocked", "m_is_prev_move_blocked"),
    ("m_bCommandAvailable", "m_command_available"),
    ("m_cPendingStopDir", "m_pending_stop_dir"),
    ("m_dwAttackEndTime", "m_attack_end_time"),
    ("m_dwCommandTime", "m_command_time"),
    ("m_cCommandCount", "m_command_count"),
    ("m_cPlayerTurn", "m_player_turn"),
    ("m_iPrevMoveX", "m_prev_move_x"),
    ("m_iPrevMoveY", "m_prev_move_y"),
    ("m_cCommand", "m_command"),
    ("m_sDestX", "m_dest_x"),
    ("m_sDestY", "m_dest_y"),

    # ═══════════════════════════════════════════
    # Tile.h members
    # ═══════════════════════════════════════════
    ("m_sDeadNpcConfigId", "m_dead_npc_config_id"),
    ("m_deadAppearance", "m_dead_appearance"),
    ("m_sNpcConfigId", "m_npc_config_id"),

    # ═══════════════════════════════════════════
    # Server ServerConsole.h members
    # ═══════════════════════════════════════════
    ("m_origTermios", "m_orig_termios"),
    ("m_dwOrigMode", "m_orig_mode"),
    ("m_iCursorPos", "m_cursor_pos"),
    ("m_iInputLen", "m_input_len"),
    ("m_szInput", "m_input"),
    ("m_bInit", "m_init"),
    ("m_hOut", "m_out"),
    ("m_hIn", "m_in"),

    # ═══════════════════════════════════════════
    # Server Game.h — m_commandPermissions
    # ═══════════════════════════════════════════
    ("m_commandPermissions", "m_command_permissions"),

], key=lambda x: -len(x[0]))  # sort longest-first

# ═══════════════════════════════════════════════════════════════════════════
# SAFE STRUCT RENAMES — distinctive struct member names (word-boundary)
# ═══════════════════════════════════════════════════════════════════════════
SAFE_STRUCT_RENAMES = sorted([
    # m_quest members
    ("bIsQuestCompleted", "is_quest_completed"),
    ("sCurrentCount", "current_count"),
    ("sTargetCount", "target_count"),
    ("sContribution", "contribution"),
    ("sTargetType", "target_type"),
    ("cTargetName", "target_name"),
    ("sQuestType", "quest_type"),
    # m_stDialogBoxExchangeInfo
    ("sItemID", "item_id"),
    # m_guild_name_cache
    ("dwRefTime", "ref_time"),
    ("cCharName", "char_name"),
    ("cGuildName", "guild_name"),
    ("iGuildRank", "guild_rank"),
    # m_stGuildOpList
    ("cOpMode", "op_mode"),
], key=lambda x: -len(x[0]))

# ═══════════════════════════════════════════════════════════════════════════
# CONTEXT STRUCT RENAMES — ambiguous short names (dot/arrow access only)
# ═══════════════════════════════════════════════════════════════════════════
CONTEXT_RENAMES = [
    # Sort longest first
    ("iAmount", "amount"),
    ("iIndex", "index"),
    ("cStatus", "status"),
    ("cStr1", "str1"),
    ("cSide", "side"),
    ("cType", "type"),
    ("cName", "name"),
    ("sRange", "range"),
    ("sWho", "who"),
]


def collect_files():
    """Collect all source files to scan."""
    files = []
    for d in ["Sources/Client", "Sources/Server", "Sources/SFMLEngine"]:
        dirpath = os.path.join(BASE, d)
        if not os.path.isdir(dirpath):
            continue
        for name in sorted(os.listdir(dirpath)):
            if name.endswith((".h", ".cpp")):
                files.append(os.path.join(dirpath, name))
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
                content = content.replace(old, new, 1)
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
    """Apply all renames to files."""
    # Pre-compile patterns
    safe_patterns = [(re.compile(r"\b" + re.escape(old) + r"\b"), new)
                     for old, new in SAFE_RENAMES]
    struct_patterns = [(re.compile(r"\b" + re.escape(old) + r"\b"), new)
                       for old, new in SAFE_STRUCT_RENAMES]
    ctx_patterns = [(re.compile(r"(?<=[.>])" + re.escape(old) + r"\b"), new)
                    for old, new in CONTEXT_RENAMES]

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

            # Apply SAFE m_-prefixed renames (word-boundary)
            for pattern, new in safe_patterns:
                line = pattern.sub(new, line)

            # Apply SAFE struct member renames (word-boundary)
            for pattern, new in struct_patterns:
                line = pattern.sub(new, line)

            # Apply CONTEXT struct member renames (dot/arrow only)
            for pattern, new in ctx_patterns:
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
    print(f"Phase 6: Client member variable renames [{mode}]")
    print(f"  SAFE renames: {len(SAFE_RENAMES)}")
    print(f"  SAFE struct renames: {len(SAFE_STRUCT_RENAMES)}")
    print(f"  Context struct renames: {len(CONTEXT_RENAMES)}")
    print()

    files = collect_files()
    print(f"Scanning {len(files)} source files...")
    print()

    # Step 1: Header definitions
    print("-- Header struct definitions --")
    def_count, def_files = apply_header_defs(dry_run)
    print(f"  {def_count} definition replacements")
    print()

    # Step 2: All renames
    print("-- Member renames --")
    mod_files, changes = apply_renames(files, dry_run)
    print()

    all_modified = sorted(set(def_files + mod_files))
    print(f"Summary: {len(all_modified)} files modified, ~{def_count + changes} changes")

    return 0


if __name__ == "__main__":
    sys.exit(main())

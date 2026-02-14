#!/usr/bin/env python3
"""
Server coding standard conversion: Hungarian notation → snake_case member variables.

Mode 2 script - justified: 680+ renames across 127 server source files.

Approach:
- Process each rename one at a time using word-boundary regex
- Sort by length (longest first) to avoid partial matches
- Skip #include lines to avoid path corruption
- Report per-file statistics

Usage:
    python Scripts/server_snake_case.py --dry-run    # Preview changes
    python Scripts/server_snake_case.py              # Apply changes
"""

import os
import re
import sys
import json
from pathlib import Path

# ============================================================================
# Master rename dictionary: old_name → new_name
# Aggregated from 8 analysis agents, conflicts resolved
# ============================================================================

RENAMES = {
    # ========================================================================
    # CNpc (Npc.h) — 90 renames
    # ========================================================================
    "m_pMagicConfigList": "m_magic_config_list",
    "m_cNpcName": "m_npc_name",
    # m_cName: shared across CNpc, CMagic, CSkill, CBuildItem, CPortion, CNpcItem, CGuildsMan
    "m_cMapIndex": "m_map_index",
    "m_sX": "m_x",
    "m_sY": "m_y",
    "m_sPrevX": "m_prev_x",
    "m_sPrevY": "m_prev_y",
    "m_dX": "m_dx",
    "m_dY": "m_dy",
    "m_vX": "m_vx",
    "m_vY": "m_vy",
    "m_tmp_iError": "m_tmp_error",
    "m_rcRandomArea": "m_random_area",
    "m_cDir": "m_dir",
    "m_cAction": "m_action",
    "m_cTurn": "m_turn",
    "m_sType": "m_type",
    "m_sOriginalType": "m_original_type",
    "m_iNpcConfigId": "m_npc_config_id",
    "m_dwTime": "m_time",
    "m_dwActionTime": "m_action_time",
    "m_dwHPupTime": "m_hp_up_time",
    "m_dwMPupTime": "m_mp_up_time",
    "m_dwDeadTime": "m_dead_time",
    "m_dwRegenTime": "m_regen_time",
    "m_iHP": "m_hp",
    "m_iMaxHP": "m_max_hp",
    "m_iExp": "m_exp",
    "m_iHitDice": "m_hit_dice",
    "m_iDefenseRatio": "m_defense_ratio",
    "m_iHitRatio": "m_hit_ratio",
    "m_iMagicHitRatio": "m_magic_hit_ratio",
    "m_iMinBravery": "m_min_bravery",
    "m_iExpDiceMin": "m_exp_dice_min",
    "m_iExpDiceMax": "m_exp_dice_max",
    "m_iGoldDiceMin": "m_gold_dice_min",
    "m_iGoldDiceMax": "m_gold_dice_max",
    "m_iDropTableId": "m_drop_table_id",
    "m_cSide": "m_side",
    "m_cActionLimit": "m_action_limit",
    "m_cSize": "m_size",
    "m_cAttackDiceThrow": "m_attack_dice_throw",
    "m_cAttackDiceRange": "m_attack_dice_range",
    "m_cAttackBonus": "m_attack_bonus",
    "m_cBravery": "m_bravery",
    "m_cResistMagic": "m_resist_magic",
    "m_cMagicLevel": "m_magic_level",
    "m_cDayOfWeekLimit": "m_day_of_week_limit",
    "m_cChatMsgPresence": "m_chat_msg_presence",
    "m_iMana": "m_mana",
    "m_iMaxMana": "m_max_mana",
    "m_cMoveType": "m_move_type",
    "m_cBehavior": "m_behavior",
    "m_sBehaviorTurnCount": "m_behavior_turn_count",
    "m_cTargetSearchRange": "m_target_search_range",
    "m_iFollowOwnerIndex": "m_follow_owner_index",
    "m_cFollowOwnerType": "m_follow_owner_type",
    "m_bIsSummoned": "m_is_summoned",
    "m_bBypassMobLimit": "m_bypass_mob_limit",
    "m_dwSummonedTime": "m_summoned_time",
    "m_iTargetIndex": "m_target_index",
    "m_cTargetType": "m_target_type",
    "m_cCurWaypoint": "m_cur_waypoint",
    "m_cTotalWaypoint": "m_total_waypoint",
    "m_iSpotMobIndex": "m_spot_mob_index",
    "m_iWayPointIndex": "m_waypoint_index",
    "m_cMagicEffectStatus": "m_magic_effect_status",
    "m_bIsPermAttackMode": "m_is_perm_attack_mode",
    "m_iNoDieRemainExp": "m_no_die_remain_exp",
    "m_iAttackStrategy": "m_attack_strategy",
    "m_iAILevel": "m_ai_level",
    "m_iAttackRange": "m_attack_range",
    "m_iAttackCount": "m_attack_count",
    "m_bIsKilled": "m_is_killed",
    "m_bIsUnsummoned": "m_is_unsummoned",
    "m_iLastDamage": "m_last_damage",
    "m_iSummonControlMode": "m_summon_control_mode",
    "m_cAttribute": "m_attribute",
    "m_iAbsDamage": "m_abs_damage",
    "m_iItemRatio": "m_item_ratio",
    "m_iAssignedItem": "m_assigned_item",
    "m_cSpecialAbility": "m_special_ability",
    "m_iBuildCount": "m_build_count",
    "m_iManaStock": "m_mana_stock",
    "m_bIsMaster": "m_is_master",
    "m_iGuildGUID": "m_guild_guid",
    "m_cCropType": "m_crop_type",
    "m_cCropSkill": "m_crop_skill",
    "m_iV1": "m_v1",
    "m_cArea": "m_area",
    "m_iNpcItemType": "m_npc_item_type",
    "m_iNpcItemMax": "m_npc_item_max",

    # ========================================================================
    # CTile (Tile.h) — 17 renames
    # ========================================================================
    "m_cOwnerClass": "m_owner_class",
    "m_sOwner": "m_owner",
    "m_cDeadOwnerClass": "m_dead_owner_class",
    "m_sDeadOwner": "m_dead_owner",
    "m_pItem": "m_item",
    "m_cTotalItem": "m_total_item",
    "m_wDynamicObjectID": "m_dynamic_object_id",
    "m_sDynamicObjectType": "m_dynamic_object_type",
    "m_dwDynamicObjectRegisterTime": "m_dynamic_object_register_time",
    "m_bIsMoveAllowed": "m_is_move_allowed",
    "m_bIsTeleport": "m_is_teleport",
    "m_bIsWater": "m_is_water",
    "m_bIsFarm": "m_is_farm",
    "m_bIsTempMoveAllowed": "m_is_temp_move_allowed",
    "m_iOccupyStatus": "m_occupy_status",
    "m_iOccupyFlagIndex": "m_occupy_flag_index",
    "m_iAttribute": "m_attribute",

    # ========================================================================
    # CMap (Map.h) — 79 direct renames
    # ========================================================================
    "m_pTile": "m_tile",
    "m_pGame": "m_game",
    "m_cName": "m_name",
    "m_cLocationName": "m_location_name",
    "m_sSizeX": "m_size_x",
    "m_sSizeY": "m_size_y",
    "m_sTileDataSize": "m_tile_data_size",
    "m_pTeleportLoc": "m_teleport_loc",
    "m_pInitialPoint": "m_initial_point",
    "m_bNamingValueUsingStatus": "m_naming_value_using_status",
    "m_bRandomMobGenerator": "m_random_mob_generator",
    "m_cRandomMobGeneratorLevel": "m_random_mob_generator_level",
    "m_iTotalActiveObject": "m_total_active_object",
    "m_iTotalAliveObject": "m_total_alive_object",
    "m_iMaximumObject": "m_maximum_object",
    "m_cType": "m_type",
    "m_bIsFixedDayMode": "m_is_fixed_day_mode",
    "m_stSpotMobGenerator": "m_spot_mob_generator",
    "m_rcMobGenAvoidRect": "m_mob_generator_avoid_rect",
    "m_rcNoAttackRect": "m_no_attack_rect",
    "m_iTotalFishPoint": "m_total_fish_point",
    "m_iMaxFish": "m_max_fish",
    "m_iCurFish": "m_cur_fish",
    "m_iApocalypseMobGenType": "m_apocalypse_mob_gen_type",
    "m_iApocalypseBossMobNpcID": "m_apocalypse_boss_mob_npc_id",
    "m_rcApocalypseBossMob": "m_apocalypse_boss_mob",
    "m_cDynamicGateType": "m_dynamic_gate_type",
    "m_rcDynamicGateCoord": "m_dynamic_gate_coord",
    "m_cDynamicGateCoordDestMap": "m_dynamic_gate_coord_dest_map",
    "m_sDynamicGateCoordTgtX": "m_dynamic_gate_coord_tgt_x",
    "m_sDynamicGateCoordTgtY": "m_dynamic_gate_coord_tgt_y",
    "m_bIsCitizenLimit": "m_is_citizen_limit",
    "m_sHeldenianTowerType": "m_heldenian_tower_type",
    "m_sHeldenianTowerXPos": "m_heldenian_tower_x_pos",
    "m_sHeldenianTowerYPos": "m_heldenian_tower_y_pos",
    "m_cHeldenianTowerSide": "m_heldenian_tower_side",
    "m_cHeldenianModeMap": "m_heldenian_mode_map",
    "m_bMineralGenerator": "m_mineral_generator",
    "m_cMineralGeneratorLevel": "m_mineral_generator_level",
    "m_iTotalMineralPoint": "m_total_mineral_point",
    "m_iMaxMineral": "m_max_mineral",
    "m_iCurMineral": "m_cur_mineral",
    "m_cWhetherStatus": "m_weather_status",          # Fix original "whether" typo → "weather"
    "m_dwWhetherLastTime": "m_weather_duration",      # Fix original "whether" typo; it's a duration in ms, not a timestamp
    "m_dwWhetherStartTime": "m_weather_start_time",  # Fix original "whether" typo → "weather"
    "m_iLevelLimit": "m_level_limit",
    "m_iUpperLevelLimit": "m_upper_level_limit",
    "m_pOccupyFlag": "m_occupy_flag",
    "m_iTotalOccupyFlags": "m_total_occupy_flags",
    "m_pStrategicPointList": "m_strategic_point_list",
    "m_bIsAttackEnabled": "m_is_attack_enabled",
    "m_bIsFightZone": "m_is_fight_zone",
    "m_stEnergySphereCreationList": "m_energy_sphere_creation_list",
    "m_iTotalEnergySphereCreationPoint": "m_total_energy_sphere_creation_point",
    "m_stEnergySphereGoalList": "m_energy_sphere_goal_list",
    "m_iTotalEnergySphereGoalPoint": "m_total_energy_sphere_goal_point",
    "m_bIsEnergySphereGoalEnabled": "m_is_energy_sphere_goal_enabled",
    "m_iCurEnergySphereGoalPointIndex": "m_cur_energy_sphere_goal_point_index",
    "m_stDynamicGateCoords": "m_dynamic_gate_coords",
    "m_stSectorInfo": "m_sector_info",
    "m_stTempSectorInfo": "m_temp_sector_info",
    "m_iTotalItemEvents": "m_total_item_events",
    "m_stItemEventList": "m_item_event_list",
    "m_stHeldenianGateDoor": "m_heldenian_gate_door",
    "m_stHeldenianTower": "m_heldenian_tower",
    "m_iMaxNx": "m_top_neutral_sector_x",
    "m_iMaxNy": "m_top_neutral_sector_y",
    "m_iMaxAx": "m_top_aresden_sector_x",
    "m_iMaxAy": "m_top_aresden_sector_y",
    "m_iMaxEx": "m_top_elvine_sector_x",
    "m_iMaxEy": "m_top_elvine_sector_y",
    "m_iMaxMx": "m_top_monster_sector_x",
    "m_iMaxMy": "m_top_monster_sector_y",
    "m_iMaxPx": "m_top_player_sector_x",
    "m_iMaxPy": "m_top_player_sector_y",
    "m_stStrikePoint": "m_strike_point",
    "m_iTotalStrikePoints": "m_total_strike_points",
    "m_bIsDisabled": "m_is_disabled",
    "m_iTotalAgriculture": "m_total_agriculture",
    "m_stCrusadeStructureInfo": "m_crusade_structure_info",
    "m_iTotalCrusadeStructures": "m_total_crusade_structures",
    "m_bIsEnergySphereAutoCreation": "m_is_energy_sphere_auto_creation",
    "m_bIsSnowEnabled": "m_is_snow_enabled",
    "m_bIsRecallImpossible": "m_is_recall_impossible",
    "m_bIsApocalypseMap": "m_is_apocalypse_map",
    "m_bIsHeldenianMap": "m_is_heldenian_map",

    # ========================================================================
    # CItem (Item.h) — 38 renames
    # ========================================================================
    "m_sIDnum": "m_id_num",
    "m_cItemType": "m_item_type",
    "m_cEquipPos": "m_equip_pos",
    "m_sItemEffectType": "m_item_effect_type",
    "m_sItemEffectValue1": "m_item_effect_value1",
    "m_sItemEffectValue2": "m_item_effect_value2",
    "m_sItemEffectValue3": "m_item_effect_value3",
    "m_sItemEffectValue4": "m_item_effect_value4",
    "m_sItemEffectValue5": "m_item_effect_value5",
    "m_sItemEffectValue6": "m_item_effect_value6",
    "m_sSpecialEffect": "m_special_effect",
    "m_sSpecialEffectValue1": "m_special_effect_value1",
    "m_sSpecialEffectValue2": "m_special_effect_value2",
    "m_sItemSpecEffectValue1": "m_item_special_effect_value1",
    "m_sItemSpecEffectValue2": "m_item_special_effect_value2",
    "m_sItemSpecEffectValue3": "m_item_special_effect_value3",
    "m_sTouchEffectType": "m_touch_effect_type",
    "m_sTouchEffectValue1": "m_touch_effect_value1",
    "m_sTouchEffectValue2": "m_touch_effect_value2",
    "m_sTouchEffectValue3": "m_touch_effect_value3",
    "m_sSprite": "m_sprite",
    "m_sSpriteFrame": "m_sprite_frame",
    "m_cApprValue": "m_appearance_value",
    "m_cItemColor": "m_item_color",
    "m_cSpeed": "m_speed",
    "m_wPrice": "m_price",
    "m_wWeight": "m_weight",
    "m_sLevelLimit": "m_level_limit",
    "m_cGenderLimit": "m_gender_limit",
    "m_sRelatedSkill": "m_related_skill",
    "m_wMaxLifeSpan": "m_max_life_span",
    "m_wCurLifeSpan": "m_cur_life_span",
    "m_cCategory": "m_category",
    "m_bIsForSale": "m_is_for_sale",
    "m_dwCount": "m_count",
    "m_dwAttribute": "m_attribute",

    # ========================================================================
    # Small server classes
    # ========================================================================
    # CBuildItem
    "m_sItemID": "m_item_id",
    "m_iSkillLimit": "m_skill_limit",
    "m_iMaterialItemID": "m_material_item_id",
    "m_iMaterialItemCount": "m_material_item_count",
    "m_iMaterialItemValue": "m_material_item_value",
    "m_iIndex": "m_index",
    "m_iMaxValue": "m_max_value",
    "m_iAverageValue": "m_average_value",
    "m_iMaxSkill": "m_max_skill",
    "m_wAttribute": "m_attribute",

    # CFish
    "m_sDynamicObjectHandle": "m_dynamic_object_handle",
    "m_sEngagingCount": "m_engaging_count",
    "m_iDifficulty": "m_difficulty",

    # CMineral
    "m_iRemain": "m_remain",

    # CPortion
    "m_sArray": "m_array",

    # CTeleportLoc
    "m_sSrcX": "m_src_x",
    "m_sSrcY": "m_src_y",
    "m_cDestMapName": "m_dest_map_name",
    "m_cDestMapName2": "m_dest_map_name2",
    "m_sDestX": "m_dest_x",
    "m_sDestY": "m_dest_y",
    "m_sDestX2": "m_dest_x2",
    "m_sDestY2": "m_dest_y2",
    "m_iV2": "m_v2",
    "m_dwTime2": "m_time2",

    # CNpcItem
    "m_sFirstProbability": "m_first_probability",
    "m_cFirstTargetValue": "m_first_target_value",
    "m_sSecondProbability": "m_second_probability",
    "m_cSecondTargetValue": "m_second_target_value",

    # COccupyFlag
    "m_iEKCount": "m_enemy_kill_count",
    "m_iDynamicObjectIndex": "m_dynamic_object_index",

    # CStrategicPoint
    "m_iSide": "m_side",
    "m_iValue": "m_value",
    "m_iX": "m_x",
    "m_iY": "m_y",

    # CGuildsMan
    "m_iRank": "m_rank",

    # CDelayEvent
    "m_iDelayType": "m_delay_type",
    "m_iEffectType": "m_effect_type",
    "m_iTargetH": "m_target_handle",
    "m_iV3": "m_v3",
    "m_dwTriggerTime": "m_trigger_time",

    # CDynamicObject
    "m_cOwnerType": "m_owner_type",
    "m_dwRegisterTime": "m_register_time",
    "m_dwLastTime": "m_last_time",
    "m_iCount": "m_count",

    # ========================================================================
    # CMagic (Magic.h) — 20 renames
    # ========================================================================
    "m_dwDelayTime": "m_delay_time",
    "m_sValue1": "m_value_1",
    "m_sValue2": "m_value_2",
    "m_sValue3": "m_value_3",
    "m_sValue4": "m_value_4",
    "m_sValue5": "m_value_5",
    "m_sValue6": "m_value_6",
    "m_sValue7": "m_value_7",
    "m_sValue8": "m_value_8",
    "m_sValue9": "m_value_9",
    "m_sValue10": "m_value_10",
    "m_sValue11": "m_value_11",
    "m_sValue12": "m_value_12",
    "m_sIntLimit": "m_intelligence_limit",
    "m_iGoldCost": "m_gold_cost",

    # CSkill
    "m_bIsUseable": "m_is_useable",
    "m_cUseMethod": "m_use_method",

    # ========================================================================
    # CQuest (Quest.h) — 22 renames
    # ========================================================================
    "m_iType": "m_type",
    "m_iTargetType": "m_target_type",
    "m_iMaxCount": "m_max_count",
    "m_iFrom": "m_from",
    "m_iMinLevel": "m_min_level",
    "m_iMaxLevel": "m_max_level",
    "m_iRequiredSkillNum": "m_required_skill_num",
    "m_iRequiredSkillLevel": "m_required_skill_level",
    "m_iTimeLimit": "m_time_limit",
    "m_iAssignType": "m_assign_type",
    "m_iRewardType": "m_reward_type",
    "m_iRewardAmount": "m_reward_amount",
    "m_iContribution": "m_contribution",
    "m_iContributionLimit": "m_contribution_limit",
    "m_iResponseMode": "m_response_mode",
    "m_cTargetName": "m_target_name",
    "m_iRange": "m_range",
    "m_iQuestID": "m_quest_id",
    "m_iReqContribution": "m_req_contribution",

    # ========================================================================
    # CGame (Game.h) — 153+ renames
    # ========================================================================
    "m_cRealmName": "m_realm_name",
    "m_cLoginListenIP": "m_login_listen_ip",
    "m_iLoginListenPort": "m_login_listen_port",
    "m_cGameListenIP": "m_game_listen_ip",
    "m_iGameListenPort": "m_game_listen_port",
    "m_cGameConnectionIP": "m_game_connection_ip",
    "m_iGameConnectionPort": "m_game_connection_port",
    "m_iLevelExp20": "m_level_exp_20",
    "m_pClientList": "m_client_list",
    "m_pNpcList": "m_npc_list",
    "m_pMapList": "m_map_list",
    "m_pTempNpcItem": "m_temp_npc_item",
    "m_pEntityManager": "m_entity_manager",
    "m_pFishingManager": "m_fishing_manager",
    "m_pMiningManager": "m_mining_manager",
    "m_pCraftingManager": "m_crafting_manager",
    "m_pQuestManager": "m_quest_manager",
    "m_pGuildManager": "m_guild_manager",
    "m_pDelayEventManager": "m_delay_event_manager",
    "m_pDynamicObjectManager": "m_dynamic_object_manager",
    "m_pLootManager": "m_loot_manager",
    "m_pCombatManager": "m_combat_manager",
    "m_pItemManager": "m_item_manager",
    "m_pMagicManager": "m_magic_manager",
    "m_pSkillManager": "m_skill_manager",
    "m_pWarManager": "m_war_manager",
    "m_pStatusEffectManager": "m_status_effect_manager",
    "m_iTotalMaps": "m_total_maps",
    "m_bIsGameStarted": "m_is_game_started",
    "m_bIsItemAvailable": "m_is_item_available",
    "m_bIsBuildItemAvailable": "m_is_build_item_available",
    "m_bIsNpcAvailable": "m_is_npc_available",
    "m_bIsMagicAvailable": "m_is_magic_available",
    "m_bIsSkillAvailable": "m_is_skill_available",
    "m_bIsPortionAvailable": "m_is_portion_available",
    "m_bIsQuestAvailable": "m_is_quest_available",
    "m_bIsTeleportAvailable": "m_is_teleport_available",
    "m_bIsDropTableAvailable": "m_is_drop_table_available",
    "m_bIsShopDataAvailable": "m_is_shop_data_available",
    "m_NpcShopMappings": "m_npc_shop_mappings",
    "m_ShopData": "m_shop_data",
    "m_DropTables": "m_drop_tables",
    "m_pItemConfigList": "m_item_config_list",
    "m_pNpcConfigList": "m_npc_config_list",
    "m_pMagicConfigList": "m_magic_config_list",
    "m_pSkillConfigList": "m_skill_config_list",
    "m_dwConfigHash": "m_config_hash",
    "m_pPartyManager": "m_party_manager",
    "m_pMsgBuffer": "m_msg_buffer",
    "m_iTotalClients": "m_total_clients",
    "m_iMaxClients": "m_max_clients",
    "m_iTotalGameServerClients": "m_total_game_server_clients",
    "m_iTotalGameServerMaxClients": "m_total_game_server_max_clients",
    "m_iTotalBots": "m_total_bots",
    "m_iMaxBots": "m_max_bots",
    "m_iTotalGameServerBots": "m_total_game_server_bots",
    "m_iTotalGameServerMaxBots": "m_total_game_server_max_bots",
    "m_MaxUserSysTime": "m_max_user_sys_time",
    "m_bOnExitProcess": "m_on_exit_process",
    "m_dwExitProcessTime": "m_exit_process_time",
    "m_dwWhetherTime": "m_weather_time",              # Fix original "whether" typo → "weather"
    "m_dwGameTime1": "m_game_time_1",
    "m_dwGameTime2": "m_game_time_2",
    "m_dwGameTime3": "m_game_time_3",
    "m_dwGameTime4": "m_game_time_4",
    "m_dwGameTime5": "m_game_time_5",
    "m_dwGameTime6": "m_game_time_6",
    "m_bIsCrusadeWarStarter": "m_is_crusade_war_starter",
    "m_bIsApocalypseStarter": "m_is_apocalypse_starter",
    "m_iLatestCrusadeDayOfWeek": "m_latest_crusade_day_of_week",
    "m_cDayOrNight": "m_day_or_night",
    "m_iSkillSSNpoint": "m_skill_progress_threshold",
    "m_pNoticeMsgList": "m_notice_msg_list",
    "m_iTotalNoticeMsg": "m_total_notice_msg",
    "m_iPrevSendNoticeMsg": "m_prev_send_notice_msg",
    "m_dwNoticeTime": "m_notice_time",
    "m_dwSpecialEventTime": "m_special_event_time",
    "m_bIsSpecialEventTime": "m_is_special_event_time",
    "m_cSpecialEventType": "m_special_event_type",
    "m_iLevelExpTable": "m_level_exp_table",
    "m_bIsServerShutdowned": "m_is_server_shutdown",
    "m_cShutDownCode": "m_shutdown_code",
    "m_iMiddlelandMapIndex": "m_middleland_map_index",
    "m_iAresdenMapIndex": "m_aresden_map_index",
    "m_iElvineMapIndex": "m_elvine_map_index",
    "m_iBTFieldMapIndex": "m_bt_field_map_index",
    "m_iGodHMapIndex": "m_godh_map_index",
    "m_iAresdenOccupyTiles": "m_aresden_occupy_tiles",
    "m_iElvineOccupyTiles": "m_elvine_occupy_tiles",
    "m_iCurMsgs": "m_cur_msgs",
    "m_iMaxMsgs": "m_max_msgs",
    "m_dwCanFightzoneReserveTime": "m_can_fightzone_reserve_time",
    "m_iFightZoneReserve": "m_fight_zone_reserve",
    "m_iFightzoneNoForceRecall": "m_fightzone_no_force_recall",
    "m_stCityStatus": "m_city_status",
    "m_iStrategicStatus": "m_strategic_status",
    "m_iAutoRebootingCount": "m_auto_rebooting_count",
    "m_pBuildItemList": "m_build_item_list",
    "m_pNoticementData": "m_notice_data",
    "m_dwNoticementDataSize": "m_notice_data_size",
    "m_dwMapSectorInfoTime": "m_map_sector_info_time",
    "m_iMapSectorInfoUpdateCount": "m_map_sector_info_update_count",
    "m_iCrusadeCount": "m_crusade_count",
    "m_bIsCrusadeMode": "m_is_crusade_mode",
    "m_bIsApocalypseMode": "m_is_apocalypse_mode",
    "m_stCrusadeStructures": "m_crusade_structures",
    "m_iCollectedMana": "m_collected_mana",
    "m_iAresdenMana": "m_aresden_mana",
    "m_iElvineMana": "m_elvine_mana",
    "m_pGuildTeleportLoc": "m_guild_teleport_loc",
    "m_iLastCrusadeWinner": "m_last_crusade_winner",
    "m_stMeteorStrikeResult": "m_meteor_strike_result",
    "m_stMiddleCrusadeStructureInfo": "m_middle_crusade_structure_info",
    "m_stBannedList": "m_banned_list",
    "m_stAdminList": "m_admin_list",
    "m_iAdminCount": "m_admin_count",
    "m_stCrusadeWarSchedule": "m_crusade_war_schedule",
    "m_stApocalypseScheduleStart": "m_apocalypse_schedule_start",
    "m_stApocalypseScheduleEnd": "m_apocalypse_schedule_end",
    "m_stHeldenianSchedule": "m_heldenian_schedule",
    "m_iTotalMiddleCrusadeStructures": "m_total_middle_crusade_structures",
    "m_iClientShortCut": "m_client_shortcut",
    "m_iNpcConstructionPoint": "m_npc_construction_point",
    "m_dwCrusadeGUID": "m_crusade_guid",
    "m_sLastCrusadeDate": "m_last_crusade_date",
    "m_iCrusadeWinnerSide": "m_crusade_winner_side",
    "m_stPartyInfo": "m_party_info",
    "m_sSlateSuccessRate": "m_slate_success_rate",
    "m_sForceRecallTime": "m_force_recall_time",
    "m_fPrimaryDropRate": "m_primary_drop_rate",
    "m_fGoldDropRate": "m_gold_drop_rate",
    "m_fSecondaryDropRate": "m_secondary_drop_rate",
    "m_iFinalShutdownCount": "m_final_shutdown_count",
    "m_bEnemyKillMode": "m_enemy_kill_mode",
    "m_iEnemyKillAdjust": "m_enemy_kill_adjust",
    "m_sRaidTimeMonday": "m_raid_time_monday",
    "m_sRaidTimeTuesday": "m_raid_time_tuesday",
    "m_sRaidTimeWednesday": "m_raid_time_wednesday",
    "m_sRaidTimeThursday": "m_raid_time_thursday",
    "m_sRaidTimeFriday": "m_raid_time_friday",
    "m_sRaidTimeSaturday": "m_raid_time_saturday",
    "m_sRaidTimeSunday": "m_raid_time_sunday",
    "m_bManualTime": "m_manual_time",
    "m_bIsApocalyseMode": "m_is_apocalypse_mode_legacy",  # Typo duplicate of m_bIsApocalypseMode; kept separate to avoid merge risk
    "m_bIsHeldenianMode": "m_is_heldenian_mode",
    "m_bIsHeldenianTeleport": "m_is_heldenian_teleport",
    "m_cHeldenianType": "m_heldenian_type",
    "m_dwApocalypseGUID": "m_apocalypse_guid",
    "m_sCharPointLimit": "m_char_point_limit",
    "m_bAllow100AllSkill": "m_allow_100_all_skill",
    "m_cRepDropModifier": "m_rep_drop_modifier",
    "m_iClientTimeout": "m_client_timeout",
    "m_iStaminaRegenInterval": "m_stamina_regen_interval",
    "m_iPoisonDamageInterval": "m_poison_damage_interval",
    "m_iHealthRegenInterval": "m_health_regen_interval",
    "m_iManaRegenInterval": "m_mana_regen_interval",
    "m_iHungerConsumeInterval": "m_hunger_consume_interval",
    "m_iSummonCreatureDuration": "m_summon_creature_duration",
    "m_iAutosaveInterval": "m_autosave_interval",
    "m_iLagProtectionInterval": "m_lag_protection_interval",
    "m_iBaseStatValue": "m_base_stat_value",
    "m_iCreationStatBonus": "m_creation_stat_bonus",
    "m_iLevelupStatGain": "m_levelup_stat_gain",
    "m_iMaxLevel": "m_max_level",
    "m_iMaxStatValue": "m_max_stat_value",
    "m_iMinimumHitRatio": "m_minimum_hit_ratio",
    "m_iMaximumHitRatio": "m_maximum_hit_ratio",
    "m_iNighttimeDuration": "m_nighttime_duration",
    "m_iStartingGuildRank": "m_starting_guild_rank",
    "m_iGrandMagicManaConsumption": "m_grand_magic_mana_consumption",
    "m_iMaxConstructionPoints": "m_max_construction_points",
    "m_iMaxSummonPoints": "m_max_summon_points",
    "m_iMaxWarContribution": "m_max_war_contribution",
    "m_iMaxBankItems": "m_max_bank_items",
    "m_cHeldenianVictoryType": "m_heldenian_victory_type",
    "m_sLastHeldenianWinner": "m_last_heldenian_winner",
    "m_cHeldenianModeType": "m_heldenian_mode_type",
    "m_iHeldenianAresdenDead": "m_heldenian_aresden_dead",
    "m_iHeldenianElvineDead": "m_heldenian_elvine_dead",
    "m_iHeldenianAresdenLeftTower": "m_heldenian_aresden_left_tower",
    "m_iHeldenianElvineLeftTower": "m_heldenian_elvine_left_tower",
    "m_dwHeldenianGUID": "m_heldenian_guid",
    "m_dwHeldenianStartHour": "m_heldenian_start_hour",
    "m_dwHeldenianStartMinute": "m_heldenian_start_minute",
    "m_dwHeldenianStartTime": "m_heldenian_start_time",
    "m_dwHeldenianFinishTime": "m_heldenian_finish_time",
    "m_bReceivedItemList": "m_received_item_list",
    "m_bHeldenianInitiated": "m_heldenian_initiated",
    "m_bHeldenianRunning": "m_heldenian_running",

    # ========================================================================
    # CClient (Client.h) — ~160 renames
    # ========================================================================
    "m_iAngelicStr": "m_angelic_str",
    "m_iAngelicInt": "m_angelic_int",
    "m_iAngelicDex": "m_angelic_dex",
    "m_iAngelicMag": "m_angelic_mag",
    "m_cVar": "m_var",
    "m_iRecentWalkTime": "m_recent_walk_time",
    "m_iRecentRunTime": "m_recent_run_time",
    "m_sV1": "m_v1",
    "m_cHeroArmourBonus": "m_hero_armour_bonus",
    "m_dwMagicFreqTime": "m_magic_freq_time",
    "m_dwMoveFreqTime": "m_move_freq_time",
    "m_dwAttackFreqTime": "m_attack_freq_time",
    "m_bIsMoveBlocked": "m_is_move_blocked",
    "m_bMagicItem": "m_magic_item",
    "m_bMagicConfirm": "m_magic_confirm",
    "m_iSpellCount": "m_spell_count",
    "m_bMagicPauseTime": "m_magic_pause_time",
    "m_bIsClientConnected": "m_is_client_connected",
    "m_dwLastMsgId": "m_last_msg_id",
    "m_dwLastMsgTime": "m_last_msg_time",
    "m_dwLastMsgSize": "m_last_msg_size",
    "m_dwLastFullObjectId": "m_last_full_object_id",
    "m_dwLastFullObjectTime": "m_last_full_object_time",
    "m_cCharName": "m_char_name",
    "m_cAccountName": "m_account_name",
    "m_cAccountPassword": "m_account_password",
    "m_bIsInitComplete": "m_is_init_complete",
    "m_bIsMsgSendAvailable": "m_is_msg_send_available",
    "m_cMapName": "m_map_name",
    "m_cGuildName": "m_guild_name",
    "m_cLocation": "m_location",
    "m_iGuildRank": "m_guild_rank",
    "m_dwHPTime": "m_hp_time",
    "m_dwMPTime": "m_mp_time",
    "m_dwSPTime": "m_sp_time",
    "m_dwAutoSaveTime": "m_auto_save_time",
    "m_dwHungerTime": "m_hunger_time",
    "m_dwWarmEffectTime": "m_warm_effect_time",
    "m_dwAfkActivityTime": "m_afk_activity_time",
    "m_cSex": "m_sex",
    "m_cSkin": "m_skin",
    "m_cHairStyle": "m_hair_style",
    "m_cHairColor": "m_hair_color",
    "m_cUnderwear": "m_underwear",
    "m_iHPstock": "m_hp_stock",
    "m_iMP": "m_mp",
    "m_iSP": "m_sp",
    "m_iNextLevelExp": "m_next_level_exp",
    "m_iDamageAbsorption_Armor": "m_damage_absorption_armor",
    "m_iDamageAbsorption_Shield": "m_damage_absorption_shield",
    "m_iLevel": "m_level",
    "m_iStr": "m_str",
    "m_iInt": "m_int",
    "m_iVit": "m_vit",
    "m_iDex": "m_dex",
    "m_iMag": "m_mag",
    "m_iCharisma": "m_charisma",
    "m_iLuck": "m_luck",
    "m_iLU_Pool": "m_levelup_pool",
    "m_cAura": "m_aura",
    "m_iGizonItemUpgradeLeft": "m_gizon_item_upgrade_left",
    "m_iAddTransMana": "m_add_trans_mana",
    "m_iAddChargeCritical": "m_add_charge_critical",
    "m_iRewardGold": "m_reward_gold",
    "m_iEnemyKillCount": "m_enemy_kill_count",
    "m_iPKCount": "m_player_kill_count",
    "m_iCurWeightLoad": "m_cur_weight_load",
    "m_bInhibition": "m_inhibition",
    "m_cAttackDiceThrow_SM": "m_attack_dice_throw_sm",
    "m_cAttackDiceRange_SM": "m_attack_dice_range_sm",
    "m_cAttackDiceThrow_L": "m_attack_dice_throw_l",
    "m_cAttackDiceRange_L": "m_attack_dice_range_l",
    "m_cAttackBonus_SM": "m_attack_bonus_sm",
    "m_cAttackBonus_L": "m_attack_bonus_l",
    "m_pItemList": "m_item_list",
    "m_pItemInBankList": "m_item_in_bank_list",
    "m_bIsItemEquipped": "m_is_item_equipped",
    "m_sItemEquipmentStatus": "m_item_equipment_status",
    "m_cArrowIndex": "m_arrow_index",
    "m_cMagicMastery": "m_magic_mastery",
    "m_cSkillMastery": "m_skill_mastery",
    "m_iSkillSSN": "m_skill_progress",
    "m_bSkillUsingStatus": "m_skill_using_status",
    "m_iSkillUsingTimeID": "m_skill_using_time_id",
    "m_iWhisperPlayerIndex": "m_whisper_player_index",
    "m_cProfile": "m_profile",
    "m_iHungerStatus": "m_hunger_status",
    "m_dwWarBeginTime": "m_war_begin_time",
    "m_bIsWarLocation": "m_is_war_location",
    "m_bIsPoisoned": "m_is_poisoned",
    "m_iPoisonLevel": "m_poison_level",
    "m_dwPoisonTime": "m_poison_time",
    "m_iPenaltyBlockYear": "m_penalty_block_year",
    "m_iPenaltyBlockMonth": "m_penalty_block_month",
    "m_iPenaltyBlockDay": "m_penalty_block_day",
    "m_iFightzoneNumber": "m_fightzone_number",
    "m_iReserveTime": "m_reserve_time",
    "m_iFightZoneTicketNumber": "m_fightzone_ticket_number",
    "m_pXSock": "m_socket",
    "m_iRating": "m_rating",
    "m_iTimeLeft_Rating": "m_time_left_rating",
    "m_iTimeLeft_ForceRecall": "m_time_left_force_recall",
    "m_iTimeLeft_FirmStaminar": "m_time_left_firm_stamina",
    "m_iForceStart": "m_force_start",
    "m_bIsOnServerChange": "m_is_on_server_change",
    "m_iExpStock": "m_exp_stock",
    "m_dwExpStockTime": "m_exp_stock_time",
    "m_iAutoExpAmount": "m_auto_exp_amount",
    "m_dwAutoExpTime": "m_auto_exp_time",
    "m_dwRecentAttackTime": "m_recent_attack_time",
    "m_iAllocatedFish": "m_allocated_fish",
    "m_iFishChance": "m_fish_chance",
    "m_cIPaddress": "m_ip_address",
    "m_bIsSafeAttackMode": "m_is_safe_attack_mode",
    "m_bIsOnWaitingProcess": "m_is_on_waiting_process",
    "m_iSuperAttackLeft": "m_super_attack_left",
    "m_iSuperAttackCount": "m_super_attack_count",
    "m_sUsingWeaponSkill": "m_using_weapon_skill",
    "m_iManaSaveRatio": "m_mana_save_ratio",
    "m_bIsLuckyEffect": "m_is_lucky_effect",
    "m_iSideEffect_MaxHPdown": "m_side_effect_max_hp_down",
    "m_iComboAttackCount": "m_combo_attack_count",
    "m_iDownSkillIndex": "m_down_skill_index",
    "m_iMagicDamageSaveItemIndex": "m_magic_damage_save_item_index",
    "m_sCharIDnum1": "m_char_id_num1",
    "m_sCharIDnum2": "m_char_id_num2",
    "m_sCharIDnum3": "m_char_id_num3",
    "m_iAbuseCount": "m_abuse_count",
    "m_bIsExchangeMode": "m_is_exchange_mode",
    "m_iExchangeH": "m_exchange_h",
    "m_cExchangeName": "m_exchange_name",
    "m_sExchangeItemID": "m_exchange_item_id",
    "m_cExchangeItemIndex": "m_exchange_item_index",
    "m_iExchangeItemAmount": "m_exchange_item_amount",
    "m_bIsExchangeConfirm": "m_is_exchange_confirm",
    "m_iQuest": "m_quest",
    "m_iAskedQuest": "m_asked_quest",
    "m_iCurQuestCount": "m_cur_quest_count",
    "m_iQuestRewardType": "m_quest_reward_type",
    "m_iQuestRewardAmount": "m_quest_reward_amount",
    "m_bQuestMatchFlag_Loc": "m_quest_match_flag_loc",
    "m_bIsQuestCompleted": "m_is_quest_completed",
    "m_iCustomItemValue_Attack": "m_custom_item_value_attack",
    "m_iCustomItemValue_Defense": "m_custom_item_value_defense",
    "m_iMinAP_SM": "m_min_attack_power_sm",
    "m_iMinAP_L": "m_min_attack_power_l",
    "m_iMaxAP_SM": "m_max_attack_power_sm",
    "m_iMaxAP_L": "m_max_attack_power_l",
    "m_bIsNeutral": "m_is_neutral",
    "m_bIsObserverMode": "m_is_observer_mode",
    "m_iSpecialEventID": "m_special_event_id",
    "m_iSpecialWeaponEffectType": "m_special_weapon_effect_type",
    "m_iSpecialWeaponEffectValue": "m_special_weapon_effect_value",
    "m_iAddHP": "m_add_hp",
    "m_iAddSP": "m_add_sp",
    "m_iAddMP": "m_add_mp",
    "m_iAddAR": "m_add_attack_ratio",
    "m_iAddPR": "m_add_poison_resistance",
    "m_iAddDR": "m_add_defense_ratio",
    "m_iAddMR": "m_add_magic_resistance",
    "m_iAddAbsPD": "m_add_abs_physical_defense",
    "m_iAddAbsMD": "m_add_abs_magical_defense",
    "m_iAddCD": "m_add_combo_damage",
    "m_iAddExp": "m_add_exp",
    "m_iAddGold": "m_add_gold",
    "m_iAddResistMagic": "m_add_resist_magic",
    "m_iAddPhysicalDamage": "m_add_physical_damage",
    "m_iAddMagicalDamage": "m_add_magical_damage",
    "m_iAddAbsAir": "m_add_abs_air",
    "m_iAddAbsEarth": "m_add_abs_earth",
    "m_iAddAbsFire": "m_add_abs_fire",
    "m_iAddAbsWater": "m_add_abs_water",
    "m_iMoveMsgRecvCount": "m_move_msg_recv_count",
    "m_iAttackMsgRecvCount": "m_attack_msg_recv_count",
    "m_iRunMsgRecvCount": "m_run_msg_recv_count",
    "m_iSkillMsgRecvCount": "m_skill_msg_recv_count",
    "m_dwMoveLAT": "m_move_last_action_time",
    "m_dwRunLAT": "m_run_last_action_time",
    "m_dwAttackLAT": "m_attack_last_action_time",
    "m_iSpecialAbilityTime": "m_special_ability_time",
    "m_bIsSpecialAbilityEnabled": "m_is_special_ability_enabled",
    "m_dwSpecialAbilityStartTime": "m_special_ability_start_time",
    "m_iSpecialAbilityLastSec": "m_special_ability_last_sec",
    "m_iSpecialAbilityType": "m_special_ability_type",
    "m_iSpecialAbilityEquipPos": "m_special_ability_equip_pos",
    "m_iAlterItemDropIndex": "m_alter_item_drop_index",
    "m_iWarContribution": "m_war_contribution",
    "m_dwSpeedHackCheckTime": "m_speed_hack_check_time",
    "m_iSpeedHackCheckExp": "m_speed_hack_check_exp",
    "m_dwLogoutHackCheck": "m_logout_hack_check",
    "m_dwInitCCTimeRcv": "m_initial_check_time_received",
    "m_dwInitCCTime": "m_initial_check_time",
    "m_cLockedMapName": "m_locked_map_name",
    "m_iLockedMapTime": "m_locked_map_time",
    "m_iCrusadeDuty": "m_crusade_duty",
    "m_bInRecallImpossibleMap": "m_in_recall_impossible_map",
    "m_iCSIsendPoint": "m_crusade_info_send_point",
    "m_cSendingMapName": "m_sending_map_name",
    "m_bIsSendingMapStatus": "m_is_sending_map_status",
    "m_iConstructionPoint": "m_construction_point",
    "m_cConstructMapName": "m_construct_map_name",
    "m_iConstructLocX": "m_construct_loc_x",
    "m_iConstructLocY": "m_construct_loc_y",
    "m_bIsPlayerCivil": "m_is_player_civil",
    "m_bIsAttackModeChange": "m_is_attack_mode_change",
    "m_iPartyID": "m_party_id",
    "m_iPartyStatus": "m_party_status",
    "m_iReqJoinPartyClientH": "m_req_join_party_client_h",
    "m_cReqJoinPartyName": "m_req_join_party_name",
    "m_iPartyRank": "m_party_rank",
    "m_iPartyMemberCount": "m_party_member_count",
    "m_iPartyGUID": "m_party_guid",
    "m_stPartyMemberName": "m_party_member_name",
    "m_dwLastActionTime": "m_last_action_time",
    "m_iDeadPenaltyTime": "m_dead_penalty_time",
    "m_cWhisperPlayerName": "m_whisper_player_name",
    "m_bIsInsideWarehouse": "m_is_inside_warehouse",
    "m_bIsInsideWizardTower": "m_is_inside_wizard_tower",
    "m_bIsInsideOwnTown": "m_is_inside_own_town",
    "m_bIsCheckingWhisperPlayer": "m_is_checking_whisper_player",
    "m_bIsOwnLocation": "m_is_own_location",
    "m_pIsProcessingAllowed": "m_is_processing_allowed",
    "m_cHeroArmorBonus": "m_hero_armor_bonus",
    "m_bIsBeingResurrected": "m_is_being_resurrected",
    "m_dwFightzoneDeadTime": "m_fightzone_dead_time",
    "m_cSaveCount": "m_save_count",
    "m_dwLastConfigRequestTime": "m_last_config_request_time",
    "m_dwLastDamageTakenTime": "m_last_damage_taken_time",
    "m_bIsGMMode": "m_is_gm_mode",
    "m_bIsAdminInvisible": "m_is_admin_invisible",
    "m_dwLastGMImmuneNotifyTime": "m_last_gm_immune_notify_time",
    "m_iAdminIndex": "m_admin_index",
    "m_iAdminLevel": "m_admin_level",
    "m_bBlockListDirty": "m_block_list_dirty",
    # PascalCase members
    "m_ItemPosList": "m_item_pos_list",
    "m_BlockedAccounts": "m_blocked_accounts",
    "m_BlockedAccountsList": "m_blocked_accounts_list",

    # ========================================================================
    # Manager class members (non-m_pGame)
    # ========================================================================
    "m_pQuestConfigList": "m_quest_config_list",
    "m_pPortionConfigList": "m_portion_config_list",
    "m_pCraftingConfigList": "m_crafting_config_list",
    "m_dwFishTime": "m_fish_time",
    "m_pFish": "m_fish",
    "m_pMineral": "m_mineral",
    "m_pDelayEventList": "m_delay_event_list",
    "m_pDynamicObjectList": "m_dynamic_object_list",
    "m_iMemberNumList": "m_member_num_list",
    "m_stMemberNameList": "m_member_name_list",
    "m_dwCheckMemberActTime": "m_check_member_act_time",

    # ========================================================================
    # Missed in first pass — EntityManager.h (7 renames)
    # ========================================================================
    "m_dwEntityGUID": "m_entity_guid",
    "m_pActiveEntityList": "m_active_entity_list",
    "m_iActiveEntityCount": "m_active_entity_count",
    "m_iMaxMaps": "m_max_maps",
    "m_iTotalEntities": "m_total_entities",
    "m_dwNextGUID": "m_next_guid",
    "m_bInitialized": "m_initialized",

    # ========================================================================
    # Missed in first pass — ASIOSocket.h (13 renames)
    # ========================================================================
    "m_bIsAvailable": "m_is_available",
    "m_bIsWriteEnabled": "m_is_write_enabled",
    "m_dwBufferSize": "m_buffer_size",
    "m_cStatus": "m_status",
    "m_dwReadSize": "m_read_size",
    "m_dwTotalReadSize": "m_total_read_size",
    "m_pAddr": "m_addr",
    "m_iPortNum": "m_port_num",
    "m_iBlockLimit": "m_block_limit",
    "m_RecvQueue": "m_recv_queue",
    "m_iSocketIndex": "m_socket_index",
    "m_bAsyncWriteInProgress": "m_async_write_in_progress",
    "m_bAsyncMode": "m_async_mode",

    # ========================================================================
    # Missed in first pass — NetworkMsg.h / CMsg (4 renames)
    # ========================================================================
    "m_cFrom": "m_from",
    "m_pData": "m_data",
    "m_dwSize": "m_size",
    "m_cKey": "m_key",

    # ========================================================================
    # Missed in first pass — Client.h struct prefix (1 rename)
    # ========================================================================
    "m_stRepairAll": "m_repair_all",

    # ========================================================================
    # Missed in first pass — Map.h nested struct fields with m_ prefix (4)
    # ========================================================================
    "m_bIsGateMap": "is_gate_map",
    "m_cDynamicGateMap": "dynamic_gate_map",
    "m_iDynamicGateX": "dynamic_gate_x",
    "m_iDynamicGateY": "dynamic_gate_y",

    # ========================================================================
    # Missed in first pass — Game.h struct fields with m_ prefix (2)
    # ========================================================================
    "m_cBannedIPaddress": "banned_ip_address",
    "m_cApprovedIP": "approved_ip",

    # ========================================================================
    # Missed in first pass — PartyManager.h nested struct field (1)
    # ========================================================================
    "m_dwServerChangeTime": "server_change_time",
}

# ============================================================================
# File processing
# ============================================================================

SERVER_DIR = Path("Z:/Helbreath-3.82/Sources/Server")
SHARED_DIR = Path("Z:/Helbreath-3.82/Sources/Dependencies/Shared")

def get_source_files():
    """Get all .h and .cpp files in server and shared directories."""
    files = []
    for d in [SERVER_DIR, SHARED_DIR]:
        if d.exists():
            for ext in ("*.h", "*.cpp"):
                files.extend(d.rglob(ext))
    return sorted(files)


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
        stripped = line.lstrip()
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
    # e.g., m_sValue10 before m_sValue1
    renames_sorted = []
    for old, new in sorted(RENAMES.items(), key=lambda x: -len(x[0])):
        pattern = re.compile(r'\b' + re.escape(old) + r'\b')
        renames_sorted.append((old, new, pattern))

    print(f"{'DRY RUN — ' if dry_run else ''}Server snake_case conversion")
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

"""
Server Method Name snake_case Conversion Script
Converts PascalCase/camelCase method names to snake_case across all server and shared source files.
Companion to server_snake_case.py (which handled member variables).

Usage:
    python Scripts/server_method_snake_case.py --dry-run   # Preview changes
    python Scripts/server_method_snake_case.py              # Apply changes
"""

import os
import re
import sys

# ============================================================================
# RENAME DICTIONARY: OldMethodName -> new_method_name
# Organized by class for readability. The script applies these globally
# using word-boundary regex, longest-first ordering.
# ============================================================================

RENAMES = {
    # ========================================================================
    # CGame (Game.h) - ~125 methods
    # ========================================================================
    "RequestNoticementHandler": "request_noticement_handler",
    "bSendClientNpcConfigs": "send_client_npc_configs",
    "bAcceptLogin": "accept_login",
    "bAcceptFromAsync": "accept_from_async",
    "bAcceptLoginFromAsync": "accept_login_from_async",
    "PartyOperation": "party_operation",
    "_bCheckCharacterData": "check_character_data",
    "GlobalUpdateConfigs": "global_update_configs",
    "LocalUpdateConfigs": "local_update_configs",
    "ReloadNpcConfigs": "reload_npc_configs",
    "SendConfigReloadNotification": "send_config_reload_notification",
    "PushConfigReloadToClients": "push_config_reload_to_clients",
    "Command_RedBall": "command_red_ball",
    "Command_BlueBall": "command_blue_ball",
    "Command_YellowBall": "command_yellow_ball",
    "SetForceRecallTime": "set_force_recall_time",
    "bCheckClientMoveFrequency": "check_client_move_frequency",
    "RequestChangePlayMode": "request_change_play_mode",
    "StateChangeHandler": "state_change_handler",
    "iGetMapLocationSide": "get_map_location_side",
    "ChatMsgHandlerGSM": "chat_msg_handler_gsm",
    "RemoveClientShortCut": "remove_client_short_cut",
    "bAddClientShortCut": "add_client_short_cut",
    "RequestHelpHandler": "request_help_handler",
    "CheckConnectionHandler": "check_connection_handler",
    "AgingMapSectorInfo": "aging_map_sector_info",
    "UpdateMapSectorInfo": "update_map_sector_info",
    "ActivateSpecialAbilityHandler": "activate_special_ability_handler",
    "JoinPartyHandler": "join_party_handler",
    "RequestShopContentsHandler": "request_shop_contents_handler",
    "RequestRestartHandler": "request_restart_handler",
    "iRequestPanningMapDataRequest": "request_panning_map_data_request",
    "RequestCheckAccountPasswordHandler": "request_check_account_password_handler",
    "CheckSpecialEvent": "check_special_event",
    "_cGetSpecialAbility": "get_special_ability",
    "GetMapInitialPoint": "get_map_initial_point",
    "iGetMaxHP": "get_max_hp",
    "iGetMaxMP": "get_max_mp",
    "iGetMaxSP": "get_max_sp",
    "bOnClose": "on_close",
    "ForceDisconnectAccount": "force_disconnect_account",
    "ToggleSafeAttackModeHandler": "toggle_safe_attack_mode_handler",
    "SpecialEventHandler": "special_event_handler",
    "iGetMapIndex": "get_map_index",
    "WhetherProcessor": "weather_processor",
    "_iForcePlayerDisconect": "force_player_disconnect",
    "_iCalcPlayerNum": "calc_player_num",
    "iGetExpLevel": "get_exp_level",
    "___RestorePlayerRating": "restore_player_rating",
    "CalcExpStock": "calc_exp_stock",
    "ResponseSavePlayerDataReplyHandler": "response_save_player_data_reply_handler",
    "NoticeHandler": "notice_handler",
    "bReadNotifyMsgListFile": "read_notify_msg_list_file",
    "SetPlayerReputation": "set_player_reputation",
    "CheckDayOrNightMode": "check_day_or_night_mode",
    "_iGetPlayerNumberOnSpot": "get_player_number_on_spot",
    "___RestorePlayerCharacteristics": "restore_player_characteristics",
    "GetPlayerProfile": "get_player_profile",
    "SetPlayerProfile": "set_player_profile",
    "CheckAndNotifyPlayerConnection": "check_and_notify_player_connection",
    "iCalcTotalWeight": "calc_total_weight",
    "SendObjectMotionRejectMsg": "send_object_motion_reject_msg",
    "iGetFollowerNumber": "get_follower_number",
    "RequestFullObjectData": "request_full_object_data",
    "_iCalcMaxLoad": "calc_max_load",
    "RequestCivilRightHandler": "request_civil_right_handler",
    "bCheckLimitedUser": "check_limited_user",
    "LevelUpSettingsHandler": "level_up_settings_handler",
    "bCheckLevelUp": "check_level_up",
    "iGetLevelExp": "get_level_exp",
    "TimeManaPointsUp": "time_mana_points_up",
    "TimeStaminarPointsUp": "time_stamina_points_up",
    "ReleaseFollowMode": "release_follow_mode",
    "RequestTeleportHandler": "request_teleport_handler",
    "ToggleCombatModeHandler": "toggle_combat_mode_handler",
    "TimeHitPointsUp": "time_hit_points_up",
    "OnStartGameSignal": "on_start_game_signal",
    "iDice": "dice",
    "_bInitNpcAttr": "init_npc_attr",
    "GetNpcConfigIdByName": "get_npc_config_id_by_name",
    "SendNotifyMsg": "send_notify_msg",
    "BroadcastServerMessage": "broadcast_server_message",
    "iClientMotion_Stop_Handler": "client_motion_stop_handler",
    "ClientCommonHandler": "client_common_handler",
    "bGetMsgQuene": "get_msg_queue",
    "MsgProcess": "msg_process",
    "bPutMsgQuene": "put_msg_queue",
    "bGetEmptyPosition": "get_empty_position",
    "cGetNextMoveDir": "get_next_move_dir",
    "iClientMotion_Attack_Handler": "client_motion_attack_handler",
    "ChatMsgHandler": "chat_msg_handler",
    "IsBlockedBy": "is_blocked_by",
    "NpcProcess": "npc_process",
    "bCreateNewNpc": "create_new_npc",
    "SpawnMapNpcsFromDatabase": "spawn_map_npcs_from_database",
    "_bGetIsStringIsNumber": "get_is_string_is_number",
    "GameProcess": "game_process",
    "InitPlayerData": "init_player_data",
    "ResponsePlayerDataHandler": "response_player_data_handler",
    "CheckClientResponseTime": "check_client_response_time",
    "OnTimer": "on_timer",
    "iComposeMoveMapData": "compose_move_map_data",
    "SendEventToNearClient_TypeB": "send_event_to_near_client_type_b",
    "SendEventToNearClient_TypeA": "send_event_to_near_client_type_a",
    "DeleteClient": "delete_client",
    "iComposeInitMapData": "compose_init_map_data",
    "FillPlayerMapObject": "fill_player_map_object",
    "FillNpcMapObject": "fill_npc_map_object",
    "RequestInitDataHandler": "request_init_data_handler",
    "RequestInitPlayerHandler": "request_init_player_handler",
    "iClientMotion_Move_Handler": "client_motion_move_handler",
    "ClientMotionHandler": "client_motion_handler",
    "OnClientRead": "on_client_read",
    "bInit": "init",
    "OnClientSocketEvent": "on_client_socket_event",
    "bAccept": "accept",
    "RequestCreatePartyHandler": "request_create_party_handler",
    "PartyOperationResultHandler": "party_operation_result_handler",
    "PartyOperationResult_Create": "party_operation_result_create",
    "PartyOperationResult_Join": "party_operation_result_join",
    "PartyOperationResult_Dismiss": "party_operation_result_dismiss",
    "PartyOperationResult_Delete": "party_operation_result_delete",
    "RequestJoinPartyHandler": "request_join_party_handler",
    "RequestDismissPartyHandler": "request_dismiss_party_handler",
    "GetPartyInfoHandler": "get_party_info_handler",
    "PartyOperationResult_Info": "party_operation_result_info",
    "RequestDeletePartyHandler": "request_delete_party_handler",
    "RequestAcceptJoinPartyHandler": "request_accept_join_party_handler",
    "GetExp": "get_exp",
    "ForceRecallProcess": "force_recall_process",
    "IsEnemyZone": "is_enemy_zone",
    "LoadPlayerDataFromDb": "load_player_data_from_db",
    "_bRegisterMap": "register_map",
    "ComputeConfigHashes": "compute_config_hashes",
    "OnClientLoginRead": "on_client_login_read",
    "OnLoginClientSocketEvent": "on_login_client_socket_event",
    "DeleteLoginClient": "delete_login_client",
    "FindAdminByAccount": "find_admin_by_account",
    "FindAdminByCharName": "find_admin_by_char_name",
    "IsClientAdmin": "is_client_admin",
    "GetCommandRequiredLevel": "get_command_required_level",
    "FindClientByName": "find_client_by_name",
    "GMTeleportTo": "gm_teleport_to",
    "CheckForceRecallTime": "check_force_recall_time",
    "SetPlayingStatus": "set_playing_status",
    "ForceChangePlayMode": "force_change_play_mode",
    "ShowVersion": "show_version",
    "ShowClientMsg": "show_client_msg",
    "RequestResurrectPlayer": "request_resurrect_player",
    "LoteryHandler": "lotery_handler",
    "GetAngelHandler": "get_angel_handler",
    "RequestEnchantUpgradeHandler": "request_enchant_upgrade_handler",
    "GetRequiredLevelForUpgrade": "get_required_level_for_upgrade",

    # ========================================================================
    # CClient (Client.h) - 1 method
    # ========================================================================
    "bCreateNewParty": "create_new_party",

    # ========================================================================
    # CEntityManager (EntityManager.h) - 50+ methods
    # ========================================================================
    "ProcessSpawns": "process_spawns",
    "CreateEntity": "create_entity",
    "DeleteEntity": "delete_entity",
    "OnEntityKilled": "on_entity_killed",
    "ProcessEntities": "process_entities",
    "UpdateDeadBehavior": "update_dead_behavior",
    "UpdateMoveBehavior": "update_move_behavior",
    "UpdateAttackBehavior": "update_attack_behavior",
    "UpdateStopBehavior": "update_stop_behavior",
    "UpdateFleeBehavior": "update_flee_behavior",
    "NpcBehavior_Move": "npc_behavior_move",
    "TargetSearch": "target_search",
    "NpcBehavior_Attack": "npc_behavior_attack",
    "NpcBehavior_Flee": "npc_behavior_flee",
    "NpcBehavior_Stop": "npc_behavior_stop",
    "NpcBehavior_Dead": "npc_behavior_dead",
    "CalcNextWayPointDestination": "calc_next_waypoint_destination",
    "NpcMagicHandler": "npc_magic_handler",
    "GetNpcRelationship": "get_npc_relationship",
    "NpcRequestAssistance": "npc_request_assistance",
    "_bNpcBehavior_ManaCollector": "npc_behavior_mana_collector",
    "_bNpcBehavior_Detector": "npc_behavior_detector",
    "_NpcBehavior_GrandMagicGenerator": "npc_behavior_grand_magic_generator",
    "bSetNpcFollowMode": "set_npc_follow_mode",
    "bSetNpcAttackMode": "set_npc_attack_mode",
    "GetEntity": "get_entity",
    "GetEntityByGUID": "get_entity_by_guid",
    "GetEntityHandleByGUID": "get_entity_handle_by_guid",
    "GetEntityGUID": "get_entity_guid",
    "GetTotalActiveEntities": "get_total_active_entities",
    "GetMapEntityCount": "get_map_entity_count",
    "FindEntityByName": "find_entity_by_name",
    "GetActiveEntityList": "get_active_entity_list",
    "GetActiveEntityCount": "get_active_entity_count",
    "GetEntityArray": "get_entity_array",
    "SetMapList": "set_map_list",
    "SetGame": "set_game",
    "InitEntityAttributes": "init_entity_attributes",
    "GetFreeEntitySlot": "get_free_entity_slot",
    "IsValidEntity": "is_valid_entity",
    "GenerateEntityLoot": "generate_entity_loot",
    "DeleteNpcInternal": "delete_npc_internal",
    "NpcDeadItemGenerator": "npc_dead_item_generator",
    "RollDropTableItem": "roll_drop_table_item",
    "SpawnNpcDropItem": "spawn_npc_drop_item",
    "ProcessRandomSpawns": "process_random_spawns",
    "ProcessSpotSpawns": "process_spot_spawns",
    "CanSpawnAtSpot": "can_spawn_at_spot",
    "GenerateEntityGUID": "generate_entity_guid",
    "AddToActiveList": "add_to_active_list",
    "RemoveFromActiveList": "remove_from_active_list",

    # ========================================================================
    # CMsg (NetworkMsg.h) - 1 safe method (Get is too generic)
    # ========================================================================
    "bPut": "put",
    # "Get" excluded - too generic for global regex

    # ========================================================================
    # GameChatCommand / GameChatCommandManager (GameChatCommand.h)
    # ========================================================================
    "GetName": "get_name",
    "GetDefaultLevel": "get_default_level",
    "RequiresGMMode": "requires_gm_mode",
    "Execute": "execute",
    "Initialize": "initialize",
    "RegisterCommand": "register_command",
    "ProcessCommand": "process_command",
    "RegisterBuiltInCommands": "register_built_in_commands",
    "SeedCommandPermissions": "seed_command_permissions",
    "LogCommand": "log_command",

    # ========================================================================
    # PartyManager (PartyManager.h)
    # ========================================================================
    "CheckMemberActivity": "check_member_activity",
    "SetServerChangeStatus": "set_server_change_status",
    "GameServerDown": "game_server_down",
    "bGetPartyInfo": "get_party_info",
    "bCheckPartyMember": "check_party_member",
    "bRemoveMember": "remove_member",
    "bAddMember": "add_member",
    "bDeleteParty": "delete_party",
    "iCreateNewParty": "create_new_party_id",

    # ========================================================================
    # ItemManager (ItemManager.h) - 60 methods
    # ========================================================================
    "bSendClientItemConfigs": "send_client_item_configs",
    "GetDropTable": "get_drop_table",
    "_ClearItemConfigList": "clear_item_config_list",
    "_bInitItemAttr": "init_item_attr",
    "ReloadItemConfigs": "reload_item_configs",
    "_AdjustRareItemValue": "adjust_rare_item_value",
    "GenerateItemAttributes": "generate_item_attributes",
    "RollAttributeValue": "roll_attribute_value",
    "bAddItem": "add_item",
    "_bAddClientItemList": "add_client_item_list",
    "_bAddClientBulkItemList": "add_client_bulk_item_list",
    "ReleaseItemHandler": "release_item_handler",
    "SetItemCount": "set_item_count",
    "SetItemCountByID": "set_item_count_by_id",
    "dwGetItemCountByID": "get_item_count_by_id",
    "_iGetItemSpaceLeft": "get_item_space_left",
    "_SetItemPos": "set_item_pos",
    "iGetItemWeight": "get_item_weight",
    "bCopyItemContents": "copy_item_contents",
    "_bCheckItemReceiveCondition": "check_item_receive_condition",
    "SendItemNotifyMsg": "send_item_notify_msg",
    "UseItemHandler": "use_item_handler",
    "ItemDepleteHandler": "item_deplete_handler",
    "_bDepleteDestTypeItemUseEffect": "deplete_dest_type_item_use_effect",
    "iCalculateUseSkillItemEffect": "calculate_use_skill_item_effect",
    "bPlantSeedBag": "plant_seed_bag",
    "bEquipItemHandler": "equip_item_handler",
    "CalcTotalItemEffect": "calc_total_item_effect",
    "CheckUniqueItemEquipment": "check_unique_item_equipment",
    "bCheckAndConvertPlusWeaponItem": "check_and_convert_plus_weapon_item",
    "_cCheckHeroItemEquipped": "check_hero_item_equipped",
    "_iGetArrowItemIndex": "get_arrow_item_index",
    "CalculateSSN_ItemIndex": "calculate_ssn_item_index",
    "DropItemHandler": "drop_item_handler",
    "iClientMotion_GetItem_Handler": "client_motion_get_item_handler",
    "GiveItemHandler": "give_item_handler",
    "ExchangeItemHandler": "exchange_item_handler",
    "SetExchangeItem": "set_exchange_item",
    "ConfirmExchangeItem": "confirm_exchange_item",
    "CancelExchangeItem": "cancel_exchange_item",
    "_ClearExchangeStatus": "clear_exchange_status",
    "bSetItemToBankItem": "set_item_to_bank_item",
    "RequestRetrieveItemHandler": "request_retrieve_item_handler",
    "RequestPurchaseItemHandler": "request_purchase_item_handler",
    "RequestSellItemListHandler": "request_sell_item_list_handler",
    "ReqSellItemHandler": "req_sell_item_handler",
    "ReqSellItemConfirmHandler": "req_sell_item_confirm_handler",
    "ReqRepairItemHandler": "req_repair_item_handler",
    "ReqRepairItemCofirmHandler": "req_repair_item_cofirm_handler",
    "RequestRepairAllItemsHandler": "request_repair_all_items_handler",
    "RequestRepairAllItemsDeleteHandler": "request_repair_all_items_delete_handler",
    "RequestRepairAllItemsConfirmHandler": "request_repair_all_items_confirm_handler",
    "BuildItemHandler": "build_item_handler",
    "bCheckIsItemUpgradeSuccess": "check_is_item_upgrade_success",
    "RequestItemUpgradeHandler": "request_item_upgrade_handler",
    "GetHeroMantleHandler": "get_hero_mantle_handler",
    "ReqCreateSlateHandler": "req_create_slate_handler",
    "SetSlateFlag": "set_slate_flag",
    "_bItemLog": "item_log",
    "_bCheckGoodItem": "check_good_item",

    # ========================================================================
    # CombatManager (CombatManager.h) - 25 methods
    # ========================================================================
    "iCalculateAttackEffect": "calculate_attack_effect",
    "bCalculateEnduranceDecrement": "calculate_endurance_decrement",
    "Effect_Damage_Spot": "effect_damage_spot",
    "Effect_Damage_Spot_DamageMove": "effect_damage_spot_damage_move",
    "Effect_HpUp_Spot": "effect_hp_up_spot",
    "Effect_SpUp_Spot": "effect_sp_up_spot",
    "Effect_SpDown_Spot": "effect_sp_down_spot",
    "bCheckResistingMagicSuccess": "check_resisting_magic_success",
    "bCheckResistingIceSuccess": "check_resisting_ice_success",
    "bCheckResistingPoisonSuccess": "check_resisting_poison_success",
    "ClientKilledHandler": "client_killed_handler",
    "PoisonEffect": "poison_effect",
    "CheckFireBluring": "check_fire_bluring",
    "ArmorLifeDecrement": "armor_life_decrement",
    "_CheckAttackType": "check_attack_type",
    "_iGetWeaponSkillType": "get_weapon_skill_type",
    "iGetComboAttackBonus": "get_combo_attack_bonus",
    "bAnalyzeCriminalAction": "analyze_criminal_action",
    "_bGetIsPlayerHostile": "get_is_player_hostile",
    "iGetPlayerRelationship": "get_player_relationship_raw",
    "GetPlayerRelationship": "get_player_relationship",
    "RemoveFromTarget": "remove_from_target",
    "iGetDangerValue": "get_danger_value",
    "bCheckClientAttackFrequency": "check_client_attack_frequency",
    "_bPKLog": "pk_log",

    # ========================================================================
    # MagicManager (MagicManager.h) - 10 methods
    # ========================================================================
    "bSendClientMagicConfigs": "send_client_magic_configs",
    "ReloadMagicConfigs": "reload_magic_configs",
    "PlayerMagicHandler": "player_magic_handler",
    "iClientMotion_Magic_Handler": "client_motion_magic_handler",
    "RequestStudyMagicHandler": "request_study_magic_handler",
    "_iGetMagicNumber": "get_magic_number",
    "GetMagicAbilityHandler": "get_magic_ability_handler",
    "bCheckMagicInt": "check_magic_int",
    "bCheckClientMagicFrequency": "check_client_magic_frequency",
    "iGetWhetherMagicBonusEffect": "get_weather_magic_bonus_effect",

    # ========================================================================
    # SkillManager (SkillManager.h) - 12 methods
    # ========================================================================
    "bSendClientSkillConfigs": "send_client_skill_configs",
    "ReloadSkillConfigs": "reload_skill_configs",
    "UseSkillHandler": "use_skill_handler",
    "ClearSkillUsingStatus": "clear_skill_using_status",
    "_TamingHandler": "taming_handler",
    "CalculateSSN_SkillIndex": "calculate_ssn_skill_index",
    "bCheckTotalSkillMasteryPoints": "check_total_skill_mastery_points",
    "_iCalcSkillSSNpoint": "calc_skill_ssn_point",
    "TrainSkillResponse": "train_skill_response",
    "SetDownSkillIndexHandler": "set_down_skill_index_handler",
    "SkillCheck": "skill_check",
    "SetSkillAll": "set_skill_all",

    # ========================================================================
    # WarManager (WarManager.h) - 62 methods
    # ========================================================================
    # Crusade
    "CrusadeWarStarter": "crusade_war_starter",
    "GlobalStartCrusadeMode": "global_start_crusade_mode",
    "LocalStartCrusadeMode": "local_start_crusade_mode",
    "LocalEndCrusadeMode": "local_end_crusade_mode",
    "ManualEndCrusadeMode": "manual_end_crusade_mode",
    "CreateCrusadeStructures": "create_crusade_structures",
    "RemoveCrusadeStructures": "remove_crusade_structures",
    "RemoveCrusadeNpcs": "remove_crusade_npcs",
    "RemoveCrusadeRecallTime": "remove_crusade_recall_time",
    "SyncMiddlelandMapInfo": "sync_middleland_map_info",
    "SelectCrusadeDutyHandler": "select_crusade_duty_handler",
    "CheckCrusadeResultCalculation": "check_crusade_result_calculation",
    "bReadCrusadeGUIDFile": "read_crusade_guid_file",
    "_CreateCrusadeGUID": "create_crusade_guid",
    "CheckCommanderConstructionPoint": "check_commander_construction_point",
    "__bSetConstructionKit": "set_construction_kit",
    # Grand Magic / Meteor Strike
    "MeteorStrikeHandler": "meteor_strike_handler",
    "MeteorStrikeMsgHandler": "meteor_strike_msg_handler",
    "CalcMeteorStrikeEffectHandler": "calc_meteor_strike_effect_handler",
    "DoMeteorStrikeDamageHandler": "do_meteor_strike_damage_handler",
    "_LinkStrikePointMapIndex": "link_strike_point_map_index",
    "_GrandMagicLaunchMsgSend": "grand_magic_launch_msg_send",
    "GrandMagicResultHandler": "grand_magic_result_handler",
    "CollectedManaHandler": "collected_mana_handler",
    "SendCollectedMana": "send_collected_mana",
    # Map Status & Guild War
    "_SendMapStatus": "send_map_status",
    "MapStatusHandler": "map_status_handler",
    "RequestSummonWarUnitHandler": "request_summon_war_unit_handler",
    "RequestGuildTeleportHandler": "request_guild_teleport_handler",
    "RequestSetGuildTeleportLocHandler": "request_set_guild_teleport_loc_handler",
    "RequestSetGuildConstructLocHandler": "request_set_guild_construct_loc_handler",
    # Heldenian
    "SetHeldenianMode": "set_heldenian_mode",
    "GlobalStartHeldenianMode": "global_start_heldenian_mode",
    "LocalStartHeldenianMode": "local_start_heldenian_mode",
    "GlobalEndHeldenianMode": "global_end_heldenian_mode",
    "LocalEndHeldenianMode": "local_end_heldenian_mode",
    "UpdateHeldenianStatus": "update_heldenian_status",
    "_CreateHeldenianGUID": "create_heldenian_guid",
    "ManualStartHeldenianMode": "manual_start_heldenian_mode",
    "ManualEndHeldenianMode": "manual_end_heldenian_mode",
    "bNotifyHeldenianWinner": "notify_heldenian_winner",
    "RemoveHeldenianNpc": "remove_heldenian_npc",
    "RequestHeldenianTeleport": "request_heldenian_teleport",
    "bCheckHeldenianMap": "check_heldenian_map",
    "CheckHeldenianResultCalculation": "check_heldenian_result_calculation",
    "RemoveOccupyFlags": "remove_occupy_flags",
    # Apocalypse
    "ApocalypseEnder": "apocalypse_ender",
    "GlobalEndApocalypseMode": "global_end_apocalypse_mode",
    "LocalEndApocalypse": "local_end_apocalypse",
    "LocalStartApocalypse": "local_start_apocalypse",
    "bReadApocalypseGUIDFile": "read_apocalypse_guid_file",
    "bReadHeldenianGUIDFile": "read_heldenian_guid_file",
    "_CreateApocalypseGUID": "create_apocalypse_guid",
    # Energy Sphere & Occupy
    "EnergySphereProcessor": "energy_sphere_processor",
    "bCheckEnergySphereDestination": "check_energy_sphere_destination",
    "GetOccupyFlagHandler": "get_occupy_flag_handler",
    "_iComposeFlagStatusContents": "compose_flag_status_contents",
    "SetSummonMobAction": "set_summon_mob_action",
    "__bSetOccupyFlag": "set_occupy_flag",
    # FightZone
    "FightzoneReserveHandler": "fightzone_reserve_handler",
    "FightzoneReserveProcessor": "fightzone_reserve_processor",
    "GetFightzoneTicketHandler": "get_fightzone_ticket_handler",

    # ========================================================================
    # GuildManager (GuildManager.h) - 15 methods
    # ========================================================================
    "RequestCreateNewGuildHandler": "request_create_new_guild_handler",
    "ResponseCreateNewGuildHandler": "response_create_new_guild_handler",
    "RequestDisbandGuildHandler": "request_disband_guild_handler",
    "ResponseDisbandGuildHandler": "response_disband_guild_handler",
    "JoinGuildApproveHandler": "join_guild_approve_handler",
    "JoinGuildRejectHandler": "join_guild_reject_handler",
    "DismissGuildApproveHandler": "dismiss_guild_approve_handler",
    "DismissGuildRejectHandler": "dismiss_guild_reject_handler",
    "SendGuildMsg": "send_guild_msg",
    "GuildNotifyHandler": "guild_notify_handler",
    "UserCommand_BanGuildsman": "user_command_ban_guildsman",
    "UserCommand_DissmissGuild": "user_command_dismiss_guild",
    "RequestCreateNewGuild": "request_create_new_guild",
    "RequestDisbandGuild": "request_disband_guild",
    "RequestGuildNameHandler": "request_guild_name_handler",

    # ========================================================================
    # QuestManager (QuestManager.h) - 12 methods
    # ========================================================================
    "NpcTalkHandler": "npc_talk_handler",
    "QuestAcceptedHandler": "quest_accepted_handler",
    "CancelQuestHandler": "cancel_quest_handler",
    "_SendQuestContents": "send_quest_contents",
    "_CheckQuestEnvironment": "check_quest_environment",
    "_bCheckIsQuestCompleted": "check_is_quest_completed",
    "_ClearQuestStatus": "clear_quest_status",
    "_iTalkToNpcResult_Cityhall": "talk_to_npc_result_cityhall",
    "_iTalkToNpcResult_Guard": "talk_to_npc_result_guard",
    "__iSearchForQuest": "search_for_quest",

    # ========================================================================
    # LootManager (LootManager.h) - 6 methods
    # ========================================================================
    "PK_KillRewardHandler": "pk_kill_reward_handler",
    "EnemyKillRewardHandler": "enemy_kill_reward_handler",
    "GetRewardMoneyHandler": "get_reward_money_handler",
    "ApplyPKpenalty": "apply_pk_penalty",
    "ApplyCombatKilledPenalty": "apply_combat_killed_penalty",
    "_PenaltyItemDrop": "penalty_item_drop",

    # ========================================================================
    # StatusEffectManager (StatusEffectManager.h) - 14 methods
    # ========================================================================
    "SetHeroFlag": "set_hero_flag",
    "SetBerserkFlag": "set_berserk_flag",
    "SetHasteFlag": "set_haste_flag",
    "SetPoisonFlag": "set_poison_flag",
    "SetDefenseShieldFlag": "set_defense_shield_flag",
    "SetMagicProtectionFlag": "set_magic_protection_flag",
    "SetProtectionFromArrowFlag": "set_protection_from_arrow_flag",
    "SetIllusionMovementFlag": "set_illusion_movement_flag",
    "SetIllusionFlag": "set_illusion_flag",
    "SetIceFlag": "set_ice_flag",
    "SetInvisibilityFlag": "set_invisibility_flag",
    "SetInhibitionCastingFlag": "set_inhibition_casting_flag",
    "SetAngelFlag": "set_angel_flag",
    "_CheckFarmingAction": "check_farming_action",

    # ========================================================================
    # FishingManager (FishingManager.h) - 7 methods
    # ========================================================================
    "FishGenerator": "fish_generator",
    "FishProcessor": "fish_processor",
    "ReqGetFishThisTimeHandler": "req_get_fish_this_time_handler",
    "iCheckFish": "check_fish",
    "iCreateFish": "create_fish",
    "bDeleteFish": "delete_fish",
    "ReleaseFishEngagement": "release_fish_engagement",

    # ========================================================================
    # MiningManager (MiningManager.h) - 4 methods
    # ========================================================================
    "MineralGenerator": "mineral_generator",
    "iCreateMineral": "create_mineral",
    "_CheckMiningAction": "check_mining_action",
    "bDeleteMineral": "delete_mineral",

    # ========================================================================
    # CraftingManager (CraftingManager.h) - 2 methods
    # ========================================================================
    "ReqCreatePortionHandler": "req_create_portion_handler",
    "ReqCreateCraftingHandler": "req_create_crafting_handler",

    # ========================================================================
    # DelayEventManager (DelayEventManager.h) - 4 methods
    # ========================================================================
    "bRegisterDelayEvent": "register_delay_event",
    "bRemoveFromDelayEventList": "remove_from_delay_event_list",
    "DelayEventProcessor": "delay_event_processor",
    "DelayEventProcess": "delay_event_process",

    # ========================================================================
    # DynamicObjectManager (DynamicObjectManager.h) - 3 methods
    # ========================================================================
    "iAddDynamicObjectList": "add_dynamic_object_list",
    "CheckDynamicObjectList": "check_dynamic_object_list",
    "DynamicObjectEffectProcessor": "dynamic_object_effect_processor",

    # ========================================================================
    # CMap (Map.h) - 35 methods
    # ========================================================================
    "bCheckFlySpaceAvailable": "check_fly_space_available",
    "bGetIsFarm": "get_is_farm",
    "RestoreStrikePoints": "restore_strike_points",
    "bRemoveCrusadeStructureInfo": "remove_crusade_structure_info",
    "bAddCrusadeStructureInfo": "add_crusade_structure_info",
    "iGetAttribute": "get_attribute",
    "_SetupNoAttackArea": "setup_no_attack_area",
    "ClearTempSectorInfo": "clear_temp_sector_info",
    "ClearSectorInfo": "clear_sector_info",
    "iRegisterOccupyFlag": "register_occupy_flag",
    "iCheckItem": "check_item",
    "SetTempMoveAllowedFlag": "set_temp_move_allowed_flag",
    "iAnalyze": "analyze",
    "bGetIsWater": "get_is_water",
    "GetDeadOwner": "get_dead_owner",
    "bGetIsMoveAllowedTile": "get_is_move_allowed_tile",
    "SetNamingValueEmpty": "set_naming_value_empty",
    "iGetEmptyNamingValue": "get_empty_naming_value",
    "bGetDynamicObject": "get_dynamic_object",
    "SetDynamicObject": "set_dynamic_object",
    "bGetIsTeleport": "get_is_teleport",
    "bSearchTeleportDest": "search_teleport_dest",
    "bIsValidLoc": "is_valid_loc",
    "pGetItem": "get_item",
    "bSetItem": "set_item",
    "ClearDeadOwner": "clear_dead_owner",
    "ClearOwner": "clear_owner",
    "bGetMoveable": "get_moveable",
    "GetOwner": "get_owner",
    "SetOwner": "set_owner",
    "SetDeadOwner": "set_dead_owner",
    "bRemoveCropsTotalSum": "remove_crops_total_sum",
    "bAddCropsTotalSum": "add_crops_total_sum",
    "SetBigOwner": "set_big_owner",
    "_bDecodeMapDataFileContents": "decode_map_data_file_contents",

    # ========================================================================
    # Shared: CItem (Item.h) - accessors
    # ========================================================================
    "GetDisplayName": "get_display_name",
    "GetEquipPos": "get_equip_pos",
    "SetEquipPos": "set_equip_pos",
    "GetItemType": "get_item_type",
    "SetItemType": "set_item_type",
    "GetItemEffectType": "get_item_effect_type",
    "SetItemEffectType": "set_item_effect_type",
    "GetTouchEffectType": "get_touch_effect_type",
    "SetTouchEffectType": "set_touch_effect_type",
    "IsCustomMade": "is_custom_made",
    "SetCustomMade": "set_custom_made",
    "GetAttributeType": "get_attribute_type",
    "GetAttributeValue": "get_attribute_value",
    "IsStackable": "is_stackable",
    "IsWeapon": "is_weapon",
    "IsArmor": "is_armor",
    "IsAccessory": "is_accessory",

    # ========================================================================
    # Shared: ASIOSocket (ASIOSocket.h) - 30+ methods
    # ========================================================================
    "bInitBufferSize": "init_buffer_size",
    "bConnect": "connect",
    "bBlockConnect": "block_connect",
    "bListen": "listen",
    "bAcceptFromSocket": "accept_from_socket",
    "iSendMsg": "send_msg",
    "iSendMsgBlockingMode": "send_msg_blocking_mode",
    "iSendMsgAsync": "send_msg_async",
    "pGetRcvDataPointer": "get_rcv_data_pointer",
    "DrainToQueue": "drain_to_queue",
    "PeekPacket": "peek_packet",
    "PopPacket": "pop_packet",
    "HasPendingPackets": "has_pending_packets",
    "GetQueueSize": "get_queue_size",
    "ClearQueue": "clear_queue",
    "QueueCompletedPacket": "queue_completed_packet",
    "iGetPeerAddress": "get_peer_address",
    "iGetSocket": "get_socket",
    "CloseConnection": "close_connection",
    "SetSocketIndex": "set_socket_index",
    "SetCallbacks": "set_callbacks",
    "StartAsyncRead": "start_async_read",
    "StartAsyncAccept": "start_async_accept",
    "CancelAsync": "cancel_async",
    "_iOnRead": "on_read",
    "_iSend": "send",
    "_iSend_ForInternalUse": "send_for_internal_use",
    "_iSendUnsentData": "send_unsent_data",
    "_iRegisterUnsentData": "register_unsent_data",
    "_DoAsyncReadHeader": "do_async_read_header",
    "_DoAsyncReadBody": "do_async_read_body",
    "_DoAsyncWrite": "do_async_write",

    # ========================================================================
    # Shared: ItemEnums.h - free functions
    # ========================================================================
    "IsWeaponSlot": "is_weapon_slot",
    "IsArmorSlot": "is_armor_slot",
    "IsAccessorySlot": "is_accessory_slot",
    "IsStackableType": "is_stackable_type",
    "IsTrueStackType": "is_true_stack_type",
    "IsAttackEffectType": "is_attack_effect_type",
    "IsConsumableEffectType": "is_consumable_effect_type",
    "ToEquipPos": "to_equip_pos",
    "ToItemType": "to_item_type",
    "ToItemEffectType": "to_item_effect_type",
    "ToTouchEffectType": "to_touch_effect_type",
    "IsSpecialItem": "is_special_item",
    "ToInt": "to_int",

    # ========================================================================
    # Shared: ItemAttributes.h - free functions
    # ========================================================================
    "HasSpecialAttributes": "has_special_attributes",
    "GetPrimaryEffectMultiplier": "get_primary_effect_multiplier",
    "GetSecondaryEffectMultiplier": "get_secondary_effect_multiplier",
    "ParseAttribute": "parse_attribute",
    "HasSpecialEffects": "has_special_effects",
    "GetEnchantBonus": "get_enchant_bonus",
    "BuildAttribute": "build_attribute",
    "SetEnchantBonus": "set_enchant_bonus",

    # ========================================================================
    # Shared: IOServicePool (IOServicePool.h)
    # ========================================================================
    "GetContext": "get_context",

    # ========================================================================
    # Shared: ConcurrentMsgQueue/ConcurrentQueue
    # ========================================================================
    # Push, Pop, Size, Empty excluded - too generic for global regex

    # ========================================================================
    # LoginServer (LoginServer.h)
    # ========================================================================
    "RequestLogin": "request_login",
    "GetCharList": "get_char_list",
    "ResponseCharacter": "response_character",
    "DeleteCharacter": "delete_character",
    "ChangePassword": "change_password",
    "RequestEnterGame": "request_enter_game",
    "CreateNewAccount": "create_new_account",
    "SendLoginMsg": "send_login_msg",
    "LocalSavePlayerData": "local_save_player_data",
    "Activated": "activated",

    # ========================================================================
    # Common methods across multiple managers
    # ========================================================================
    "InitArrays": "init_arrays",
    "CleanupArrays": "cleanup_arrays",
}

# ============================================================================
# Processing Logic (same pattern as server_snake_case.py)
# ============================================================================

SOURCE_DIRS = [
    os.path.join("Sources", "Server"),
    os.path.join("Sources", "Dependencies", "Shared"),
]

EXTENSIONS = {".h", ".cpp"}

def find_source_files(base_dir):
    """Find all .h and .cpp files in the source directories."""
    files = []
    for src_dir in SOURCE_DIRS:
        full_dir = os.path.join(base_dir, src_dir)
        if not os.path.isdir(full_dir):
            continue
        for root, dirs, filenames in os.walk(full_dir):
            for fn in filenames:
                ext = os.path.splitext(fn)[1].lower()
                if ext in EXTENSIONS:
                    files.append(os.path.join(root, fn))
    return sorted(files)


def build_replacements():
    """Build sorted list of (compiled_regex, new_name) tuples, longest-first."""
    items = sorted(RENAMES.items(), key=lambda kv: len(kv[0]), reverse=True)
    result = []
    for old, new in items:
        # Escape the old name for regex (handles underscores etc.)
        escaped = re.escape(old)
        pattern = re.compile(r"\b" + escaped + r"\b")
        result.append((old, pattern, new))
    return result


def process_file(filepath, replacements, dry_run=False):
    """Apply all renames to a single file. Returns (change_count, detail_lines)."""
    with open(filepath, "r", encoding="utf-8", errors="replace") as f:
        lines = f.readlines()

    total_changes = 0
    details = []
    new_lines = []

    for line_num, line in enumerate(lines, start=1):
        # Skip #include lines to avoid corrupting include paths
        stripped = line.lstrip()
        if stripped.startswith("#include"):
            new_lines.append(line)
            continue

        new_line = line
        for old_name, pattern, new_name in replacements:
            matches = pattern.findall(new_line)
            if matches:
                count = len(matches)
                new_line = pattern.sub(new_name, new_line)
                total_changes += count
                details.append(f"  L{line_num}: {old_name} -> {new_name} (x{count})")

        new_lines.append(new_line)

    if total_changes > 0 and not dry_run:
        with open(filepath, "w", encoding="utf-8", newline="") as f:
            f.writelines(new_lines)

    return total_changes, details


def main():
    dry_run = "--dry-run" in sys.argv

    # Find the project root (script is in Scripts/)
    script_dir = os.path.dirname(os.path.abspath(__file__))
    base_dir = os.path.dirname(script_dir)

    print(f"{'DRY RUN - ' if dry_run else ''}Server Method snake_case Conversion")
    print(f"Base directory: {base_dir}")
    print(f"Rename entries: {len(RENAMES)}")
    print()

    files = find_source_files(base_dir)
    print(f"Source files found: {len(files)}")
    print()

    replacements = build_replacements()

    total_changes = 0
    files_changed = 0

    for filepath in files:
        rel_path = os.path.relpath(filepath, base_dir)
        changes, details = process_file(filepath, replacements, dry_run)
        if changes > 0:
            files_changed += 1
            total_changes += changes
            print(f"{rel_path}: {changes} replacements")
            for d in details[:20]:  # Limit detail output per file
                print(d)
            if len(details) > 20:
                print(f"  ... and {len(details) - 20} more")

    print()
    print(f"Total: {total_changes} replacements across {files_changed} files")
    if dry_run:
        print("DRY RUN - no files were modified")


if __name__ == "__main__":
    main()

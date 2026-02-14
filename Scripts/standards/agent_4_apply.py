#!/usr/bin/env python3
"""Agent 4: GuildManager + QuestManager snake_case conversion.

Converts all non-compliant identifiers in GuildManager.h/.cpp and
QuestManager.h/.cpp (and all call sites across Client/) to snake_case
per the project coding standards.
"""
import glob, os, re

# Skip .bak files -- only process actual source files
def get_source_files():
    files = sorted(
        glob.glob('Z:/Helbreath-3.82/Sources/Client/*.h') +
        glob.glob('Z:/Helbreath-3.82/Sources/Client/*.cpp')
    )
    return [f for f in files if '.bak_' not in f]

REPLACEMENTS = [
    # =====================================================================
    # GuildManager methods (long, unique names -- safe with \b)
    # =====================================================================
    (r'\bHandleCreateNewGuildResponse\b', 'handle_create_new_guild_response',
     'GuildManager::HandleCreateNewGuildResponse -> handle_create_new_guild_response'),
    (r'\bHandleDisbandGuildResponse\b', 'handle_disband_guild_response',
     'GuildManager::HandleDisbandGuildResponse -> handle_disband_guild_response'),
    (r'\bHandleGuildDisbanded\b', 'handle_guild_disbanded',
     'GuildManager::HandleGuildDisbanded -> handle_guild_disbanded'),
    (r'\bHandleNewGuildsMan\b', 'handle_new_guilds_man',
     'GuildManager::HandleNewGuildsMan -> handle_new_guilds_man'),
    (r'\bHandleDismissGuildsMan\b', 'handle_dismiss_guilds_man',
     'GuildManager::HandleDismissGuildsMan -> handle_dismiss_guilds_man'),
    (r'\bHandleCannotJoinMoreGuildsMan\b', 'handle_cannot_join_more_guilds_man',
     'GuildManager::HandleCannotJoinMoreGuildsMan -> handle_cannot_join_more_guilds_man'),
    (r'\bHandleJoinGuildApprove\b', 'handle_join_guild_approve',
     'GuildManager::HandleJoinGuildApprove -> handle_join_guild_approve'),
    (r'\bHandleJoinGuildReject\b', 'handle_join_guild_reject',
     'GuildManager::HandleJoinGuildReject -> handle_join_guild_reject'),
    (r'\bHandleDismissGuildApprove\b', 'handle_dismiss_guild_approve',
     'GuildManager::HandleDismissGuildApprove -> handle_dismiss_guild_approve'),
    (r'\bHandleDismissGuildReject\b', 'handle_dismiss_guild_reject',
     'GuildManager::HandleDismissGuildReject -> handle_dismiss_guild_reject'),
    (r'\bHandleQueryJoinGuildPermission\b', 'handle_query_join_guild_permission',
     'GuildManager::HandleQueryJoinGuildPermission -> handle_query_join_guild_permission'),
    (r'\bHandleQueryDismissGuildPermission\b', 'handle_query_dismiss_guild_permission',
     'GuildManager::HandleQueryDismissGuildPermission -> handle_query_dismiss_guild_permission'),
    (r'\bHandleReqGuildNameAnswer\b', 'handle_req_guild_name_answer',
     'GuildManager::HandleReqGuildNameAnswer -> handle_req_guild_name_answer'),
    (r'\bHandleNoGuildMasterLevel\b', 'handle_no_guild_master_level',
     'GuildManager::HandleNoGuildMasterLevel -> handle_no_guild_master_level'),
    (r'\bHandleSuccessBanGuildMan\b', 'handle_success_ban_guild_man',
     'GuildManager::HandleSuccessBanGuildMan -> handle_success_ban_guild_man'),
    (r'\bHandleCannotBanGuildMan\b', 'handle_cannot_ban_guild_man',
     'GuildManager::HandleCannotBanGuildMan -> handle_cannot_ban_guild_man'),
    (r'\bUpdateLocationFlags\b', 'update_location_flags',
     'GuildManager::UpdateLocationFlags -> update_location_flags'),

    # =====================================================================
    # QuestManager methods (long, unique names -- safe with \b)
    # =====================================================================
    (r'\bHandleQuestCounter\b', 'handle_quest_counter',
     'QuestManager::HandleQuestCounter -> handle_quest_counter'),
    (r'\bHandleQuestContents\b', 'handle_quest_contents',
     'QuestManager::HandleQuestContents -> handle_quest_contents'),
    (r'\bHandleQuestReward\b', 'handle_quest_reward',
     'QuestManager::HandleQuestReward -> handle_quest_reward'),
    (r'\bHandleQuestCompleted\b', 'handle_quest_completed',
     'QuestManager::HandleQuestCompleted -> handle_quest_completed'),
    (r'\bHandleQuestAborted\b', 'handle_quest_aborted',
     'QuestManager::HandleQuestAborted -> handle_quest_aborted'),

    # =====================================================================
    # Shared method: SetGame (used by many manager classes)
    # =====================================================================
    (r'\bSetGame\b', 'set_game',
     'SetGame -> set_game (shared across all managers)'),

    # =====================================================================
    # Quest struct DEFINITION in Game.h (BEFORE dot-prefix field renames)
    # Must match ORIGINAL text since dot-prefix patterns don't affect decls.
    # =====================================================================
    # Full declaration line with all fields including sX, sY, sRange
    (r'\bshort sWho, sQuestType, sContribution, sTargetType, sTargetCount, sX, sY, sRange;',
     'short who, quest_type, contribution, target_type, target_count, x, y, range;',
     'quest struct definition: rename all fields in multi-field declaration'),
    (r'\bshort sCurrentCount;', 'short current_count;',
     'quest struct definition: sCurrentCount -> current_count'),
    (r'\bbool bIsQuestCompleted;', 'bool is_quest_completed;',
     'quest struct definition: bIsQuestCompleted -> is_quest_completed'),
    (r'\bstd::string cTargetName;', 'std::string target_name;',
     'quest struct definition: cTargetName -> target_name'),

    # =====================================================================
    # Guild struct DEFINITION in Game.h (BEFORE dot-prefix field renames)
    # =====================================================================
    (r'\buint32_t dwRefTime;', 'uint32_t ref_time;',
     'guild struct definition: dwRefTime -> ref_time'),
    (r'\bint iGuildRank;', 'int guild_rank;',
     'guild struct definition: iGuildRank -> guild_rank'),
    (r'\bstd::string cCharName;', 'std::string char_name;',
     'guild struct definition: cCharName -> char_name'),
    (r'\bstd::string cGuildName;', 'std::string guild_name;',
     'guild struct definition: cGuildName -> guild_name'),

    # =====================================================================
    # Quest struct fields -- DOT-PREFIXED access patterns
    # These MUST be applied BEFORE renaming m_stQuest -> m_quest
    # =====================================================================
    (r'\.bIsQuestCompleted\b', '.is_quest_completed',
     'quest struct access: .bIsQuestCompleted -> .is_quest_completed'),
    (r'\.sQuestType\b', '.quest_type',
     'quest struct access: .sQuestType -> .quest_type'),
    (r'\.sContribution\b', '.contribution',
     'quest struct access: .sContribution -> .contribution'),
    (r'\.sTargetType\b', '.target_type',
     'quest struct access: .sTargetType -> .target_type'),
    (r'\.sTargetCount\b', '.target_count',
     'quest struct access: .sTargetCount -> .target_count'),
    (r'\.sCurrentCount\b', '.current_count',
     'quest struct access: .sCurrentCount -> .current_count'),
    (r'\.cTargetName\b', '.target_name',
     'quest struct access: .cTargetName -> .target_name'),
    (r'\.sWho\b', '.who',
     'quest struct access: .sWho -> .who'),
    # Short fields sX, sY, sRange -- scoped to m_stQuest context only
    # (these are too common in other structs to use bare \.sX)
    (r'(m_stQuest\.)sX\b', r'\1x',
     'quest struct access: m_stQuest.sX -> m_stQuest.x'),
    (r'(m_stQuest\.)sY\b', r'\1y',
     'quest struct access: m_stQuest.sY -> m_stQuest.y'),
    (r'(m_stQuest\.)sRange\b', r'\1range',
     'quest struct access: m_stQuest.sRange -> m_stQuest.range'),

    # =====================================================================
    # Guild struct fields -- DOT-PREFIXED access patterns
    # These MUST be applied BEFORE renaming m_stGuildName
    # =====================================================================
    (r'\.dwRefTime\b', '.ref_time',
     'guild struct access: .dwRefTime -> .ref_time'),
    (r'\.iGuildRank\b', '.guild_rank',
     'guild struct access: .iGuildRank -> .guild_rank'),
    (r'\.cCharName\b', '.char_name',
     'guild struct access: .cCharName -> .char_name'),
    (r'\.cGuildName\b', '.guild_name',
     'guild struct access: .cGuildName -> .guild_name'),

    # =====================================================================
    # CGame struct member renames (AFTER field renames above)
    # =====================================================================
    (r'\bm_stQuest\b', 'm_quest',
     'CGame member: m_stQuest -> m_quest'),
    (r'\bm_stGuildName\b', 'm_guild_name_cache',
     'CGame member: m_stGuildName -> m_guild_name_cache'),

    # =====================================================================
    # Shared member: m_pGame (used by many manager classes)
    # =====================================================================
    (r'\bm_pGame\b', 'm_game',
     'member: m_pGame -> m_game (shared across managers/dialogs)'),

    # =====================================================================
    # Parameter renames: pGame, pData
    # These are ubiquitous parameter names across all NetworkMessage handlers
    # =====================================================================
    (r'\bpGame\b', 'game',
     'param: pGame -> game'),
    (r'\bpData\b', 'data',
     'param: pData -> data'),

    # =====================================================================
    # Local variable renames -- UNIQUE to assigned files
    # These only appear in GuildManager.cpp or QuestManager.cpp
    # =====================================================================
    (r'\bcLocation\b', 'location',
     'local/param: cLocation -> location (GuildManager only)'),
    (r'\bsRank\b', 'rank',
     'local: sRank -> rank (GuildManager only)'),
    (r'\bsFlag\b', 'flag',
     'local: sFlag -> flag (QuestManager only)'),
    (r'\biPreCon\b', 'pre_contribution',
     'local: iPreCon -> pre_contribution (QuestManager only)'),
    (r'\bcRewardName\b', 'reward_name',
     'local: cRewardName -> reward_name (QuestManager only)'),

    # =====================================================================
    # Local variable renames -- BROAD identifiers (appear in many files)
    # These are all local variables/params that need snake_case per standard.
    # Renaming globally is safe (idempotent, no name conflicts).
    # =====================================================================
    (r'\bsWho\b', 'who',
     'local: sWho -> who (quest contexts)'),
    (r'\biAmount\b', 'amount',
     'local: iAmount -> amount'),
    (r'\biIndex\b', 'index',
     'local: iIndex -> index'),
    (r'\bcTxt\b', 'text',
     'local: cTxt -> text'),
    (r'\bcName\b', 'name',
     'local: cName -> name'),
    (r'\bcTemp\b', 'temp',
     'local: cTemp -> temp'),
    (r'\bsV1\b', 'value1',
     'local: sV1 -> value1'),
    (r'\bsV2\b', 'value2',
     'local: sV2 -> value2'),
]

files = get_source_files()
total_changes = 0
for f in files:
    with open(f, 'r', encoding='utf-8', errors='replace') as fh:
        content = fh.read()
    original = content
    for pattern, replacement, desc in REPLACEMENTS:
        content = re.sub(pattern, replacement, content)
    if content != original:
        with open(f, 'w', encoding='utf-8', newline='') as fh:
            fh.write(content)
        total_changes += 1
        print(f'  Updated: {os.path.basename(f)}')
print(f'\nAgent 4 done. {total_changes} file(s) updated, {len(REPLACEMENTS)} replacements applied.')

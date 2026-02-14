"""Phase 3: Server struct member renames (~150 violations across ~20 files).

Two categories:
  SAFE_RENAMES   – distinctive camelCase names, word-boundary replace everywhere
  CONTEXT_RENAMES – short Hungarian names that collide with common parameters,
                    only replaced when preceded by '.' (struct member access)
                    PLUS their declarations in the header files

Files processed: all .h and .cpp under Sources/Server/
"""

import re, os, sys

ROOT = os.path.join(os.path.dirname(__file__), "..", "Sources", "Server")

# ── Distinctive names: safe for global word-boundary replacement ─────────────
SAFE_RENAMES = [
    # AccountSqliteStore.h – AccountDbAccountData
    ("passwordChangedAt",       "password_changed_at"),
    ("createdAt",               "created_at"),
    ("lastIp",                  "last_ip"),

    # AccountSqliteStore.h – AccountDbCharacterData / State / Summary
    ("accountName",             "account_name"),
    ("characterName",           "character_name"),
    ("mapName",                 "map_name"),
    ("hairStyle",               "hair_style"),
    ("hairColor",               "hair_color"),
    ("mapX",                    "map_x"),
    ("mapY",                    "map_y"),

    # AccountSqliteStore.h – AccountDbCharacterState (long names first)
    ("fightzoneTicketNumber",   "fightzone_ticket_number"),
    ("timeleftForceRecall",     "timeleft_force_recall"),
    ("timeleftFirmStaminar",    "timeleft_firm_stamina"),
    ("gizonItemUpgradeLeft",    "gizon_item_upgrade_left"),
    ("specialAbilityTime",      "special_ability_time"),
    ("questRewardAmount",       "quest_reward_amount"),
    ("currentQuestCount",       "current_quest_count"),
    ("penaltyBlockMonth",       "penalty_block_month"),
    ("penaltyBlockYear",        "penalty_block_year"),
    ("penaltyBlockDay",         "penalty_block_day"),
    ("deadPenaltyTime",         "dead_penalty_time"),
    ("questRewardType",         "quest_reward_type"),
    ("fightzoneNumber",         "fightzone_number"),
    ("questCompleted",          "quest_completed"),
    ("specialEventId",          "special_event_id"),
    ("superAttackLeft",         "super_attack_left"),
    ("timeleftRating",          "timeleft_rating"),
    ("enemyKillCount",          "enemy_kill_count"),
    ("warContribution",         "war_contribution"),
    ("constructPoint",          "construct_point"),
    ("downSkillIndex",          "down_skill_index"),
    ("hungerStatus",            "hunger_status"),
    ("lockedMapName",           "locked_map_name"),
    ("lockedMapTime",           "locked_map_time"),
    ("questNumber",             "quest_number"),
    ("reserveTime",             "reserve_time"),
    ("crusadeGuid",             "crusade_guid"),
    ("rewardGold",              "reward_gold"),
    ("crusadeJob",              "crusade_job"),
    ("guildName",               "guild_name"),
    ("guildGuid",               "guild_guid"),
    ("guildRank",               "guild_rank"),
    ("questId",                 "quest_id"),
    ("partyId",                 "party_id"),
    ("pkCount",                 "pk_count"),
    ("luPool",                  "lu_pool"),
    ("idnum1",                  "id_num1"),
    ("idnum2",                  "id_num2"),
    ("idnum3",                  "id_num3"),

    # AccountSqliteStore.h – AccountDbItemRow / BankItemRow / EquippedItem
    ("touchEffectValue1",       "touch_effect_value1"),
    ("touchEffectValue2",       "touch_effect_value2"),
    ("touchEffectValue3",       "touch_effect_value3"),
    ("touchEffectType",         "touch_effect_type"),
    ("specEffectValue1",        "spec_effect_value1"),
    ("specEffectValue2",        "spec_effect_value2"),
    ("specEffectValue3",        "spec_effect_value3"),
    ("curLifeSpan",             "cur_life_span"),
    ("isEquipped",              "is_equipped"),
    ("itemColor",               "item_color"),
    ("itemId",                  "item_id"),
    ("posX",                    "pos_x"),
    ("posY",                    "pos_y"),

    # Game.h – DropEntry / DropTable
    ("totalWeight",             "total_weight"),
    ("minCount",                "min_count"),
    ("maxCount",                "max_count"),

    # Game.h – NpcShopMapping / ShopData
    ("npcType",                 "npc_type"),
    ("shopId",                  "shop_id"),
    ("itemIds",                 "item_ids"),

    # Game.h – CommandPermission
    ("iAdminLevel",             "admin_level"),
    ("sDescription",            "description"),

    # Game.h – LoginClient (underscore-prefixed members)
    ("_timeout_tm",             "timeout_tm"),
    ("_sock",                   "sock"),
    ("_ip",                     "ip"),

    # Map.h – CMap class members (PascalCase)
    ("m_WaypointList",          "m_waypoint_list"),
    ("m_FishPointList",         "m_fish_point_list"),
    ("m_MineralPointList",      "m_mineral_point_list"),

    # Map.h – anonymous struct members (distinctive Hungarian)
    ("iTotalActiveMob",         "total_active_mob"),
    ("iNpcConfigId",            "npc_config_id"),
    ("iMaxMobs",                "max_mobs"),
    ("iCurMobs",                "cur_mobs"),
    ("iProbSA",                 "prob_sa"),
    ("iKindSA",                 "kind_sa"),
    ("iPlayerActivity",         "player_activity"),
    ("iNeutralActivity",        "neutral_activity"),
    ("iAresdenActivity",        "aresden_activity"),
    ("iElvineActivity",         "elvine_activity"),
    ("iMonsterActivity",        "monster_activity"),
    ("sMobEventAmount",         "m_mob_event_amount"),  # class member, needs m_
    ("cRelatedMapName",         "related_map_name"),
    ("iMapIndex",               "map_index"),
    ("iInitHP",                 "init_hp"),
    ("iEffectX",                "effect_x"),
    ("iEffectY",                "effect_y"),
    ("cItemName",               "item_name"),
    ("iTotalNum",               "total_num"),
    ("aresdenX",                "aresden_x"),
    ("aresdenY",                "aresden_y"),
    ("elvineX",                 "elvine_x"),
    ("elvineY",                 "elvine_y"),
    ("sTypeID",                 "type_id"),
    ("bDefined",                "is_defined"),

    # Client.h (server) – CClient class members (missing m_ or wrong naming)
    ("totalItemRepair",         "total_item_repair"),
    ("isForceSet",              "is_force_set"),
    ("iExchangeCount",          "exchange_count"),
]

# ── Ambiguous short names: only replace when accessed via '.' ─────────────────
# (These are anonymous-struct members in Map.h & Client.h that share names with
#  common parameters like cType, sX, sY, dX, dY, cDir, etc.)
CONTEXT_RENAMES = [
    ("cType",       "type"),
    ("cWaypoint",   "waypoints"),
    ("cResult",     "result"),
    ("cDir",        "dir"),
    ("cSide",       "side"),
    ("iAmount",     "amount"),
    ("iTotal",      "total"),
    ("iMonth",      "month"),
    ("iDay",        "day"),
    ("iHP",         "hp"),
    ("iIndex",      "index"),
    ("cName",       "name"),
    # sX/sY/dX/dY all become x/y
    ("sX",          "x"),
    ("sY",          "y"),
    ("dX",          "x"),
    ("dY",          "y"),
]

# ── Sort both lists by old-name length descending (prevent substring clobber) ─
SAFE_RENAMES.sort(key=lambda t: len(t[0]), reverse=True)
CONTEXT_RENAMES.sort(key=lambda t: len(t[0]), reverse=True)

# De-duplicate safe renames (iNpcConfigId appeared twice in manual list)
seen = set()
deduped = []
for old, new in SAFE_RENAMES:
    if old not in seen:
        seen.add(old)
        deduped.append((old, new))
SAFE_RENAMES = deduped

def should_skip_line(line):
    """Skip preprocessor directives that shouldn't be modified."""
    stripped = line.lstrip()
    return stripped.startswith("#include") or stripped.startswith("#pragma")

def apply_safe_renames(line):
    """Apply word-boundary renames for distinctive names."""
    if should_skip_line(line):
        return line
    for old, new in SAFE_RENAMES:
        line = re.sub(r'\b' + re.escape(old) + r'\b', new, line)
    return line

def apply_context_renames(line):
    """Apply dot-access-only renames for ambiguous short names."""
    if should_skip_line(line):
        return line
    for old, new in CONTEXT_RENAMES:
        # Replace .oldName (dot access)
        line = re.sub(r'\.' + re.escape(old) + r'\b', '.' + new, line)
        # Replace ->oldName (arrow access, just in case)
        line = re.sub(r'->' + re.escape(old) + r'\b', '->' + new, line)
    return line

def process_file(filepath):
    """Process a single file, applying both rename categories."""
    with open(filepath, "r", encoding="utf-8", errors="replace") as f:
        original = f.read()

    lines = original.split("\n")
    new_lines = []
    for line in lines:
        line = apply_safe_renames(line)
        line = apply_context_renames(line)
        new_lines.append(line)

    result = "\n".join(new_lines)
    if result != original:
        with open(filepath, "w", encoding="utf-8", newline="\n") as f:
            f.write(result)
        return True
    return False

def main():
    # Collect all .h and .cpp files in Sources/Server/
    files = []
    for fname in os.listdir(ROOT):
        if fname.endswith((".h", ".cpp")):
            files.append(os.path.join(ROOT, fname))
    files.sort()

    total_changed = 0
    for fpath in files:
        if process_file(fpath):
            total_changed += 1
            print(f"  Modified: {os.path.basename(fpath)}")

    print(f"\nDone. Modified {total_changed} of {len(files)} files.")

if __name__ == "__main__":
    main()

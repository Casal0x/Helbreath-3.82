#!/usr/bin/env python3
"""Agent 3: CraftingManager + FishingManager + ItemNameFormatter snake_case conversion.

Renames methods, members, parameters, and locals defined in:
  - CraftingManager.h / CraftingManager.cpp
  - FishingManager.h / FishingManager.cpp
  - ItemNameFormatter.h / ItemNameFormatter.cpp

Also renames identifiers used across many client files that originate from these classes
(e.g. ItemNameFormatter::Get().Format() call sites, SetGame shared method, etc.).
"""
import glob, os, re

REPLACEMENTS = [
    # =====================================================================
    # CraftingManager methods (unique names, used in CraftingManager + NetworkMessages_Crafting + NetworkMessageManager)
    # =====================================================================
    (r'\bHandleCraftingSuccess\b', 'handle_crafting_success', 'CraftingManager::HandleCraftingSuccess -> handle_crafting_success'),
    (r'\bHandleCraftingFail\b', 'handle_crafting_fail', 'CraftingManager::HandleCraftingFail -> handle_crafting_fail'),
    (r'\bHandleBuildItemSuccess\b', 'handle_build_item_success', 'CraftingManager::HandleBuildItemSuccess -> handle_build_item_success'),
    (r'\bHandleBuildItemFail\b', 'handle_build_item_fail', 'CraftingManager::HandleBuildItemFail -> handle_build_item_fail'),
    (r'\bHandlePortionSuccess\b', 'handle_portion_success', 'CraftingManager::HandlePortionSuccess -> handle_portion_success'),
    (r'\bHandlePortionFail\b', 'handle_portion_fail', 'CraftingManager::HandlePortionFail -> handle_portion_fail'),
    (r'\bHandleLowPortionSkill\b', 'handle_low_portion_skill', 'CraftingManager::HandleLowPortionSkill -> handle_low_portion_skill'),
    (r'\bHandleNoMatchingPortion\b', 'handle_no_matching_portion', 'CraftingManager::HandleNoMatchingPortion -> handle_no_matching_portion'),

    # =====================================================================
    # FishingManager methods (unique names, used in FishingManager + NetworkMessages_Fish + NetworkMessageManager)
    # =====================================================================
    (r'\bHandleFishChance\b', 'handle_fish_chance', 'FishingManager::HandleFishChance -> handle_fish_chance'),
    (r'\bHandleEventFishMode\b', 'handle_event_fish_mode', 'FishingManager::HandleEventFishMode -> handle_event_fish_mode'),
    (r'\bHandleFishCanceled\b', 'handle_fish_canceled', 'FishingManager::HandleFishCanceled -> handle_fish_canceled'),
    (r'\bHandleFishSuccess\b', 'handle_fish_success', 'FishingManager::HandleFishSuccess -> handle_fish_success'),
    (r'\bHandleFishFail\b', 'handle_fish_fail', 'FishingManager::HandleFishFail -> handle_fish_fail'),

    # =====================================================================
    # ItemNameFormatter methods
    # =====================================================================
    (r'\bSetItemConfigs\b', 'set_item_configs', 'ItemNameFormatter::SetItemConfigs -> set_item_configs'),
    (r'\bGetConfig\b', 'get_config', 'ItemNameFormatter::GetConfig -> get_config (private, only in ItemNameFormatter)'),
    # Format -> format: only used as ItemNameFormatter method (verified via grep)
    (r'\bFormat\b', 'format', 'ItemNameFormatter::Format -> format (all occurrences are ItemNameFormatter)'),

    # ItemNameFormatter::Get() singleton accessor -> get()
    # Use lookbehind to specifically target ItemNameFormatter::Get
    (r'(?<=ItemNameFormatter::)Get(?=\(\))', 'get', 'ItemNameFormatter::Get() -> get() singleton accessor'),

    # =====================================================================
    # Shared method across all manager classes (SetGame)
    # All managers use identical signature: void SetGame(CGame* pGame)
    # =====================================================================
    (r'\bSetGame\b', 'set_game', 'All managers: SetGame -> set_game'),

    # =====================================================================
    # Member variables
    # =====================================================================
    # m_pGame -> m_game (shared across CraftingManager, FishingManager, and many other managers)
    (r'\bm_pGame\b', 'm_game', 'm_pGame -> m_game (remove pointer prefix)'),

    # =====================================================================
    # Parameters - widely used across handler functions
    # =====================================================================
    # pData -> data (used in all network message handlers)
    (r'\bpData\b', 'data', 'param pData -> data (network handler parameter)'),
    # pGame -> game (used in NetworkMessageHandlers namespace wrapper functions)
    (r'\bpGame\b', 'game', 'param pGame -> game (handler wrapper parameter)'),
    # pItem -> item (used in ItemNameFormatter::Format and many dialog boxes)
    (r'\bpItem\b', 'item', 'param pItem -> item (item pointer parameter)'),
    # pCfg -> config (used in ItemNameFormatter and dialog boxes)
    (r'\bpCfg\b', 'config', 'local pCfg -> config (item config pointer)'),

    # =====================================================================
    # Local variables in ItemNameFormatter.cpp (unique to this file, verified via grep)
    # =====================================================================
    (r'\biManaSaveValue\b', 'mana_save_value', 'local iManaSaveValue -> mana_save_value (ItemNameFormatter only)'),
    (r'\bexistingPlus\b', 'existing_plus', 'local existingPlus -> existing_plus (ItemNameFormatter only)'),
    (r'\bplusPos\b', 'plus_pos', 'local plusPos -> plus_pos (ItemNameFormatter only)'),
    # dwType1/dwType2/dwValue1/dwValue2/dwValue3 - only in ItemNameFormatter.cpp
    (r'\bdwType1\b', 'type1', 'local dwType1 -> type1 (ItemNameFormatter only)'),
    (r'\bdwType2\b', 'type2', 'local dwType2 -> type2 (ItemNameFormatter only)'),
    (r'\bdwValue1\b', 'value1', 'local dwValue1 -> value1 (ItemNameFormatter only)'),
    (r'\bdwValue2\b', 'value2', 'local dwValue2 -> value2 (ItemNameFormatter only)'),
    (r'\bdwValue3\b', 'value3', 'local dwValue3 -> value3 (ItemNameFormatter only)'),

    # =====================================================================
    # Parameters in ItemNameFormatter method signatures
    # =====================================================================
    # iItemID -> item_id (used in ItemNameFormatter::GetConfig and CGame::GetItemConfig)
    (r'\biItemID\b', 'item_id', 'param iItemID -> item_id (ItemNameFormatter + Game)'),
    # sItemId -> item_id (used in ItemNameFormatter::Format overload)
    (r'\bsItemId\b', 'item_id', 'param sItemId -> item_id (ItemNameFormatter)'),
    # dwAttribute -> attribute (parameter in ItemNameFormatter + NetworkMessages)
    (r'\bdwAttribute\b', 'attribute', 'param/local dwAttribute -> attribute'),

    # =====================================================================
    # Local variables in FishingManager.cpp
    # =====================================================================
    (r'\biFishChance\b', 'fish_chance', 'local iFishChance -> fish_chance (FishingManager only)'),
    # sSpriteFrame MUST come before sSprite to avoid partial match
    (r'\bsSpriteFrame\b', 'sprite_frame', 'local sSpriteFrame -> sprite_frame'),
    # sSprite as local variable (FishingManager + NetworkMessages_Items + NetworkMessages_Bank)
    (r'\bsSprite\b', 'sprite', 'local sSprite -> sprite'),
    # wPrice as local variable (FishingManager only)
    (r'\bwPrice\b', 'price', 'local wPrice -> price (FishingManager only)'),

    # =====================================================================
    # Local variables - itemInfo camelCase locals used with ItemNameFormatter results
    # These are all auto-typed locals holding ItemNameInfo return values.
    # Rename longer suffixed versions FIRST to avoid partial matches.
    # =====================================================================
    (r'\bitemInfo9\b', 'item_info9', 'local itemInfo9 -> item_info9'),
    (r'\bitemInfo8\b', 'item_info8', 'local itemInfo8 -> item_info8'),
    (r'\bitemInfo7\b', 'item_info7', 'local itemInfo7 -> item_info7'),
    (r'\bitemInfo6\b', 'item_info6', 'local itemInfo6 -> item_info6'),
    (r'\bitemInfo5\b', 'item_info5', 'local itemInfo5 -> item_info5'),
    (r'\bitemInfo4\b', 'item_info4', 'local itemInfo4 -> item_info4'),
    (r'\bitemInfo3\b', 'item_info3', 'local itemInfo3 -> item_info3'),
    (r'\bitemInfo2\b', 'item_info2', 'local itemInfo2 -> item_info2'),
    (r'\bitemInfo\b', 'item_info', 'local itemInfo -> item_info'),
]

# Only process .h and .cpp files (skip .bak_* files)
all_files = sorted(
    glob.glob('Z:/Helbreath-3.82/Sources/Client/*.h') +
    glob.glob('Z:/Helbreath-3.82/Sources/Client/*.cpp')
)
files = [f for f in all_files if '.bak_' not in f]

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
print(f'\nAgent 3 done. {total_changes} file(s) updated, {len(REPLACEMENTS)} replacements applied.')

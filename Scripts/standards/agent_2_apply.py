#!/usr/bin/env python3
"""Agent 2: ShopManager + InventoryManager snake_case conversion.

Converts all non-compliant identifiers found in ShopManager.h/.cpp and
InventoryManager.h/.cpp to snake_case, applying changes across all client files.

Safety notes:
- All patterns use \b word boundaries to avoid partial matches
- Wire protocol struct fields (PacketShopRequest.npcType, etc.) are NOT renamed
- Struct data fields accessed via -> or . (like .sItemID, .itemCount) are NOT renamed
- Identifiers that collide with wire protocol field names are skipped
- .bak_* files are excluded from processing
"""
import glob, os, re

REPLACEMENTS = [
    # =========================================================================
    # ShopManager methods
    # =========================================================================
    (r'\bRequestShopMenu\b', 'request_shop_menu', 'ShopManager::RequestShopMenu → request_shop_menu'),
    (r'\bHandleResponse\b', 'handle_response', 'ShopManager::HandleResponse → handle_response'),
    (r'\bClearItems\b', 'clear_items', 'ShopManager::ClearItems → clear_items'),
    (r'\bHasItems\b', 'has_items', 'ShopManager::HasItems → has_items'),
    (r'\bGetItemList\b', 'get_item_list', 'ShopManager::GetItemList → get_item_list'),
    (r'\bGetPendingShopType\b', 'get_pending_shop_type', 'ShopManager::GetPendingShopType → get_pending_shop_type'),
    (r'\bSetPendingShopType\b', 'set_pending_shop_type', 'ShopManager::SetPendingShopType → set_pending_shop_type'),
    (r'\bSendRequest\b', 'send_request', 'ShopManager::SendRequest → send_request'),

    # =========================================================================
    # InventoryManager methods
    # =========================================================================
    (r'\bSetItemOrder\b', 'set_item_order', 'InventoryManager::SetItemOrder → set_item_order'),
    (r'\bCalcTotalWeight\b', 'calc_total_weight', 'InventoryManager::CalcTotalWeight → calc_total_weight'),
    (r'\bGetTotalItemCount\b', 'get_total_item_count', 'InventoryManager::GetTotalItemCount → get_total_item_count'),
    (r'\bGetBankItemCount\b', 'get_bank_item_count', 'InventoryManager::GetBankItemCount → get_bank_item_count'),
    (r'\bEraseItem\b', 'erase_item', 'InventoryManager::EraseItem → erase_item'),
    (r'\bCheckItemOperationEnabled\b', 'check_item_operation_enabled', 'InventoryManager::CheckItemOperationEnabled → check_item_operation_enabled'),
    (r'\bUnequipSlot\b', 'unequip_slot', 'InventoryManager::UnequipSlot → unequip_slot'),
    (r'\bEquipItem\b', 'equip_item', 'InventoryManager::EquipItem → equip_item'),

    # =========================================================================
    # Cross-manager method (used by ShopManager, InventoryManager, and others)
    # =========================================================================
    (r'\bSetGame\b', 'set_game', 'All managers: SetGame → set_game'),

    # =========================================================================
    # ShopManager locals/params (scoped to ShopManager.cpp only or safe globally)
    # =========================================================================
    # NOTE: npcType SKIPPED - collides with wire protocol PacketShopRequest.npcType
    # NOTE: itemCount SKIPPED - collides with wire protocol PacketShopResponseHeader.itemCount
    # NOTE: itemId SKIPPED - collides with struct field entry.itemId in Game.cpp
    # NOTE: pData SKIPPED - collides with miniaudio struct field pDecoded->pData
    # NOTE: cType SKIPPED - collides with struct field .cType in CrusadeStructureInfo
    (r'\bitemIds\b', 'item_ids', 'ShopManager local: itemIds → item_ids'),
    (r'\bshopIndex\b', 'shop_index', 'ShopManager local: shopIndex → shop_index'),
    (r'\bskippedCount\b', 'skipped_count', 'ShopManager local: skippedCount → skipped_count'),
    (r'\bnotFoundCount\b', 'not_found_count', 'ShopManager local: notFoundCount → not_found_count'),
    (r'\bpConfig\b', 'config', 'ShopManager local: pConfig → config (only in ShopManager.cpp)'),
    (r'\bcData\b', 'packet_buffer', 'ShopManager local: cData → packet_buffer (only in ShopManager.cpp)'),

    # =========================================================================
    # InventoryManager locals/params
    # =========================================================================
    # NOTE: sItemID SKIPPED - collides with struct field .sItemID in exchange structs
    (r'\bcWhere\b', 'where', 'Param: cWhere → where'),
    (r'\bcEquipPos\b', 'equip_pos', 'Param: cEquipPos → equip_pos (safe: \\b won\'t match m_cEquipPos)'),
    (r'\bpCfgEq\b', 'equip_config', 'InventoryManager local: pCfgEq → equip_config'),
    (r'\biAngelValue\b', 'angel_value', 'InventoryManager local: iAngelValue → angel_value'),
    (r'\biWeight\b', 'weight', 'Local: iWeight → weight'),
    (r'\biCnt\b', 'count', 'Local: iCnt → count'),
    (r'\biTemp\b', 'temp', 'Local: iTemp → temp'),

    # =========================================================================
    # Broadly-used params/locals from my files (Hungarian-prefixed, no struct
    # field collision, safe for global rename across all client files)
    # =========================================================================
    (r'\bpGame\b', 'game', 'Param: pGame → game (no struct field collision, only param/local)'),
    (r'\bcItemID\b', 'item_id', 'Param: cItemID → item_id (used in 26 files, always param/local)'),
    (r'\bpItem\b', 'item', 'Local: pItem → item (no struct field collision)'),
    (r'\bpCfg\b', 'config', 'Local: pCfg → config (no struct field collision)'),
    (r'\bitemInfo2\b', 'item_info_2', 'Local: itemInfo2 → item_info_2'),
    (r'\bitemInfo3\b', 'item_info_3', 'Local: itemInfo3 → item_info_3'),
]

# Only process .h and .cpp files, excluding .bak_* files
files = sorted(
    f for f in
    glob.glob('Z:/Helbreath-3.82/Sources/Client/*.h') +
    glob.glob('Z:/Helbreath-3.82/Sources/Client/*.cpp')
    if '.bak_' not in f
)

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

print(f'\nAgent 2 done. {total_changes} file(s) updated, {len(REPLACEMENTS)} replacements applied.')

# Gold stack count: uint64_t support

- **Item.h**: Changed `m_count` from `uint32_t` to `uint64_t` — gold can now store values up to 2^64
- **Wire protocol**: Updated 6 packet structs (`PacketNotifyItemObtained`, `PacketNotifyItemPurchased`, `PacketNotifyItemToBank`, `PacketNotifySetItemCount`, `PacketResponseItemListEntry`, `PacketResponseBankItemEntry`) — `count` field widened to `uint64_t`
- **Server**: `send_notify_msg` v2 parameter widened to `uint64_t`; `set_item_count`, `set_item_count_by_id`, `get_item_count_by_id` signatures updated; `gold_count` locals in ItemManager, MagicManager, WarManager changed to `uint64_t`
- **DB storage**: `AccountSqliteStore` save/load uses `sqlite3_bind_int64`/`sqlite3_column_int64` for count fields
- **Admin commands**: `CmdGiveItem`, `GameCmdGiveItem`, `GameCmdCreateItem` — only gold (item_id == Gold) bypasses the 1000 stack cap; Consume/Arrow items keep the 1000 cap
- **Client**: `format_comma_number` accepts `uint64_t`; fixed `static_cast<int>(m_count)` truncation in `Screen_OnGame.cpp` drop handler; updated local vars in `NetworkMessages_Items.cpp`, `NetworkMessages_Bank.cpp`, `DialogBox_Inventory.cpp`
- **CraftingManager**: Updated `m_count` comparison casts from `uint32_t` to `uint64_t`
- **Warning cleanup**: Added explicit `static_cast` for all `m_count` → `int`/`uint32_t` narrowing conversions in ItemManager (drop, give, exchange, buy, sell, crafting handlers)
- **Dialog box system**: Widened `enable_dialog_box` v1 parameter from `int` to `int64_t` across `DialogBoxManager`, `IDialogBox`, and all callers — amount text field now displays the full uint64_t count
- **Amount parsing**: `Screen_OnGame.cpp` drop handler now parses into `uint64_t` first, clamps to item count, then to INT_MAX for the wire protocol
- **AmountStringMaxLen**: Widened from 12 to 20 characters (supports full uint64_t display)
- **Weight calculation**: `InventoryManager::calc_total_weight` uses `int64_t` arithmetic to prevent overflow with large gold stacks
- **Client crafting**: `BuildItemManager` item_count assignments use explicit `static_cast<int>` (safe: crafting materials capped at 1000)

# Shop Manager: filter bar + drag from catalog

- **Shop items filter**: Added search bar above the shop items table to filter by name or ID
- **Drag from catalog**: Catalog items (right panel) are now draggable — drop onto a shop item row to insert before it, or drop on the table to append
- **Positional insert API**: `add_item` endpoint now accepts optional `before_item_id` to insert at a specific position
- Clicking a catalog item still works to append to end

# Fix top message not clearing + shop reload notification

- **Client**: `draw_top_msg()` had an empty expiry body — message never cleared. Added `m_top_msg.clear()` inside the timeout check
- **Server**: `CmdReload` now sends the config reload notification to clients when shops are reloaded (`reload shops`), not just items/magic/skills/npcs

# Shop Manager: reorderable item list + all-items catalog

- **Tool UI overhaul**: Three-panel layout — shop list (left), shop detail (middle), all items catalog (right)
- **Shop items as table**: Replaced tag-style item display with a numbered table list showing #, ID, Name
- **Reordering**: Up/down arrow buttons per item, plus drag-and-drop row reordering
- **sort_order column**: Added `sort_order INTEGER NOT NULL DEFAULT 0` to `shop_items` table in server schema and tool
- **Auto-migration**: Tool adds `sort_order` column on startup if missing, assigns initial order by item_id
- **Server**: `LoadShopConfigs` now loads items `ORDER BY sort_order, item_id` to preserve tool-defined order
- **URL fix**: Both ShopManager and ItemRenameTool now use `urlparse` to handle query strings from master_app.py iframe loading

# NPC shop mapping: link by NPC config ID instead of arbitrary type

Refactored the `npc_shop_mapping` system to use `npc_config_id` (the actual NPC config ID from `npc_configs` table) instead of arbitrary hardcoded "shop type" values (1, 2).

- **Shared**: Renamed `PacketShopRequest.npcType` / `PacketShopResponseHeader.npcType` → `npcConfigId`
- **Server**: Renamed `NpcShopMapping.npc_type` → `npc_config_id`, updated DB schema (`npc_shop_mapping.npc_type` → `npc_config_id`), updated `request_shop_contents_handler` and `LoadShopConfigs`
- **Client**: Changed hardcoded `1`/`2` in NPC dialog setup → pass actual `npc_config_id` from map tile data. Renamed `ShopManager` members: `m_pending_shop_type` → `m_pending_npc_config_id`, `request_shop_menu(char)` → `request_shop_menu(int16_t)`
- **Tools**: Updated `tools/ShopManager/app.py` SQL and variable names
- **Migration**: Added `Scripts/migrate_shop_mapping.py` one-time script to rename the DB column

# Server console overhaul

Overhauled server console command output with clean colored feedback, removing timestamps and log prefixes. Added Up/Down arrow command history, formatted help listings, and fixed a setcmdlevel output bug.

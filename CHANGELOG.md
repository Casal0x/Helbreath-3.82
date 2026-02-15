# NPC name lookup cleanup

Removed type-based NPC name lookup in favor of config_id-only lookup. NPC dialog boxes, item drop confirmations, quest displays, and nameplate rendering now all resolve NPC names by their unique config ID instead of the ambiguous type index. Server quest system changed to store and match target NPCs by config_id rather than type, fixing cases where multiple NPC configs sharing a type would show the wrong name.

## Bug fix: Hovered NPC shows "Unknown" name

- **Root cause**: CursorTarget focus highlight pipeline was missing `m_npc_config_id` — when an NPC was hovered/focused and redrawn with transparency highlight, the entity state's config_id remained at -1 (stale from reset), causing `draw_npc_name` to return "Unknown"
- Added `m_npc_config_id` to `FocusedObject` and `TargetObjectInfo` structs (CursorTarget.h)
- Propagated config_id through `test_object()` and `get_focus_highlight_data()` (CursorTarget.cpp)
- Set `m_entity_state.m_npc_config_id` from focus highlight data in Game.DrawObjects.cpp
- Populated `info.m_npc_config_id` for both dead body and live body TargetObjectInfo construction
- Removed all debug logging from Game.cpp (draw_npc_name, read_map_data, motion_event) and MapData.cpp (set_owner WROTE/CONFLICT)

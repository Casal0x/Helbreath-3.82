# Changelog

## Server 0.1.15 / Client 0.2.18 — goto Elimination

### Refactoring
- **Client**: Removed all 21 goto statements across BuildItemManager.cpp and MapData.cpp
  - BuildItemManager: 6 copy-pasted element blocks replaced with `for (elem = 1..6)` loops (2 functions)
  - MapData `set_chat_msg_owner`: Inverted condition replaces forward goto
  - MapData `set_object_data`: `bool found_old` flag replaces 8 goto statements in nested search loops
- **Server**: Removed all 199 goto statements across 12 files
  - MagicManager.cpp: 121 `goto MAGIC_NOEFFECT` replaced with lambda + return pattern
  - EntityManager.cpp: 33 gotos replaced (flags, `break`, condition inversion for NPC AI/magic)
  - Game.cpp: 18 gotos replaced (`break` for loop exits, `bool` flags for multi-path convergence)
  - CombatManager.cpp: 9 gotos replaced (condition restructuring, `bool skip_counter`, `do/while(false)`)
  - WarManager.cpp: 5 gotos replaced (`continue`, `break`, `bool` flags)
  - ItemManager.cpp: 4 gotos replaced (`bool guild_item_handled` flag, unconditional flow, `break`)
  - CraftingManager.cpp: 2 gotos replaced with `break`
  - Misc.h: 2 gotos replaced with `break` (Bresenham line tracing)
  - GameGeometry.h: 2 gotos replaced with `break` (Bresenham line tracing)
  - LootManager.cpp, Map.cpp, PartyManager.cpp: 1 goto each replaced with `break`/flag
- **BuildItem.h**: Added `get_element_name(int index)` accessor for loop-based element access
- Zero goto statements remain in all project-owned source files (vendor libraries excluded)

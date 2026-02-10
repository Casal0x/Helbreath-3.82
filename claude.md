# Helbreath 3.82 - Project Guide

## Build
```powershell
# From Sources/ directory. Requires VS2022 Community MSBuild.
powershell -ExecutionPolicy Bypass -File Sources\build.ps1 -Target All -Config Debug
powershell -ExecutionPolicy Bypass -File Sources\build.ps1 -Target Game -Config Debug
powershell -ExecutionPolicy Bypass -File Sources\build.ps1 -Target Server -Config Debug
```
- Delete `build_*.log` files before building for clean logs.
- Output: `Sources\Debug\Game_SFML_x64.exe` (client), `Sources\Debug\Server.exe` (server).
- Platform is x64. Solution: `Sources\Helbreath.sln`. C++17 server, C++20 client.
- 78 LNK4099 warnings (missing SFML/freetype PDBs) are cosmetic - ignore them.
- MSBuild path in `build.ps1` may need updating per machine (Community vs BuildTools edition).

## Transformation Workflow: Backup-Script-Build

**This is the mandatory workflow for ALL code transformations (refactors, renames, define conversions, etc.). Never deviate.**

**IMPORTANT: Never use git commands (commit, stash, checkout, etc.). The user handles all git operations. Backups use `.bak` files.**

1. **Start from clean build** — current code compiles with 0 errors before starting.
2. **Write ONE script** in `Scripts/` that performs the complete transformation for one phase. Scripts never go in the project root.
3. **Script creates `.bak` backups**: Before modifying any file, the script copies it to `<filename>.bak` in the same directory. This is the safety net.
4. **Run the script**.
5. **Build immediately**: `powershell -ExecutionPolicy Bypass -File Sources\build.ps1 -Target All -Config Debug`
6. **If errors**: Restore all `.bak` files to their originals (overwrite the broken versions). Analyze what went wrong **in the script itself**, fix the script, re-run from step 4.
7. **Never write a second "fix" script** — cascading patches cause cascading corruption. Always fix the original script and re-run cleanly.
8. **Once 0 errors**: Delete all `.bak` files — this is your "commit". Move to next phase.

### Why This Matters
- `.bak` files let you revert without touching git — the user controls all commits.
- Fixing the script (not the output) produces **reproducible, correct transformations**.
- Restoring from `.bak` before re-running ensures each attempt starts from a known-good state.
- One script per phase keeps the change atomic and reviewable.
- Building after every script run catches problems immediately while context is fresh.

### Regex Safety Rules (Learned from Prior Failures)
- **NEVER** use `::TypeName` as a regex pattern — it matches inside prefixed names (`sf::Color` → `sfhb::shared::types::Color`).
- **NEVER** replace inside `#define` macro names — `#define DEF_X` cannot become `#define hb::shared::X`.
- **PREFER** explicit `content.replace("exact_old", "exact_new")` over regex for known patterns.
- **ALWAYS** order replacements longest-first to avoid partial matches (`DEF_OBJECTMOVE_CONFIRM` before `DEF_OBJECTMOVE`).
- **TEST** the script on 2-3 files first when dealing with regex patterns.

## Solution Structure (3 projects)
- **Client** (`Sources/Client/`) - Game client. Depends on SFMLEngine.
- **SFMLEngine** (`Sources/SFMLEngine/`) - Rendering abstraction library over SFML.
- **Server** (`Sources/Server/`) - Game server. Standalone.
- **Shared** (`Sources/Dependencies/Shared/`) - Headers shared by both (protocol, enums, item system, packets, appearance, networking).

## Runtime Layout
- `Binaries/Game/` - Client executable, configs, sprites, sounds, maps, fonts.
- `Binaries/Server/` - Server executable, SQLite databases (`GameConfigs.db`, `MapInfo.db`), account storage in `Accounts/`.

## Modernization Direction
- Legacy C-style code is being modernized to C++17/20 while preserving game logic.
- Goal: OOP, polymorphism, templates, clearly scoped systems. No spaghetti.
- When touching code, modernize it. Don't gold-plate untouched code.

---

## Client Architecture

### Entry & Main Loop (`Wmain.cpp`)
Decoupled update/render loop: `UpdateFrame()` runs every iteration (processes network, audio, game state); `RenderFrame()` is gated by frame limiter. Network polling happens in update so the game stays responsive regardless of FPS.

### Game Class (`CGame`) - God Object
Central orchestrator holding all subsystems: player, map data, effects, networking, dialog manager, camera, renderer, sprite collections, item/magic/skill configs. This is the primary target for decomposition - too many responsibilities in one class.

### Screen State Machine (`IGameScreen` + `GameModeManager`)
Template-based type-safe screen management with fade transitions. Screens: Splash, MainMenu, Login, SelectCharacter, CreateCharacter, CreateAccount, Loading, OnGame. Overlays (Connecting, WaitingResponse, Msg) display on top and block the base screen. Lifecycle: `on_initialize()` → `on_update()` → `on_render()` → `on_uninitialize()`.

### Dialog System (`IDialogBox` + `DialogBoxManager`)
61 dialog boxes registered by enum ID with z-order management. Virtual interface: `OnDraw`, `OnClick`, `OnDoubleClick`, `OnPress`, `OnItemDrop`, `OnUpdate`. Covers all UI panels: inventory, character, bank, magic, skills, shops, guild, quests, exchange, crafting.

### Networking (`NetworkMessageManager` + `ASIOSocket`)
Client uses polling mode (0 async threads). Two socket connections: game server (`m_pGSock`) and login server (`m_pLSock`). Messages routed by ID through `NetworkMessageManager::ProcessMessage()` to handler methods on CGame. Message handlers are split across ~20 `NetworkMessages_*.cpp` files by category.

### Rendering Pipeline (SFMLEngine)
Abstract interfaces (`IRenderer`, `IWindow`, `ISprite`, `IBitmapFont`) with SFML implementations. Render order: clear → map tiles → lighting effects → entities (z-ordered) → UI dialogs → particle effects → present. Sprites loaded from PAK files via `SFMLSpriteFactory` with lazy loading.

### Entity System
- `CPlayer` - Full player state (stats, inventory, appearance, skills, magic).
- `CPlayerRenderer` / `CNpcRenderer` - Separated rendering from logic.
- `AnimationState` - 10 animation types with frame timing and status modifiers.
- `CMapData` - 60x55 tile grid with entity tracking, items, chat anchors.

### Audio (`AudioManager`)
Miniaudio-based. Pre-decoded WAV sounds in 3 categories (character, monster, effect). Background music streamed. Positional audio with distance attenuation and stereo panning. Up to 32 concurrent sounds.

### Config (`ConfigManager`)
JSON-based `settings.json`. Manages shortcuts, audio, window, display settings. Singleton with file I/O.

---

## Server Architecture

### Entry & Game Loop (`Wmain.cpp`)
Hidden window + Windows console. Event loop: process Win messages, run `GameProcess()` every ~30ms (300ms / 10x multiplier), drain socket accept/error queues, poll console input.

### Game Class (`CGame`) - God Object
Even larger than client's. Manages: 2000 clients, 100 maps, 15000 NPCs, 5000 item types, 60000 dynamic objects/delay events. Handles all game logic: combat, magic, quests, guilds, crusades, crafting, fishing, mining.

### Client Connections (`CClient`)
Represents a connected player. Holds: character data, position, stats, inventory (50 items), bank (1000 items), equipped items, magic/skill mastery, guild info, party info, status effects, admin level. 30s timeout.

### Map/World (`CMap` + `CTile`)
Grid-based maps with tiles tracking: owner entity, up to 12 items per tile, dynamic objects, movement/teleport flags, water/farm flags, occupy status (Aresden/Elvine faction). Maps have: teleport locations, spawn points, waypoints, fishing/mining points, no-attack zones, crusade structures.

### Entity Management (`EntityManager` + `CNpc`)
Extracted from CGame to manage NPC lifecycle. Active entity list optimization (iterate only live NPCs). NPCs have: movement types (waypoint, random, follow, guard), behavior states (stop, move, attack, flee, dead), special abilities (invisibility, poison, explosive), AI levels, summon support.

### Database Layer (SQLite)
- `AccountSqliteStore` - Per-account `.db` files in `Accounts/`. Tables: accounts, characters, character_state, items, bank, equips, magic/skill mastery.
- `GameConfigSqliteStore` - `GameConfigs.db`. Tables: items, npcs, magic, skills, quests, drop_tables, shops, admin_config, settings.
- `MapInfoSqliteStore` - `MapInfo.db`. Per-map tables: dimensions, teleports, spawns, waypoints, restrictions, mob generators.

### Command System
- Console commands via `ServerCommand` base class + `ServerCommandManager` singleton.
- In-game `/commands` via `GameChatCommand` with admin level permissions.
- Commands: broadcast, giveitem, goto, spawn, invis, regen, come, block/unblock.

### Login Flow (`LoginServer`)
Client connects to login socket → validates credentials → returns character list → client selects character → redirected to game socket → character data loaded from SQLite → enters world.

---

## Shared Protocol (`Dependencies/Shared/`)
- `NetMessages.h` - 100+ message IDs (login, motion, events, config, notifications).
- `NetConstants.h` - Buffer sizes, name limits, view range (25x17 tiles), game limits.
- `Packet/` - 23 packet structure headers with strict byte-packing.
- `Item/` - Unified item class, enums (35 effect types, 15 equip positions, 13 item types).
- `Appearance.h` - Player/NPC visual data (body, weapons, armor, effects, colors).
- `PlayerStatusData.h` - Status flags (poison, berserk, frozen, invisibility, etc.).
- `OwnerType.h` - Entity type IDs (players 1-6, NPCs 10+, 100+ monster types).
- `ASIOSocket` - Shared networking (polling for client, async for server).

## Key Anti-Patterns to Address
1. **God Objects** - CGame on both client and server holds too much. Decompose into focused systems.
2. **Global State** - Heavy use of globals (`G_pGame`, etc.). Move toward dependency injection.
3. **Mixed C/C++** - `char[]` alongside `std::string`, `memcpy` alongside STL, `new/delete` alongside smart pointers. Standardize to modern C++.
4. **Spaghetti Message Handlers** - Network handlers mix parsing, game logic, and UI updates. Separate concerns.
5. **Magic Numbers** - Scattered constants. Use named constants or config values.
6. **Fixed-Size Arrays** - Entity lists as `T*[N]` with linear slot search. Consider containers with proper lifecycle.

## Coding Standards (Active)
Full reference: `PLANS/CODING_STANDARDS.md`. Key rules:
- **Tabs** for indentation, **Allman** braces.
- **Classes**: PascalCase, no `C` prefix. Interfaces keep `I` prefix.
- **Methods**: PascalCase, no Hungarian return-type prefixes (`GetIndex()` not `iGetIndex()`).
- **Members**: `m_snake_case` (`m_position_x`, not `m_iPositionX`).
- **Params/Locals**: snake_case (`target_id`, not `sTargetID`).
- **Constants**: `constexpr` PascalCase. No new `#define` constants.
- **Enums**: Namespace-wrapped unscoped enum with explicit type for auto-conversion. `enum class` only when implicit conversion is undesirable.
- **Namespaces**: `hb::<shared|client|server>::snake_case` (e.g., `hb::client::combat`).
- **Lifecycle hooks**: snake_case (`on_initialize()`, `on_update()`).
- **Ownership**: `std::unique_ptr` only. Raw pointers are non-owning. Prefer `&` references over pointers for access.
- **Error handling**: Exceptions for critical/unrecoverable failures. `bool` returns for recoverable errors. No `goto` ever.
- **Null**: `nullptr` only. Never `NULL` or `0`.
- **Headers**: `#pragma once`. Never `using namespace` in headers.
- **Comments**: `//` only. Comment the "why", not the "what".
- **Wire protocol structs**: Intentional exception -- Hungarian without `m_` to match binary format.
- Legacy code retains old conventions until actively refactored. Do not reformat untouched code.

## Testing
No automated tests. Manual verification: run server, then client with configs in `Binaries/`.
For network changes, rebuild both client and server.

# Architecture Reference

## Solution Structure
- **Client** (`Sources/Client/`) — Game client, C++20. Depends on SFMLEngine.
- **SFMLEngine** (`Sources/SFMLEngine/`) — Rendering abstraction over SFML.
- **Server** (`Sources/Server/`) — Game server, C++17. Standalone.
- **Shared** (`Sources/Dependencies/Shared/`) — Headers shared by both (protocol, enums, items, packets, networking).

## Runtime Layout
- `Binaries/Game/` — Client exe, configs, sprites, sounds, maps, fonts.
- `Binaries/Server/` — Server exe, SQLite DBs (`GameConfigs.db`, `MapInfo.db`), `Accounts/`.

---

## Client

### Entry & Main Loop (`Wmain.cpp`)
Decoupled update/render: `UpdateFrame()` every iteration (network, audio, game state); `RenderFrame()` gated by frame limiter.

### Game Class (`CGame`)
Central orchestrator: player, map, effects, networking, dialogs, camera, renderer, sprites, configs. Primary decomposition target — extracted managers: AudioManager, ConfigManager, GameModeManager, DialogBoxManager, WeatherManager, BuildItemManager, GuildManager, MagicCastingSystem, CombatSystem, ShopManager, InventoryManager, CrusadeManager, TeleportManager, TextInputManager, EventListManager, FloatingTextManager, EffectManager, HotkeyManager, CCamera, PlayerRenderer, NpcRenderer.

### Screen State Machine (`IGameScreen` + `GameModeManager`)
Type-safe screen management with fade transitions. Screens: Splash, MainMenu, Login, SelectCharacter, CreateCharacter, CreateAccount, Loading, OnGame. Overlays: Connecting, WaitingResponse, Msg.

### Dialog System (`IDialogBox` + `DialogBoxManager`)
61 dialog boxes by enum ID with z-order. Interface: `OnDraw`, `OnClick`, `OnDoubleClick`, `OnPress`, `OnItemDrop`, `OnUpdate`.

### Networking (`NetworkMessageManager` + `ASIOSocket`)
Polling mode, two sockets (game `m_pGSock`, login `m_pLSock`). Handlers split across ~20 `NetworkMessages_*.cpp` files.

### Rendering (SFMLEngine)
Abstract interfaces (`IRenderer`, `IWindow`, `ISprite`, `IBitmapFont`) with SFML implementations. Render order: clear → tiles → lighting → entities (z-ordered) → dialogs → particles → present. Sprites from PAK files via `SFMLSpriteFactory`.

### Entity System
- `CPlayer` — Full player state. `CPlayerRenderer`/`CNpcRenderer` — separated rendering.
- `AnimationState` — 10 animation types with frame timing.
- `CMapData` — 60x55 tile grid with entity tracking, items, chat anchors.

### Audio (`AudioManager`)
Miniaudio. Pre-decoded WAV in 3 categories. Positional audio, 32 concurrent sounds.

---

## Server

### Entry & Game Loop (`Wmain.cpp`)
Hidden window + console. `GameProcess()` every ~30ms, drain socket queues, poll console input.

### Game Class (`CGame`)
Manages: 2000 clients, 100 maps, 15000 NPCs, 5000 item types, 60000 dynamic objects/delay events. Extracted managers: FishingManager, MiningManager, CraftingManager, QuestManager, GuildManager, DelayEventManager, DynamicObjectManager, LootManager, CombatManager, ItemManager, MagicManager, SkillManager, WarManager, StatusEffectManager, EntityManager.

### Client Connections (`CClient`)
Connected player: character data, position, stats, inventory (50), bank (1000), equipped items, magic/skill mastery, guild, party, status effects, admin level. 30s timeout.

### Map/World (`CMap` + `CTile`)
Grid-based. Tiles track: owner entity, 12 items, dynamic objects, movement/teleport flags, faction occupy. Maps have: teleports, spawn points, waypoints, fishing/mining, no-attack zones, crusade structures.

### Entity Management (`EntityManager` + `CNpc`)
NPC lifecycle with active entity list optimization. Movement types (waypoint, random, follow, guard), behavior states, special abilities, AI levels, summon support.

### Database (SQLite)
- `AccountSqliteStore` — Per-account `.db` in `Accounts/`. Characters, items, bank, equips, mastery.
- `GameConfigSqliteStore` — `GameConfigs.db`. Items, NPCs, magic, skills, quests, drops, shops.
- `MapInfoSqliteStore` — `MapInfo.db`. Dimensions, teleports, spawns, waypoints, generators.

### Commands
Console: `ServerCommand` + `ServerCommandManager`. In-game: `GameChatCommand` with admin levels.

### Login (`LoginServer`)
Connect → validate → character list → select → redirect to game socket → load from SQLite → enter world.

---

## Shared Protocol (`Dependencies/Shared/`)
- `NetMessages.h` — 100+ message IDs. `NetConstants.h` — buffer sizes, limits, view range (25x17).
- `Packet/` — 23 packet headers with byte-packing.
- `Item/` — Unified item class, enums (35 effects, 15 equip positions, 13 types).
- `Appearance.h` — Player/NPC visuals. `PlayerStatusData.h` — Status flags.
- `OwnerType.h` — Entity type IDs. `ASIOSocket` — Shared networking.

## Key Anti-Patterns
1. **God Objects** — CGame on both sides. Decompose into focused systems.
2. **Global State** — `G_pGame` etc. Move toward dependency injection.
3. **Mixed C/C++** — `char[]`/`std::string`, `memcpy`/STL, `new`/smart pointers. Standardize.
4. **Spaghetti Handlers** — Network handlers mix parsing, logic, UI. Separate concerns.
5. **Magic Numbers** — Use named constants. **Fixed Arrays** — `T*[N]` with linear search, consider containers.

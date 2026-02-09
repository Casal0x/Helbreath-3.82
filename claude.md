# CLAUDE.md

Helbreath 3.82 - Classic MMORPG client-server in C++ for Windows (x64). Server v2.24b.

## Critical Rules

- **Build from solution** - Use `Helbreath.sln`, never individual project folders
- **Build early and often** - Rebuild after each logical change
- **No over-optimization** - Implement only what is requested
- **Delete refactored code** - Git has history, don't keep old code
- **Backup before bulk edits** - Create `.bak` files before mass find/replace
- **Use Python for file ops** - Prefer Python over PowerShell for text manipulation
- **No git commits** - Work only in the working tree
- **Never remove 100+ lines in one edit** - Remove large blocks in chunks

## Build Commands

```powershell
powershell.exe -ExecutionPolicy Bypass -File "C:\Users\ShadowEvil\source\Repos3\Helbreath-3.82\Sources\build.ps1" -Target Game -Config Debug
```

- `-Target`: `Game` | `Server` | `All`
- `-Config`: `Debug` | `Release`
- Renderer is SFML, platform is x64 (hardcoded)
- Outputs: `x64\Debug\Game_SFML.exe`, `x64\Debug\Server.exe`
- Logs (UTF-8): `build_game.log`, `build_server.log`, `build_all.log` in `Sources/`

## Project Structure

```
Sources/
├── Helbreath.sln              # VS 2022 solution (3 projects)
├── build.ps1                  # Build script
├── Client/                    # Game client (~250 files)
├── Server/                    # Game server (~100 files)
├── SFMLEngine/                # SFML 3.x renderer (static lib)
└── Dependencies/
    ├── Shared/                # Interfaces, packets, items, formulas
    │   ├── Item/              # Unified CItem, ItemEnums.h
    │   └── Packet/            # Packed binary network structs
    ├── Client/Includes/       # DDRAW.H, DINPUT.H, miniaudio.h, PAK.h
    ├── Server/Includes/SQLite/# sqlite3
    └── SFML/                  # SFML 3.x headers
Binaries/
├── Game/                      # Client runtime (settings.json, assets)
└── Server/                    # Server runtime, GameConfigs/, Accounts/
PLANS/                         # Implementation plans for significant changes
```

## Code Style

- **Hungarian notation:** `m_` members, `p` pointers, `i` int, `sz` strings
- **Classes:** `C` prefix, PascalCase (`CGame`, `CClient`)
- **Constants:** `DEF_` prefix, ALL_CAPS
- **Format:** Tabs, Allman braces, `#pragma once`
- **Modern C++:** `std::unique_ptr`, `std::array`, `std::chrono` alongside legacy patterns

## Architecture

### Client

`CGame` is the central coordinator holding all managers, sprites, sockets, and state.

- **Rendering:** `IRenderer`/`ISprite`/`ITexture` abstraction. Single backend: SFMLEngine (GPU). 800x600 logical resolution.
- **Screens:** `IGameScreen` base (`on_initialize/update/render/uninitialize`). `GameModeManager` singleton manages 11 screens + 11 overlays with fade transitions.
- **Dialogs:** `IDialogBox` base (`OnDraw/OnClick/OnDoubleClick/OnPress/OnItemDrop`). `DialogBoxManager` manages 42 dialog types with z-ordering.
- **Input:** `HotkeyManager` singleton - keyboard shortcuts with Ctrl/Shift/Alt modifiers. Screen/dialog handlers for mouse/key events.
- **Audio:** `AudioManager` singleton using miniaudio. 5-channel volume, 32 concurrent sounds, positional audio.
- **Camera:** `CCamera` with smooth interpolation, world-to-screen conversion, shake effects.
- **Player:** `CPlayer` holds stats, skills, magic mastery, equipment, party/guild state.
- **Chat:** `ChatCommandManager` singleton with `ChatCommand` base class.
- **Effects:** `EffectManager` - 300 max active effects, separate draw/light passes.
- **Networking:** `ASIOSocket` async TCP. `NetworkMessageManager` routes messages to 18+ handler files. Dual sockets: game + login server.

### Server

`CGame` coordinator running game loop, message queue (100K ring buffer), entity lifecycle.

- **Sessions:** `CClient` - per-player state, inventory (50), bank (1000), skills (60), magic (100).
- **NPCs:** `CNpc` - behavior states (Stop/Move/Attack/Flee/Dead), movement types, AI levels 1-3.
- **Maps:** `CMap` - tile grids, teleports, waypoints, spawn zones, fish/mineral nodes.
- **Entities:** `EntityManager` - optimized active-entity list over 15K NPC slots.
- **Parties:** `PartyManager` - up to 5K parties, 100 members each.
- **Persistence:** SQLite3 with three DB types:
  - Per-account `.db` files (`AccountSqliteStore`) - credentials, characters, items, skills
  - `GameConfigs.db` (`GameConfigSqliteStore`) - items, NPCs, magic, drops, shops, settings
  - `MapInfo.db` (`MapInfoSqliteStore`) - map data, teleports, spawners, strategic areas
- **Login:** `LoginServer` - authentication, character CRUD, game entry. Separate from game sockets.

## Shared Code (Dependencies/Shared/)

Must stay synchronized between client and server:

- **Interfaces:** IRenderer, ISprite, ISpriteFactory, ITexture, ITextRenderer, IBitmapFont, IWindow, IInput
- **Protocol:** `NetMessages.h` (message IDs), `NetConstants.h` (buffer sizes, `hb::limits::`, `hb::net::`), `ActionID.h` (object states), `Packet/*.h` (22 packed structs)
- **Items:** `Item/Item.h` (unified CItem), `Item/ItemEnums.h` (EquipPos, ItemType, effects), `Item/ItemAttributes.h`, `Item/SharedItem.h`
- **Formulas:** `SharedCalculations.h` (MaxHP/MP/SP/Load, LevelExp) - must match on both sides
- **Text:** `TextLib.h` (unified text API with font IDs, shadow styles, RAII batching)
- **Sprites:** `SpriteLoader.h` (PAK loading), `SpriteTypes.h` (DrawParams, BlendMode)
- **Graphics:** `PrimitiveTypes.h` (Color struct, BlendMode enum), `ResolutionConfig.h` (800x600 base)
- **Constants:** `MagicTypes.h` (`hb::magic::` namespace), `AdminLevel.h`, `EntityRelationship.h`
- **Networking:** `ASIOSocket.h/.cpp` (ASIO TCP), `IOServicePool.h/.cpp`, `ConcurrentMsgQueue.h`

## Rendering Pipeline

**Game Loop** (split update/render):
- `UpdateFrame()`: Audio, timers, network, game state (runs every iteration)
- `RenderFrame()`: Clear backbuffer → `GameModeManager::Render()` → `DialogBoxManager::DrawDialogBoxs()` → `DrawFadeOverlay()` → `DrawCursor()` → `EndFrame()` (gated by frame limit)

**SFML Renderer Implementations:**
| Interface | Implementation |
|-----------|---------------|
| IRenderer | SFMLRenderer (sf::RenderTexture back buffer) |
| ISprite | SFMLSprite (GPU blend) |
| ITextRenderer | SFMLTextRenderer |
| IWindow | SFMLWindow |
| IInput | SFMLInput |
| IBitmapFont | SFMLBitmapFont |

## Workflow

1. **Significant changes:** Write plan in `PLANS/` first
2. **Adding files:** Update `.vcxproj` to include new sources
3. **Network changes:** Update both client and server, rebuild both
4. **Large changes:** Pause after build, let user test
5. **No automated tests** - manual verification via running server then client
6. **Changelog:** After changes are verified and accepted by the user, append a summary to `CHANGELOG.md` (git-ignored). Use this format:
   ```
   ## Short Title
   Brief description of what was fixed/updated and how it should feel/look better.
   - Previously: old behavior/value (reason it was wrong)
   - Now: new behavior/value
   ```

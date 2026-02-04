# CLAUDE.md

Helbreath 3.82 - Classic MMORPG client-server in C++ for Windows (Win32/x86 only). Server v2.24b.

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
├── Helbreath.sln              # VS 2022 solution (4 projects)
├── build.ps1                  # Build script
├── Client/                    # Game client (~250 files)
├── Server/                    # Game server (~75 files)
├── DDrawEngine/               # DirectDraw 7 renderer (static lib)
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

- **Rendering:** `IRenderer`/`ISprite`/`ITexture` abstraction. Two backends: DDrawEngine (CPU), SFMLEngine (GPU). 640x480 logical resolution. `RendererFactory` selects backend at compile time.
- **Screens:** `IGameScreen` base (`on_initialize/update/render/uninitialize`). `GameModeManager` singleton manages 10 screens + 11 overlays with fade transitions.
- **Dialogs:** `IDialogBox` base (`OnDraw/OnClick/OnDoubleClick/OnPress/OnItemDrop`). `DialogBoxManager` manages 41 dialog types with z-ordering.
- **Input:** `InputManager` singleton - frame-based key/mouse state (pressed/released/down).
- **Audio:** `AudioManager` singleton using miniaudio. 5-channel volume, 32 concurrent sounds, positional audio.
- **Camera:** `CCamera` with smooth interpolation, world-to-screen conversion, shake effects.
- **Player:** `CPlayer` holds stats, skills, magic mastery, equipment, party/guild state.
- **Chat:** `ChatCommandManager` singleton with `ChatCommand` base class.
- **Effects:** `EffectManager` - 300 max active effects, separate draw/light passes.
- **Networking:** `XSocket` async TCP. `NetworkMessageManager` routes messages. Dual sockets: game + log server.

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
- **Protocol:** `NetMessages.h` (message IDs), `NetConstants.h` (buffer sizes), `ActionID.h` (object states), `Packet/*.h` (packed structs)
- **Items:** `Item/Item.h` (unified CItem), `Item/ItemEnums.h` (EquipPos, ItemType, effects)
- **Formulas:** `SharedCalculations.h` (MaxHP/MP/SP/Load, LevelExp) - must match on both sides
- **Text:** `TextLib.h` (unified text API with font IDs, shadow styles, RAII batching)
- **Sprites:** `SpriteLoader.h` (PAK loading), `SpriteTypes.h` (DrawParams, BlendMode)

## Rendering Pipeline

**Frame Loop** (`CGame::RenderFrame`):
1. `BeginFrame()` → 2. `GameModeManager::Render()` → 3. `DialogBoxManager::DrawDialogBoxs()` → 4. `DrawFadeOverlay()` → 5. `DrawCursor()` → 6. `EndFrame()`

**DC Management (DDraw only) - Critical:**
- `_GetBackBufferDC()` locks surface + acquires DC. `_ReleaseBackBufferDC()` releases both.
- DC locks surface - sprite drawing requires unlocked surface.
- Text calls must wrap in `BeginTextBatch()`/`EndTextBatch()` pairs.

**Renderer Implementations:**
| Interface | DDraw | SFML |
|-----------|-------|------|
| IRenderer | DDrawRenderer → DXC_ddraw | SFMLRenderer |
| ISprite | DDrawSprite (CPU blend) | SFMLSprite (GPU blend) |
| ITextRenderer | DDrawTextRenderer (GDI) | SFMLTextRenderer |
| IWindow | Win32Window | SFMLWindow |
| IInput | Win32Input | SFMLInput |
| IBitmapFont | DDrawBitmapFont | SFMLBitmapFont |

## Workflow

1. **Significant changes:** Write plan in `PLANS/` first
2. **Adding files:** Update `.vcxproj` to include new sources
3. **Network changes:** Update both client and server, rebuild both
4. **Large changes:** Pause after build, let user test
5. **No automated tests** - manual verification via running server then client

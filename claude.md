# CLAUDE.md

Helbreath 3.82 - Classic MMORPG client-server in C++ for Windows (Win32/x86 only).

## Critical Rules

- **Build from solution** - Use `Helbreath.sln`, never individual project folders
- **Build early and often** - Rebuild after each logical change
- **No over-optimization** - Implement only what is requested
- **Delete refactored code** - Git has history, don't keep old code
- **Backup before bulk edits** - Create `.bak` files before mass find/replace
- **Use Python for file ops** - Prefer Python over PowerShell for text manipulation

## Build Commands

Use `Sources\build.ps1` for all builds:

```powershell
powershell.exe -ExecutionPolicy Bypass -File "C:\Users\ShadowEvil\source\Repos3\Helbreath-3.82\Sources\build.ps1" -Target Game -Config Debug -Renderer DDraw
```

**Parameters:**
- `-Target`: `Game` | `Server` | `All`
- `-Config`: `Debug` | `Release`
- `-Renderer`: `DDraw` | `SFML` (ignored for Server)

**Common builds:**
| Command | Output |
|---------|--------|
| `-Target Game -Config Debug -Renderer DDraw` | `Debug\Game_DDRAW.exe` |
| `-Target Game -Config Debug -Renderer SFML` | `Debug\Game_SFML.exe` |
| `-Target Server` | `Debug\Server.exe` |

**Log files** (UTF-8): `build_game.log`, `build_server.log`, `build_all.log`

## Project Structure

| Path | Description |
|------|-------------|
| `Sources/Client/` | Game client (Screen_*, Overlay_* classes) |
| `Sources/Server/` | Game server (CGame, CClient, CNpc, CMap) |
| `Sources/DDrawEngine/` | DirectDraw 7 renderer (static lib) |
| `Sources/SFMLEngine/` | SFML 3.x renderer (static lib) |
| `Dependencies/Shared/` | NetMessages.h, ActionID.h (must match client/server) |

## Code Style

- **Hungarian notation:** `m_` members, `p` pointers, `i` int, `sz` strings
- **Classes:** `C` prefix, PascalCase (`CGame`, `CClient`)
- **Constants:** `DEF_` prefix, ALL_CAPS
- **Format:** Tabs, Allman braces, `#pragma once`

## Architecture

**Client:** `CGame` core with `IRenderer`/`ISprite`/`ITexture` abstraction. Two backends: DDrawEngine, SFMLEngine. `XSocket` async networking.

**Screen System:** `IGameScreen` base class with `on_initialize()`, `on_update()`, `on_render()`, `on_uninitialize()`. `Screen_*` for full screens, `Overlay_*` for modals. Screen-specific logic stays in screen class.

**Server:** `CGame` coordinator, `CClient` sessions, `CNpc` AI, `CMap` tiles. ODBC persistence.

## Workflow

1. **Significant changes:** Write plan in `PLANS/` first
2. **Adding files:** Update `.vcxproj` to include new sources
3. **Large changes:** Pause after build, let user test

## Safe Code Removal

Remove large code blocks **in chunks** to avoid corruption:

1. Replace function body with placeholder comment
2. Verify edit succeeded (read file)
3. Delete entire function
4. Remove header declarations

**Never remove 100+ lines in one edit.** If orphaned code appears, stop and ask user to evaluate.

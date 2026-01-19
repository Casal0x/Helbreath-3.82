# CLAUDE.md

Helbreath 3.82 - Classic MMORPG client-server in C++ for Windows.

## Critical Rules

| Rule | Details |
|------|---------|
| **Build from solution** | Use `Helbreath.sln`, never individual project folders |
| **Win32 only** | Target x86/Win32 exclusively |
| **No git commits** | Work only in the working tree |
| **No over-optimization** | Implement only what is requested |
| **Delete refactored code** | Do not keep old code - git has history |
| **Build early and often** | Rebuild after each logical change |
| **Backup before bulk edits** | Create `.bak` files before mass find/replace |
| **Use Python for file ops** | Prefer Python over PowerShell for text manipulation |

## Build Commands

**ALWAYS use full paths:**
```bash
# Debug builds
"C:\Users\ShadowEvil\source\Repos3\Helbreath-3.82\build_game_ddraw.bat"   # DDraw client (Debug)
"C:\Users\ShadowEvil\source\Repos3\Helbreath-3.82\build_game_sfml.bat"    # SFML client (Debug)

# Release builds
"C:\Users\ShadowEvil\source\Repos3\Helbreath-3.82\build_game_release_ddraw.bat"   # DDraw client (Release)
"C:\Users\ShadowEvil\source\Repos3\Helbreath-3.82\build_game_release_sfml.bat"    # SFML client (Release)

# Other builds
"C:\Users\ShadowEvil\source\Repos3\Helbreath-3.82\build_all.bat"    # All projects
"C:\Users\ShadowEvil\source\Repos3\Helbreath-3.82\build_server.bat" # Server only
```
Logs: `build_*.log` | Output: `Debug\Game.exe`, `Debug-SFML\Game.exe`, `Release\Game.exe`, `Release-SFML\Game.exe`

## Project Structure

| Path | Description |
|------|-------------|
| `Sources/Client/` | Game client (links DDrawEngine or SFMLEngine) |
| `Sources/Server/` | Game server (networking, AI, world state) |
| `Sources/DDrawEngine/` | DirectDraw renderer (static lib) |
| `Sources/SFMLEngine/` | SFML renderer (static lib) |
| `Dependencies/Shared/` | Shared headers (NetMessages.h, ActionID.h) |
| `Dependencies/Client/` | DirectX SDK headers and libs |
| `Dependencies/SFML/` | SFML 3.x headers and libs |
| `Binaries/` | Client/Server configs and assets |
| `PLANS/` | Implementation plans |

## Key Files

| File | Purpose |
|------|---------|
| `Sources/Client/Game.cpp` | Client game loop, rendering, UI |
| `Sources/Server/Game.cpp` | Server logic, entity management |
| `Dependencies/Shared/NetMessages.h` | Network protocol (must match both sides) |
| `Sources/*/IRenderer.h` | Graphics abstraction interface |

## Code Style

- **Naming:** Hungarian notation (`m_` members, `p` pointers, `i` int, `sz` strings)
- **Classes:** `C` prefix, PascalCase (`CGame`, `CClient`, `CNpc`)
- **Constants:** `DEF_` prefix, ALL_CAPS | **Formatting:** Tabs, Allman braces
- **Memory:** Manual `new`/`delete` | **Headers:** `#pragma once`

## Architecture

**Client:** `CGame` core class with graphics abstraction (`IRenderer`/`ISprite`/`ITexture`).
Two renderer backends: `DDrawEngine` (legacy DirectDraw 7) and `SFMLEngine` (modern SFML 3.x).
`XSocket` for async networking via Windows message pump.

**Server:** `CGame` coordinator, `CClient` sessions, `CNpc` AI, `CMap` tile world. ODBC persistence.

**Network:** Binary packed structures in `NetMessages.h`. Update both sides when changing protocol.

## Workflow

1. **Before significant changes:** Write plan in `PLANS/`
2. **Adding files:** Update `.vcxproj` to include new source/header files
3. **Large changes:** Pause after successful build, ask user to test manually

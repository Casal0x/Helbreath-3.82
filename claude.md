# CLAUDE.md

Helbreath 3.82 - Classic MMORPG client-server in C++ for Windows (early 2000s codebase).

## Critical Rules

| Rule | Details |
|------|---------|
| **Always build from solution** | Use `Helbreath.sln`, never from `Sources/Client/` or `Sources/Server/` |
| **Win32 only** | Target x86/Win32 architecture exclusively |
| **No git commits** | Work only in the working tree |
| **No over-optimization** | Implement only what is requested |
| **Delete refactored code** | Do not keep old code "for reference" - git has history |
| **Build early and often** | Rebuild after each logical change |
| **Backup before bulk edits** | Create `.bak` files before mass find/replace operations |
| **Use Python for file ops** | Prefer Python over PowerShell for text manipulation |

## Build Commands

**ALWAYS use the full path to batch files (do NOT use `cmd.exe /c "cd && batch"`):**

```bash
# Build both Client and Server
"C:\Users\ShadowEvil\source\Repos3\Helbreath-3.82\build_all.bat"

# Client only
"C:\Users\ShadowEvil\source\Repos3\Helbreath-3.82\build_game.bat"

# Server only
"C:\Users\ShadowEvil\source\Repos3\Helbreath-3.82\build_server.bat"
```

Log files are created: `build_all.log`, `build_game.log`, `build_server.log`

**Output:** `Debug\Game.exe`, `Debug\Server.exe` (or `Release\`)

## Project Structure

```
Sources/Client/      - Game client (DirectDraw/DirectInput/DirectSound)
Sources/Server/      - Game server (networking, AI, world state)
Dependencies/Shared/ - Shared headers (NetMessages.h, ActionID.h, DynamicObjectID.h)
Dependencies/Client/ - DirectX SDK headers and libs
Binaries/Game/       - Client configs and assets
Binaries/Server/     - Server configs
PLANS/               - Implementation plans for significant changes
```

## Key Files

| File | Purpose |
|------|---------|
| `Sources/Client/Game.cpp` (~1.7MB) | Client game loop, rendering, UI |
| `Sources/Server/Game.cpp` (~2.1MB) | Server logic, entity management |
| `Dependencies/Shared/NetMessages.h` | Network protocol (must match both sides) |
| `XSocket.cpp/h` | Async socket wrapper (both sides) |

## Code Style

- **Naming:** Hungarian notation (`m_` members, `p` pointers, `i` int, `sz` strings)
- **Classes:** `C` prefix, PascalCase (`CGame`, `CClient`, `CNpc`)
- **Constants:** `DEF_` prefix, ALL_CAPS
- **Formatting:** Tabs, Allman braces
- **Memory:** Manual `new`/`delete`, no smart pointers
- **Headers:** `#pragma once`

## Architecture

### Client (Game)
- `CGame` - Monolithic class: rendering, input, network, UI
- `XSocket` - Async sockets via Windows message pump (`WM_USER_GAMESOCKETEVENT`)
- DirectX 7/8: DirectDraw, DirectInput, DirectSound

### Server
- `CGame` - Central coordinator for all systems
- `CClient` - Player session (inventory, stats, skills, connection)
- `CNpc` - NPC behavior and AI
- `CMap` - Tile-based world, collision, visibility
- ODBC for database persistence

### Network Protocol
- Binary packed structures with message IDs in `NetMessages.h`
- Both sides must have identical definitions
- When changing: update both client and server, rebuild both

## Workflow

1. **Before significant changes:** Write plan in `PLANS/`
2. **Adding files:** Update `.vcxproj` to include new source/header files
3. **After completing TODO items:** Update `DONE.md`, mark complete in `TODO.md`
4. **Large changes:** Pause after successful build, ask user to test manually

## Testing

No automated tests. Manual only:
- Run server then client using configs in `Binaries/`
- For network changes, verify message symmetry between client and server

## Server Limits

```cpp
DEF_MAXCLIENTS          2000
DEF_MAXNPCS             5000
DEF_MAXMAPS             100
DEF_MAXITEMTYPES        5000
DEF_MAXDYNAMICOBJECTS   60000
```

## Known Issues

- 16 header files duplicated between Client and Server (see `TODO.md`)
- Korean comments in server code

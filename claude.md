# Helbreath 3.82 - Project Guide

## Build
```powershell
powershell -ExecutionPolicy Bypass -File Sources\build.ps1 -Target All -Config Debug
powershell -ExecutionPolicy Bypass -File Sources\build.ps1 -Target Game -Config Debug
powershell -ExecutionPolicy Bypass -File Sources\build.ps1 -Target Server -Config Debug
```
- Delete `build_*.log` before building for clean logs.
- Output: `Sources\Debug\Game_SFML_x64.exe` (client), `Sources\Debug\Server.exe` (server).
- x64 platform. `Sources\Helbreath.sln`. C++17 server, C++20 client.
- 78 LNK4099 warnings (missing SFML/freetype PDBs) are cosmetic — ignore.

## Workflow

**Never use git commands. The user handles all git operations. Backups use `.bak_<guid>` files via `Scripts/bak.py`.**

Two modes. **Default to Mode 1.** Use Mode 2 only when justified.

### Mode 1: Direct Edit (DEFAULT — 1-9 files)

```
bak.py guard <files>  →  Read/Edit tools  →  Build  →  bak.py commit (or revert)
```

1. **Guard** — `python Scripts/bak.py guard <file1> [file2 ...]` — creates versioned checkpoint (.bak_<guid>).
2. **Edit** — Read and Edit tools.
3. **Build** — `powershell -ExecutionPolicy Bypass -File Sources/build.ps1 -Target All -Config Debug`
4. **If build succeeds** — `python Scripts/bak.py commit` — deletes all .bak files, accepts changes.
5. **If build fails** — choose:
   - `guard` again to checkpoint, then fix and rebuild (layer the fix).
   - `revert` to undo last attempt, retry from previous checkpoint.
   - `revert --all` to return to original clean state.

Rules:
- If the change is specific edits to specific lines → Mode 1.
- One logical change per guard-edit-build-commit cycle.
- Each `guard` creates a new checkpoint — layer fixes and peel back selectively.

### Mode 2: Python Script (BULK — 10+ files)

Must justify: "This touches N files with pattern X, script appropriate because Y."
See `CLAUDE_WORKFLOW.md` for script pattern and regex safety rules.

### `bak.py` Commands

| Command | Purpose |
|---------|---------|
| `bak.py guard <files>` | Create versioned checkpoint (.bak_<guid>) |
| `bak.py status` | List checkpoints with dirty/clean status (exit 1 if any) |
| `bak.py revert` | Peel back one checkpoint layer (most recent) |
| `bak.py revert --all` | Revert to original state (oldest checkpoint per file) |
| `bak.py commit` | Delete all .bak* files (accept current state) |

## Code Search

`Scripts/grep.py` — two modes: brief (`-b`) for quick stdout, detailed for full-context log file.

```
python Scripts/grep.py "pattern" -b                       # brief: one line per match to stdout
python Scripts/grep.py "pattern"                          # detailed: context blocks to log file
python Scripts/grep.py "pattern" -F --path Sources/Client # fixed string, scoped to directory
python Scripts/grep.py "pattern" -C 8 -i -o results.log  # 8 context lines, case-insensitive
```

Brief (`-b`): prints `file:line | match` to stdout. Detailed (default): writes to `Scripts/output/grep_results.log` with context, enclosing scope, `>>>` markers. Output dir auto-clears at 10MB.

## Project Structure

- **Client** (`Sources/Client/`) — Game client, C++20. Depends on SFMLEngine.
- **SFMLEngine** (`Sources/SFMLEngine/`) — Rendering abstraction over SFML.
- **Server** (`Sources/Server/`) — Game server, C++17. Standalone.
- **Shared** (`Sources/Dependencies/Shared/`) — Protocol, enums, items, packets, networking.
- `Binaries/Game/` — Client runtime (configs, sprites, sounds, maps, fonts).
- `Binaries/Server/` — Server runtime (SQLite DBs, accounts).

See `CLAUDE_ARCHITECTURE.md` for detailed client/server/shared architecture.

## Modernization Direction

- Legacy C-style → C++17/20 while preserving game logic.
- Goal: OOP, polymorphism, templates, clearly scoped systems.
- Modernize code you touch. Don't gold-plate untouched code.

## Coding Standards

**Convention: `snake_case` throughout.** Full reference: `PLANS/CODING_STANDARDS.md`.

Key rules:
- **Tabs** for indentation, **Allman** braces.
- **Everything `snake_case`**: classes, methods, members, params, constants, enum values.
- **No prefixes**: `weather_manager` not `CWeatherManager`, `renderer` not `IRenderer`.
- **Members**: `m_snake_case`. Struct data members omit `m_`.
- **Constants**: `constexpr snake_case`. No new `#define` constants.
- **Enums**: Namespace-wrapped unscoped enum (`attack_type::normal`). `enum class` only when implicit conversion undesirable.
- **Namespaces**: `hb::<shared|client|server>::snake_case`.
- **Ownership**: `std::unique_ptr` only. Raw pointers non-owning. Prefer `&` references.
- **Error handling**: Exceptions for critical failures. `bool` for recoverable. No `goto`.
- **Null**: `nullptr` only. **Headers**: `#pragma once`. No `using namespace` in headers.
- **Wire protocol structs**: Exception — Hungarian without `m_` to match binary format.
- Legacy code retains old conventions until actively refactored. Do not reformat untouched code.

## Testing

No automated tests. Manual: run server, then client with configs in `Binaries/`.
For network changes, rebuild both client and server.

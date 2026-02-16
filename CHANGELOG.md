# Unified Versioning System

Replaced the legacy client (3.82.0) and server (2.24b build 1126) version systems with a modern, unified versioning system managed from a single configuration file.

## How It Works

All version numbers live in `Sources/version.cfg` — an INI-style file with three sections:

- **Compatibility** — The protocol version. If the client's compatibility version doesn't match the server's, the connection is rejected before login. This is the gatekeeper that ensures client and server can talk to each other.
- **Client Version** — The client's own identity. Displayed in the window title bar and the in-game version overlay. Independent of the server version.
- **Server Version** — The server's own identity. Displayed in the server console banner and logs. Independent of the client version.

Each track uses **major.minor.patch** with a stage tag (alpha/beta/rc/release). A pre-build script (`Sources/version_gen.py`) reads the config, auto-increments a build counter, and generates C++ headers, Windows resource metadata, and CMake variables — all from that single source file.

For the full versioning guide, see `VERSION_STANDARDS.md`.

## Version Check on Login

- Client sends its compatibility version (major.minor.patch) with every login and enter-game request
- Server checks all three fields match before credential validation
- Mismatched clients see the "Version Not Match" overlay (previously orphaned UI, now wired up)
- Added `LogResMsg::VersionMismatch` (0x0A08) response code
- Added version fields to `LoginRequest`, `EnterGameRequest`, and `EnterGameRequestFull` packet structs

## Assembly / Binary Versioning

- **Windows**: Client (`Game.rc`) and Server (`Server.rc`) embed VERSIONINFO resources in the .exe — visible in right-click > Properties > Details
- **Linux**: CMake `project(VERSION ...)` populated from generated `version.cmake`; version strings compiled into the binary via constexpr
- **macOS**: Not yet a build target; Info.plist template planned for when it's added

## Cross-Platform Window Icon

- **Windows**: Icon embedded in the .exe via `Game.rc` (uses `icon1.ico`)
- **Linux/macOS**: Icon pixel data embedded in `embedded_icon.h` (generated from `icon1.ico`), applied at startup via SFML's `setIcon()`
- Regenerate with `python Scripts/gen_embedded_icon.py` after changing the icon

## Generated Files (auto-updated per build)

| File | Purpose |
|------|---------|
| `Sources/Dependencies/Shared/version_info.h` | C++ constexpr header (`hb::version::{compatibility,server,client}::`) |
| `Sources/Dependencies/Shared/version_rc.h` | Windows RC `#define` values for VERSIONINFO |
| `Sources/version.cmake` | CMake `set()` variables for Linux builds |
| `Sources/Dependencies/Shared/embedded_icon.h` | 32x32 RGBA icon pixel data for Linux/macOS |

## Build System Changes

- `Sources/version_gen.py` runs automatically before every build (hooked into build.ps1, build_linux.sh, build_client_linux.sh)
- Build counter (`Sources/build_counter.txt`) auto-increments — shared across all three version tracks
- Fixed two C4244 narrowing warnings in Server/Game.cpp (uint64_t to uint32_t/int)

## Removed

- `Sources/Client/Version.h`, `Sources/Client/Version.cpp`, `Sources/Server/Version.h` — replaced by shared `version_info.h`

## Current Versions

| Track | Version |
|-------|---------|
| Compatibility | 0.1.1-alpha |
| Client | 0.1.1-alpha |
| Server | 0.1-alpha |

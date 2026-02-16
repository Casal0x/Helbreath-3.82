# Version Standards

## Overview

Helbreath uses a three-track versioning system with a single source of truth: `Sources/version.cfg`. Each build auto-generates C++ headers, Windows resource metadata, and CMake variables from this file.

### The Three Version Tracks

| Track | Purpose | Who cares |
|-------|---------|-----------|
| **Compatibility** | Protocol version — must match between client and server | Server gate, login screen |
| **Client** | Client identity — displayed in window title and in-game | Players, bug reports |
| **Server** | Server identity — displayed in console banner and logs | Operators, tracebacks |

The **Compatibility** version is the gatekeeper: if the client's compatibility version doesn't exactly match the server's (major.minor.patch), the connection is rejected before credentials are even checked. Client and Server versions can drift independently.

### Version Format

```
0.1.1-alpha+build.42
| | |   |        |
| | |   |        +-- auto-incremented per build (never edit manually)
| | |   +-- stage: alpha -> beta -> rc -> (empty for release)
| | +-- patch: hotfixes
| +-- minor: additional content
+-- major: game-breaking logic changes
```

**Display version**: Omits patch when 0, omits stage when empty.
- `0.1-alpha` (patch=0, stage=alpha)
- `1.2.3` (patch=3, stage=empty/release)

**Full version**: Always includes everything plus build metadata.
- `0.1.1-alpha+build.42`

## Source File: `Sources/version.cfg`

INI-style file with three sections:

```ini
[Compatibility]
major=0
minor=1
patch=1
stage=alpha

[Server Version]
major=0
minor=1
patch=0
stage=alpha

[Client Version]
major=0
minor=1
patch=0
stage=alpha
```

**This is the only file you edit to change version numbers.**

## When to Update Each Track

### Compatibility Version

Update when the client and server **must be updated together** to communicate:

| Field | When |
|-------|------|
| **major** | Fundamental protocol redesign (new packet format, changed handshake flow) |
| **minor** | New packets, changed packet fields, new login/enter-game requirements |
| **patch** | Hotfixes to protocol behavior (changed validation, fixed packet handling) |

**Rule**: If you change anything in `Sources/Dependencies/Shared/Packet/` or `Sources/Dependencies/Shared/Net/NetMessages.h`, you almost certainly need a compatibility bump.

### Client Version

Update when the client changes, regardless of server:

| Field | When |
|-------|------|
| **major** | Major UI overhaul, new game systems visible to players |
| **minor** | New features, noticeable improvements, content additions |
| **patch** | Bug fixes, visual tweaks, performance improvements |

### Server Version

Update when the server changes, regardless of client:

| Field | When |
|-------|------|
| **major** | Database schema changes, major game logic rewrites |
| **minor** | New server commands, balance changes, new NPC/item types |
| **patch** | Bug fixes, crash fixes, security patches |

### Examples

- **Added a new spell**: Client minor bump (new UI/effects) + Server minor bump (new logic) + Compatibility minor bump (new packets)
- **Fixed a client rendering bug**: Client patch bump only
- **Fixed a server crash**: Server patch bump only
- **Changed how damage packets work**: Compatibility patch bump + Server patch bump + Client patch bump
- **New version check behavior**: Compatibility patch bump (protocol behavior changed)

## Build Flow

The pre-build script `Sources/version_gen.py` runs automatically before every build:

```
Sources/version.cfg         (you edit this)
Sources/build_counter.txt   (auto-incremented)
        |
        v
Sources/version_gen.py      (runs pre-build)
        |
        +---> Sources/Dependencies/Shared/version_info.h   (C++ constexpr header)
        +---> Sources/Dependencies/Shared/version_rc.h     (Windows RC #defines)
        +---> Sources/version.cmake                        (CMake variables)
```

### Generated Files (never edit these)

**`version_info.h`** — C++ constexpr values used in game code:
```cpp
hb::version::compatibility::major      // Protocol version check
hb::version::client::display_version   // "0.1-alpha" in window title
hb::version::server::full_version      // "0.1.0-alpha+build.42" in server logs
hb::version::build_number              // Auto-incremented build counter
hb::version::build_timestamp           // ISO 8601 build time
```

**`version_rc.h`** — `#define` values for Windows resource (VERSIONINFO) metadata:
```c
VER_CLIENT_FILEVERSION   0,1,0,42    // Shows in file Properties > Details
VER_SERVER_FILEVERSION   0,1,0,42
VER_CLIENT_FULL          "0.1.0-alpha+build.42"
```

**`version.cmake`** — CMake variables for Linux builds:
```cmake
HB_CLIENT_VERSION "0.1.0"   // Used in project(VERSION ...)
HB_SERVER_VERSION "0.1.0"
HB_BUILD_NUMBER 42
```

### Build Counter

`Sources/build_counter.txt` contains a single integer that increments on every build. This is shared across all three version tracks. Don't edit it manually — it auto-increments when `version_gen.py` runs.

## Platform-Specific Versioning

### Windows

The `.exe` files embed version metadata via Windows Resource (`.rc`) files:
- `Sources/Client/Game.rc` — Client VERSIONINFO (visible in right-click > Properties > Details)
- `Sources/Server/Server.rc` — Server VERSIONINFO

Both include `version_rc.h` for the version numbers. The client's RC file also embeds the application icon (`icon1.ico`).

### Linux

CMake `project(VERSION ...)` is populated from `version.cmake`. The version is compiled into the binary via `version_info.h` constexpr strings. There's no native ELF equivalent to Windows VERSIONINFO — `strings HelbreathClient | grep build` will find the embedded version strings.

### macOS

Not yet supported as a build target. When added, an `Info.plist` template should be created with `CFBundleShortVersionString` and `CFBundleVersion` populated from the generated version values.

## Window Icon

### Windows
The window icon is embedded in the `.exe` via the `Game.rc` resource file referencing `icon1.ico`. Windows uses this for the title bar, taskbar, ALT+TAB, and File Explorer.

### Linux / macOS
SFML's `setIcon()` is called at startup with embedded pixel data from `Sources/Dependencies/Shared/embedded_icon.h`. This header is generated from `icon1.ico` and contains a 32x32 RGBA pixel array. To regenerate it after changing the icon, run:
```bash
python3 Scripts/gen_embedded_icon.py
```

## Quick Reference

| I want to... | Do this |
|--------------|---------|
| Bump a version | Edit `Sources/version.cfg`, rebuild |
| Check current versions | Look at `Sources/version.cfg` or the generated `version_info.h` |
| See version in the client | Window title bar or in-game overlay |
| See version in the server | Console banner on startup |
| Change the build counter | Don't — it auto-increments |
| Add version to a new display | Use `hb::version::{client,server}::display_version` or `full_version` |
| Change the icon | Replace `Sources/Client/icon1.ico`, regenerate `embedded_icon.h` |

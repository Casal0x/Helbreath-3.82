# Changelog

### Bug Fixes
- Fixed window flash/flicker on alt-tab — removed legacy `ChangeDisplayMode` call from focus regain handler (was recreating the entire SFML window on every alt-tab); added no-op guard to `set_fullscreen` to prevent window recreation when mode hasn't changed
- Fixed inventory double-click using stale item selection — `OnDoubleClick` now reads from `CursorTarget` (set by pixel-perfect `OnPress`) instead of re-testing via bounding-box `FindClickedItem`, preventing mismatched item actions (e.g., re-depositing a just-withdrawn bank item)
- Fixed bank deposit dropping items on the ground instead of depositing — server GiveItemHandler compared NPC name "Howard" but NPC was renamed to "Warehouse Keeper"; replaced all NPC name comparisons with `m_iNpcConfigId` checks (58 = Warehouse Keeper, 56 = Shop Keeper)
- Initialized client logger (`hb::logger::initialize`) so debug log output actually reaches console and log files
- Added diagnostic logging throughout bank deposit flow (client: inventory, bank dialog, cursor, drag-drop; server: GiveItemHandler entry, GetOwner results, objectID validation, drop/deposit branches)
- Change password no longer hangs when the old password is wrong — server now sends a failure response instead of silently dropping the request
- Added diagnostic logging to chat message handler to help debug potential struct packing mismatches between Windows client and Linux server
- Fixed server console output having no text colors — routed logger console output through the console writer instead of buffered stdout which raced with color attribute resets
- Fixed prompt artifact appearing mid-startup output — removed premature prompt draw from console init

### New Features
- Server now builds and runs on Linux — full cross-platform support with CMake build system and portable entry point
- Created application base class — owns main loop, routes input transparently, converts window lifecycle to discrete events
- Two-phase window init: allocate and stage params, then create OS window
- CGame now derives from application with 5 required + 3 optional lifecycle overrides (initialize, start, uninitialize, run, event, key event, native message, text input)
- Close-to-quit now goes through a clean request path, allowing logout countdown
- Entry point rewritten as minimal bootstrapper: create game, attach window, initialize, run
- Created lightweight event type with ID + union payload for engine and game events
- Created DevConsole removal — deleted overlay, toggle hotkey, key routing, visibility checks, and 39 debug print calls

### Code Cleanup
- Renamed all window, input, and renderer APIs from PascalCase to snake_case across 41 files
- Renamed window params struct and members to snake_case
- Fixed 148 build errors by adding const to char* parameters that receive string literals across server game, war manager, and NPC constructors
- Removed 19 unreferenced local variables across 8 server files (leftover from logging migration)
- Migrated 422 server log calls from legacy functions to channel-based logger across 29 files
- Removed old logging infrastructure (4 files deleted, ~300 lines of function bodies removed, extern declarations cleaned from 15+ files)
- Added item info formatting and suspicious item detection as helpers extracted from deleted logging class
- Modernized ~200 log messages: removed prefix markers, fixed misspellings, converted printf format specifiers to std::format syntax

### Infrastructure — Server Cross-Platform
- Replaced 32 backslash path prefixes with forward slashes across 8 files
- Replaced 20 ZeroMemory calls with standard memset across 3 files
- Created string compatibility header for case-insensitive string functions, replaced 77 occurrences across 19 files
- Created portable local time utility, replaced platform-specific time calls across 10 files
- Replaced platform-specific directory creation with standard filesystem across 5 files
- Replaced Win32 file I/O (CreateFile, ReadFile, etc.) with standard C/C++ equivalents across 3 files
- Replaced Win32 directory enumeration with standard filesystem iterator
- Replaced Win32 file attribute and module path queries with standard filesystem equivalents
- Rewrote password hashing: replaced Windows BCrypt with portable inline SHA-256 implementation, platform-guarded CSPRNG
- Platform-guarded chat tail viewer: CreateProcess on Windows, system() on POSIX
- Rewrote server console output using ANSI escape codes instead of Win32 console API, added POSIX terminal support
- Removed windows.h from 43 server files, replaced remaining Windows types with standard integer types
- Removed dead F-key hotkey code (~135 lines) that was unreachable after entry point rewrite
- Rewrote entry point: replaced WinMain/WndProc/message pump with portable main() + deterministic tick loop
- Eliminated hidden window, multimedia timer, and WM_USER message posting
- Changed server project subsystem from Windows to Console

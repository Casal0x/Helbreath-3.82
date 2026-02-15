# Changelog

## Direction Enum — Type-Safe Compass Directions

### New Type
- Added `enum direction : int8_t` in `hb::shared::direction` namespace (DirectionHelpers.h) with values `north=1` through `northwest=8`; zero handled via `direction{}` (value-initialization)
- Added `constexpr int8_t direction_count = 8`
- Added `operator++` (prefix and postfix) for direction cycling
- Changed `ApplyOffset()` overloads from `char dir` to `direction dir`

### Client Headers (type changes: char/int8_t -> direction)
- Player.h: `m_player_dir`
- EntityMotion.h: `m_direction`, `m_pending_direction`, 4 function params
- AnimationState.h: `m_dir`, `set_action`/`set_direction` params
- Tile.h: `m_dead_dir`
- EntityRenderState.h: `m_dir`
- Effect.h: `m_dir`
- PlayerController.h: `m_pending_stop_dir`, `get_next_move_dir` return type
- CursorTarget.h: `FocusedObject::m_direction`, `TargetObjectInfo::m_direction`, `get_focus_highlight_data` param
- Misc.h (client): `get_next_move_dir`, `calc_direction` return types
- MapData.h: 4 function dir params
- Game.h: `m_menu_dir`

### Server Headers (type changes: char -> direction)
- Client.h: `m_dir`
- Npc.h: `m_dir`
- TeleportLoc.h: `m_dir`
- Map.h: `m_heldenian_gate_door[].dir`, `check_fly_space_available`/`search_teleport_dest` params
- Misc.h (server): `get_next_move_dir` return, `GetDirPoint` param
- Game.h: 4 function signatures, `get_next_move_dir` return
- CombatManager.h, MagicManager.h, ItemManager.h: function params

### Source Files Updated (~25 .cpp files)
- All integer literals for directions (1-8) replaced with named enum values (`direction::north` through `direction::northwest`)
- All `= 0` direction assignments replaced with `direction{}`
- Wire protocol structs unchanged (`uint8_t` for network packets) — `static_cast<direction>()` used only at packet boundaries, database reads, and RNG results
- Direction inversion switch cases (NpcRenderer, PlayerRenderer) use named enum values
- Menu direction cycling (6 overlays + 2 screens) uses `operator++` with named wrap check

## FloatingTextManager: Modernize to std::string / std::string_view

### Bug Fix
- Rewrote `draw_message()` line-splitting from raw `char*` pointer arithmetic (`cp += 20` past end of string) to `std::string::substr()` — fixes text artifacts on Linux/GCC where memory after SSO buffer contains garbage

### Modernization
- Changed all `floating_text` constructors from `const char* pMsg` to `std::string_view msg` (FloatingText.h)
- Changed `add_chat_text`, `add_damage_text`, `add_notify_text` public API from `const char*` to `std::string_view` (FloatingTextManager.h/cpp)
- Replaced fixed `char[22]` line buffers with `std::string` in sprite-font rendering path
- Width-measurement loop now uses `msg_a.size()` instead of null-byte check in fixed array
- Removed 6 unnecessary `.c_str()` calls at call sites (Game.cpp, NetworkMessages_Skills.cpp, FloatingTextManager.cpp)

## Linux Build Script Improvements

- Updated `build_linux.sh` and `build_client_linux.sh` to copy built binaries into `Sources/Debug/` or `Sources/Release/` — mirrors the Windows output layout
- Added proper `debug`/`release` argument parsing (combinable with `clean`, e.g. `./build_linux.sh clean release`)
- Added auto-reconfigure: switching between Debug and Release no longer requires a manual `clean` — the script detects the cached build type and reconfigures automatically
- Updated `CLAUDE.md` build documentation to reflect new usage and output paths

## Client Windows Dependency Cleanup (Phase D1)

### Code Cleanup
- Replaced all unguarded `DWORD` (33 occurrences) with `uint32_t` across 8 client files
- Replaced all unguarded `WORD` (21 occurrences) with `uint16_t` across 9 client files
- Removed unguarded `#include <windows.h>` from 23 client files (7 headers, 16 .cpp files)
- Removed unused `#include <mmsystem.h>` from Tile.h and `#include <io.h>` from MapData.h
- Added `#include <cstdint>` to CharInfo.h and Tile.h (needed after windows.h removal)
- Rewrote `MapData::open_map_data_file()` from Win32 CreateFile/ReadFile to `std::ifstream`
- Rewrote `Game::init_data_response_handler()` file size check from Win32 CreateFile/GetFileSize to `std::filesystem::file_size`
- Replaced `SYSTEMTIME`/`GetLocalTime` in DialogBox_SysMenu.cpp with `std::chrono`/`localtime_s`/`localtime_r`
- Made `Game::create_screen_shot()` `localtime_s` call cross-platform with `#ifdef _WIN32` guard
- Replaced `_mkdir` in LocalCacheManager.cpp with `std::filesystem::create_directories`
- Replaced `__declspec(selectany)` with C++17 `inline` in Benchmark.h (5 static members)
- Replaced `GetTickCount()` with `GameClock::get_time_ms()` in SFMLSprite.cpp (3 call sites)
- Made SFMLTextRenderer font resolution cross-platform — checks bundled `fonts/` directory first, then platform-specific system font paths
- Removed unused DDraw legacy methods from ITexture interface: `Blt`, `BltFast`, `GetNativeHandle`, `IsLost`, `Restore`, and `TextureFlags` namespace
- Removed dead `RECT` forward-declaration from SpriteTypes.h
- Removed `#ifdef _WIN32` / `#include <windows.h>` block from CommonTypes.h — breaks transitive windows.h dependency chain
- Removed `#define _WINSOCKAPI_` from MapData.cpp
- Updated `DWORD` references in Game.cpp comment blocks to `uint32_t`
- Renamed `ChatManager::GetMessage` to `get_message` — avoids Win32 `GetMessage` macro collision

### New Features
- Created `platform_headers.h` — centralized OS-specific header management. Single point of contact for `<windows.h>` inclusion with NOMINMAX, WIN32_LEAN_AND_MEAN, winsock2 ordering, and comprehensive Win32 macro cleanup (#undef for GetMessage, DrawText, CreateFont, PlaySound, CreateFile, DeleteFile, etc.)
- All direct `#include <windows.h>` in Client, SFMLEngine, and Shared replaced with `#include "platform_headers.h"`
- Removed scattered `#undef` workarounds from IRenderer.h, ITextRenderer.h, BitmapFontFactory.h, SFMLBitmapFont.h, NativeTypes.h — now handled centrally
- Added `hb::platform` namespace to `platform_headers.h` with cross-platform inline wrappers: `get_screen_width`, `get_screen_height`, `get_monitor_refresh_rate`, `set_timer_resolution`, `restore_timer_resolution`, `add_minimize_button`, `position_window`, `resize_window`, `focus_window`, `get_client_size`, `show_error_dialog`
- Moved `#include <mmsystem.h>` into `platform_headers.h` (centralized with other OS headers)
- Replaced all 14 direct Win32 API calls in SFMLWindow.cpp with `hb::platform::` wrappers — `GetSystemMetrics`, `SetWindowPos`, `GetWindowLong`/`SetWindowLong`, `SetForegroundWindow`/`SetFocus`, `EnumDisplaySettings`, `GetClientRect`, `MessageBox`, `timeBeginPeriod`/`timeEndPeriod`
- Removed 7 `#ifdef _WIN32` guards from SFMLWindow.cpp (platform logic now inside wrappers)
- Moved `namespace MouseButton` alias outside `#ifdef _WIN32` block (it's engine-agnostic, not OS-specific)
- Moved `#include "platform_headers.h"` outside `#ifdef _WIN32` guards in 5 files (NativeTypes.h, SFMLRenderer.cpp, Benchmark.h, Game.cpp, GameWindowHandler.cpp) — platform_headers.h is self-guarding and must be unconditional so `hb::platform::` stubs compile on Linux

### Linux Client Build (Phase 7)
- Created `Sources/Client/CMakeLists.txt` — CMake build for the client on Linux. Compiles SFMLEngine (9 files) as static library, Shared sources (8 files), and all Client .cpp files. SFML 3 auto-fetched via FetchContent if not installed system-wide
- Created `Sources/build_client_linux.sh` — companion build script mirroring server pattern (incremental/clean/release modes)
- Created `Scripts/build_sfml.bat` — Windows batch script to download, build, and install SFML 3.0.2 from source into `Sources/Dependencies/SFML/` (headers + x64 static libs, Debug + Release)
- Updated `CLAUDE.md` with client Linux build section, SFML rebuild instructions, and deploy documentation

### Linux Build Fixes
- Merged `NativeTypes.h` type definitions into `platform_headers.h` — `hb::shared::types::NativeWindowHandle` and `NativeInstance` now defined alongside OS headers. NativeTypes.h reduced to a thin redirect (`#include "platform_headers.h"`)
- Added `using handle = hb::shared::types::NativeWindowHandle` alias in `hb::platform` namespace — wrappers now use the correct platform type (`HWND` on Windows, `unsigned long` on Linux) instead of `void*`
- Fixed `nullptr` assignment to non-pointer `NativeWindowHandle` on Linux: `m_handle(nullptr)` → `m_handle{}` in SFMLWindow.cpp and SFMLInput.cpp, `m_handle = nullptr` → `m_handle = {}` in SFMLWindow.cpp
- Fixed ternary type mismatch in RendererFactory.cpp: `s_window->get_handle() : nullptr` → `s_window->get_handle() : NativeWindowHandle{}`
- Added `#include <cstring>` to PAK.h — GCC requires explicit include for `std::memcpy` (MSVC provides it transitively)
- Replaced `GetSystemMetrics(SM_CXSCREEN/SM_CYSCREEN)` in DialogBox_SysMenu.cpp with `hb::platform::get_screen_width/height()` — was the last direct Win32 API call in client code outside platform_headers.h
- Fixed `GameWindowHandler::on_destroy()`: `m_game->Quit()` → `m_game->request_quit()` (method didn't exist, `request_quit()` is inherited from `application` base class)
- Removed dead `check_important_file()` call from `GameWindowHandler::on_activate()` — legacy file integrity check method no longer exists
- Fixed GCC `-Wformat-truncation` warning in server `ItemManager.cpp`: `snprintf "fightzone%d"` → `"fightzone%c"` with `'0' + (zoneNum % 10)` — makes single-digit output explicit to the compiler
- Fixed `GameWindowHandler::on_destroy()`: `request_quit()` is protected in `application` — use `Window::close()` instead, which makes the run loop exit naturally
- Fixed GCC `-Wformat-truncation` warning in `DialogBox_Shop.cpp`: `temp[21]` → `temp[ItemNameLen]` (42) — buffer was too small for item names up to 41 chars
- Fixed GCC `-Wformat-truncation` warning in `Game.cpp` `chat_msg_handler`: `message[100]` → `message[204]` — output buffer needs room for `"%s: %s"` combining two 99-char inputs
- Fixed Linux PAK file loading crash: `SpriteLoader.h` used `"\\"` (backslash) for path separator — changed to `"/"` (forward slash) which works on both Windows and Linux. `SFMLSpriteFactory::BuildPakPath` already used `"/"` correctly, but `SpriteLoader::open_pak` did not
- Replaced all Windows backslash path separators with forward slashes across client code (Windows handles `/` fine, Linux requires it):
  - `Game.cpp`: `"mapdata\\"` → `"mapdata/"`, `"mw\\defaultmw"` → `"mw/defaultmw"`, `"contents\\\\"` → `"contents/"`, `"save\\"` → `"save/"`
  - `AudioManager.cpp`: `"sounds\\"` → `"sounds/"` (3 occurrences: character, monster, effect sounds), `"music\\"` → `"music/"`
  - `BuildItemManager.cpp`: `"contents\\"` → `"contents/"`
  - `LocalCacheManager.cpp`: `"cache\\"` → `"cache/"` (4 cache file paths) — on Linux, backslash is a valid filename character, so files were created as e.g. `cache\{guid}.bin` in the working directory instead of `cache/{guid}.bin` in the subdirectory
- Fixed fullscreen mouse lock on Linux: added `SFMLWindow::apply_cursor_grab()` helper that skips `setMouseCursorGrabbed(true)` in fullscreen mode (cursor is already confined by the OS). On Linux/X11, grabbing the cursor in fullscreen locks it completely. Replaced all 8 direct `setMouseCursorGrabbed` call sites with `apply_cursor_grab()`
- Fixed window centering on Linux: added `SFMLWindow::get_desktop_size()` (SFML `getDesktopMode()` on Linux, `GetSystemMetrics` on Windows) and `SFMLWindow::move_window()` (SFML `setPosition` on Linux, `SetWindowPos` on Windows). Platform wrappers returned 0/no-op on Linux
- Fixed progressive window shrinking on fullscreen toggle: resize events from window decorations were overwriting `m_width`/`m_height`, which got saved as `m_windowed_width/height` on next fullscreen toggle. Now `m_windowed_width/height` are only updated by `realize()` and `set_size()`, never from resize events
- Fixed fullscreen on Linux using borderless windowed mode: `sf::State::Fullscreen` on X11 grabs the pointer at the driver level, locking mouse movement entirely. On Linux, `realize()` and `set_fullscreen()` now use `sf::State::Windowed` with `sf::Style::None` at desktop resolution instead of true X11 fullscreen — visually identical but without pointer grab
- Added 4:3 fullscreen mouse clamping on Windows: `SFMLWindow::update_cursor_clip()` uses `ClipCursor()` to restrict the OS cursor to the letterbox render area in fullscreen non-stretch mode, preventing the hidden cursor from drifting into black bars. Clip is applied on realize, fullscreen toggle, stretch toggle, focus gain; released on focus loss and destroy

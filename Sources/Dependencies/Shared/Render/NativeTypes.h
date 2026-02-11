// NativeTypes.h: Compile-time platform type aliases
//
// Provides platform-neutral type aliases for window handles and instance handles.
// On Windows, these resolve to the real Win32 types (HWND, HINSTANCE) - zero casts needed.
//////////////////////////////////////////////////////////////////////

#pragma once

#ifdef _WIN32
    #include <winsock2.h>
    #include <windows.h>
    // Undefine Win32 macros that collide with game method names
    #undef PlaySound
#endif

namespace hb::shared::types {

#ifdef _WIN32
    using NativeWindowHandle = HWND;
    using NativeInstance = HINSTANCE;
#elif defined(__linux__)
    using NativeWindowHandle = unsigned long;  // X11 hb::shared::render::Window
    using NativeInstance = void*;
#elif defined(__APPLE__)
    using NativeWindowHandle = void*;  // NSWindow*
    using NativeInstance = void*;
#else
    using NativeWindowHandle = void*;
    using NativeInstance = void*;
#endif

} // namespace hb::shared::types

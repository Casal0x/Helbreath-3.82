// SFMLWindow.cpp: Pure SFML window implementing hb::shared::render::IWindow interface
//
// Part of SFMLEngine static library
// Supports two-phase init: allocate → configure via setters → realize()
//////////////////////////////////////////////////////////////////////

#include "SFMLWindow.h"
#include "SFMLInput.h"
#include "SFMLRenderer.h"
#include "RendererFactory.h"
#include "RenderConstants.h"
#include <SFML/Window/Event.hpp>

#ifdef _WIN32
#include <windows.h>

namespace MouseButton = hb::shared::input::MouseButton;

#endif

SFMLWindow::SFMLWindow()
    : m_handle(nullptr)
    , m_event_handler(nullptr)
    , m_realized(false)
    , m_open(false)
    , m_active(true)
    , m_title("Application")
    , m_width(800)
    , m_height(600)
    , m_fullscreen(false)
    , m_fullscreen_stretch(false)
    , m_borderless(true)
    , m_mouse_capture_enabled(false)
    , m_vsync(false)
    , m_fps_limit(0)
    , m_windowed_width(0)
    , m_windowed_height(0)
    , m_nativeInstance{}
    , m_icon_resource_id(0)
{
}

SFMLWindow::~SFMLWindow()
{
    destroy();
}

bool SFMLWindow::realize()
{
    if (m_realized)
        return false;

    m_windowed_width = m_width;
    m_windowed_height = m_height;

    // Create SFML window from staged params
    sf::VideoMode videoMode({static_cast<unsigned int>(m_width), static_cast<unsigned int>(m_height)});

    sf::State state = m_fullscreen ? sf::State::Fullscreen : sf::State::Windowed;

    // Pick style: fullscreen/borderless use None, bordered uses Titlebar
    auto sfStyle = sf::Style::None;
    if (!m_fullscreen && !m_borderless)
    {
        sfStyle = sf::Style::Titlebar;
    }

    m_renderWindow.create(videoMode, m_title.c_str(), sfStyle, state);

    if (!m_renderWindow.isOpen())
        return false;

    // get native handle for anything that needs it
#ifdef _WIN32
    m_handle = static_cast<HWND>(m_renderWindow.getNativeHandle());

    // For bordered mode, add minimize button (sf::Style::Titlebar doesn't include it)
    if (!m_fullscreen && !m_borderless && m_handle)
    {
        LONG style = GetWindowLong(m_handle, GWL_STYLE);
        style |= WS_MINIMIZEBOX;
        SetWindowLong(m_handle, GWL_STYLE, style);
        SetWindowPos(m_handle, nullptr, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }
#endif

    // Hide the system mouse cursor (game draws its own cursor)
    m_renderWindow.setMouseCursorVisible(false);
    m_renderWindow.setMouseCursorGrabbed(m_mouse_capture_enabled);

    // Request 1ms timer resolution for accurate sleep-based frame limiting
#ifdef _WIN32
    timeBeginPeriod(1);
#endif

    m_realized = true;
    m_open = true;
    m_active = true;

    return true;
}

void SFMLWindow::destroy()
{
    m_event_handler = nullptr;

    if (m_renderWindow.isOpen())
    {
        m_renderWindow.close();
    }

    // Restore default timer resolution
#ifdef _WIN32
    timeEndPeriod(1);
#endif

    m_handle = nullptr;
    m_realized = false;
    m_open = false;
    m_active = false;
}

bool SFMLWindow::is_open() const
{
    return m_open && m_renderWindow.isOpen();
}

void SFMLWindow::close()
{
    // Don't call m_event_handler->on_close() here — that creates infinite recursion
    // since on_close() may call Window::close(). on_close() is only called from the
    // SFML event loop when the user clicks the close button.
    m_open = false;
    m_renderWindow.close();
}

hb::shared::types::NativeWindowHandle SFMLWindow::get_handle() const
{
    return m_handle;
}

int SFMLWindow::get_width() const
{
    return m_width;
}

int SFMLWindow::get_height() const
{
    return m_height;
}

bool SFMLWindow::is_fullscreen() const
{
    return m_fullscreen;
}

bool SFMLWindow::is_active() const
{
    return m_active;
}

void SFMLWindow::set_fullscreen(bool fullscreen)
{
    // Skip window recreation if mode hasn't actually changed
    // (SFML doesn't need surface restoration like DirectDraw did)
    if (m_realized && m_fullscreen == fullscreen)
        return;

    m_fullscreen = fullscreen;

    if (!m_realized)
        return;

    // get display dimensions based on mode
    int windowWidth, windowHeight;
    if (fullscreen)
    {
        // Save windowed dimensions before going fullscreen
        m_windowed_width = m_width;
        m_windowed_height = m_height;
        windowWidth = GetSystemMetrics(SM_CXSCREEN);
        windowHeight = GetSystemMetrics(SM_CYSCREEN);
    }
    else
    {
        // Restore saved windowed dimensions
        windowWidth = m_windowed_width > 0 ? m_windowed_width : LOGICAL_WIDTH();
        windowHeight = m_windowed_height > 0 ? m_windowed_height : LOGICAL_HEIGHT();
    }

    // Recreate window with new mode
    sf::VideoMode videoMode({static_cast<unsigned int>(windowWidth), static_cast<unsigned int>(windowHeight)});
    sf::State state = fullscreen ? sf::State::Fullscreen : sf::State::Windowed;

    // Pick style based on borderless setting (fullscreen always None)
    auto sfStyle = sf::Style::None;
    if (!fullscreen && !m_borderless)
    {
        sfStyle = sf::Style::Titlebar;
    }

    m_renderWindow.create(videoMode, m_title.c_str(), sfStyle, state);

    // Update stored dimensions
    m_width = windowWidth;
    m_height = windowHeight;

#ifdef _WIN32
    m_handle = static_cast<HWND>(m_renderWindow.getNativeHandle());

    // For bordered windowed mode, add minimize button
    if (!fullscreen && !m_borderless && m_handle)
    {
        LONG style = GetWindowLong(m_handle, GWL_STYLE);
        style |= WS_MINIMIZEBOX;
        SetWindowLong(m_handle, GWL_STYLE, style);
        SetWindowPos(m_handle, nullptr, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }
#endif

    // Reapply cursor settings
    m_renderWindow.setMouseCursorVisible(false);
    m_renderWindow.setMouseCursorGrabbed(m_mouse_capture_enabled);

    // If switching to windowed mode, center the window
    if (!fullscreen)
    {
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        int newX = (screenWidth - windowWidth) / 2;
        int newY = (screenHeight - windowHeight) / 2;
        SetWindowPos(m_handle, HWND_TOP, newX, newY, windowWidth, windowHeight, SWP_SHOWWINDOW);
    }
}

void SFMLWindow::set_borderless(bool borderless)
{
    m_borderless = borderless;

    if (!m_realized || m_fullscreen)
        return;

    // Recreate window with new style (same pattern as set_fullscreen)
    sf::VideoMode videoMode({static_cast<unsigned int>(m_width), static_cast<unsigned int>(m_height)});

    auto sfStyle = borderless ? sf::Style::None : sf::Style::Titlebar;

    m_renderWindow.create(videoMode, m_title.c_str(), sfStyle, sf::State::Windowed);

#ifdef _WIN32
    m_handle = static_cast<HWND>(m_renderWindow.getNativeHandle());

    // For bordered mode, add minimize button
    if (!borderless && m_handle)
    {
        LONG style = GetWindowLong(m_handle, GWL_STYLE);
        style |= WS_MINIMIZEBOX;
        SetWindowLong(m_handle, GWL_STYLE, style);
        SetWindowPos(m_handle, nullptr, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }
#endif


    m_renderWindow.setMouseCursorVisible(false);
    m_renderWindow.setMouseCursorGrabbed(m_mouse_capture_enabled);

    // Center the window
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int posX = (screenWidth - m_width) / 2;
    int posY = (screenHeight - m_height) / 2;
    SetWindowPos(m_handle, HWND_TOP, posX, posY, m_width, m_height, SWP_SHOWWINDOW);

    // Update input system with new window handle
    if (hb::shared::input::get())
    {
        SFMLInput* input = static_cast<SFMLInput*>(hb::shared::input::get());
        input->Initialize(m_handle);
        input->SetRenderWindow(&m_renderWindow);
    }

    // Update renderer with new render window
    hb::shared::render::IRenderer* renderer = hb::shared::render::Renderer::get();
    if (renderer)
    {
        static_cast<SFMLRenderer*>(renderer)->SetRenderWindow(&m_renderWindow);
    }
}

bool SFMLWindow::is_borderless() const
{
    return m_borderless;
}

void SFMLWindow::set_size(int width, int height, bool center)
{
    if (width <= 0 || height <= 0)
        return;

    m_width = width;
    m_height = height;

    if (!m_realized)
        return;

    // Update the SFML window size
    m_renderWindow.setSize({static_cast<unsigned int>(width), static_cast<unsigned int>(height)});

    // Update Win32 window position/size
    if (m_handle)
    {
        int posX = 0, posY = 0;
        if (center)
        {
            int screenWidth = GetSystemMetrics(SM_CXSCREEN);
            int screenHeight = GetSystemMetrics(SM_CYSCREEN);
            posX = (screenWidth - width) / 2;
            posY = (screenHeight - height) / 2;
            SetWindowPos(m_handle, HWND_TOP, posX, posY, width, height, SWP_SHOWWINDOW);
        }
        else
        {
            // Just resize, keep position
            SetWindowPos(m_handle, HWND_TOP, 0, 0, width, height, SWP_NOMOVE | SWP_SHOWWINDOW);
        }
    }

    // Re-apply mouse cursor grab to update grab boundaries to new window size
    m_renderWindow.setMouseCursorGrabbed(false);
    m_renderWindow.setMouseCursorGrabbed(m_mouse_capture_enabled);
}

void SFMLWindow::show()
{
    m_renderWindow.setVisible(true);
    m_renderWindow.requestFocus();
    // On Windows, also force foreground window to ensure focus
    if (m_handle)
    {
        SetForegroundWindow(m_handle);
        SetFocus(m_handle);
    }
}

void SFMLWindow::hide()
{
    m_renderWindow.setVisible(false);
}

void SFMLWindow::set_title(const char* title)
{
    if (title)
    {
        m_title = title;
        if (m_realized)
            m_renderWindow.setTitle(title);
    }
}

void SFMLWindow::set_framerate_limit(int limit)
{
    m_fps_limit = limit;
    if (!m_realized)
        return;
    // When VSync is active, the monitor refresh rate controls timing —
    // don't let an FPS limit override it. The saved m_fps_limit will be
    // applied when VSync is turned off (see set_vsync_enabled).
    if (m_vsync) return;
    // Forward to renderer for engine-owned frame limiting
    hb::shared::render::IRenderer* renderer = hb::shared::render::Renderer::get();
    if (renderer)
        static_cast<SFMLRenderer*>(renderer)->SetFramerateLimit(limit);
}

int SFMLWindow::get_framerate_limit() const
{
    return m_fps_limit;
}

void SFMLWindow::set_vsync_enabled(bool enabled)
{
    m_vsync = enabled;
    if (!m_realized)
        return;
    hb::shared::render::IRenderer* renderer = hb::shared::render::Renderer::get();
    if (!renderer) return;

    auto* sfml_renderer = static_cast<SFMLRenderer*>(renderer);

    if (enabled)
    {
        // Query the monitor's refresh rate and use it as the FPS target
        // This avoids SFML's blocking display() and keeps our engine-owned timing
        int refreshRate = 60; // Safe fallback
#ifdef _WIN32
        DEVMODE devMode = {};
        devMode.dmSize = sizeof(devMode);
        if (EnumDisplaySettings(nullptr, ENUM_CURRENT_SETTINGS, &devMode))
        {
            if (devMode.dmDisplayFrequency > 0)
                refreshRate = static_cast<int>(devMode.dmDisplayFrequency);
        }
#endif
        sfml_renderer->SetVSyncMode(true);
        sfml_renderer->SetFramerateLimit(refreshRate);
    }
    else
    {
        sfml_renderer->SetVSyncMode(false);
        // Restore the user's FPS limit setting
        sfml_renderer->SetFramerateLimit(m_fps_limit);
    }
}

bool SFMLWindow::is_vsync_enabled() const
{
    return m_vsync;
}

void SFMLWindow::set_fullscreen_stretch(bool stretch)
{
    m_fullscreen_stretch = stretch;
}

bool SFMLWindow::is_fullscreen_stretch() const
{
    return m_fullscreen_stretch;
}

void SFMLWindow::set_native_instance(hb::shared::types::NativeInstance instance)
{
    m_nativeInstance = instance;
}

void SFMLWindow::set_icon_resource_id(int id)
{
    m_icon_resource_id = id;
}

void SFMLWindow::set_mouse_cursor_visible(bool visible)
{
    if (m_realized)
        m_renderWindow.setMouseCursorVisible(visible);
}

void SFMLWindow::set_mouse_capture_enabled(bool enabled)
{
    m_mouse_capture_enabled = enabled;
    if (m_realized)
        m_renderWindow.setMouseCursorGrabbed(enabled);
}

void SFMLWindow::show_message_box(const char* title, const char* message)
{
#ifdef _WIN32
    MessageBox(m_handle, message, title, MB_ICONEXCLAMATION | MB_OK);
#else
    fprintf(stderr, "%s: %s\n", title, message);
#endif
}

bool SFMLWindow::process_messages()
{
    // Check if window was closed programmatically (via close() method)
    if (!m_open || !m_renderWindow.isOpen())
        return false;

    while (const std::optional<sf::Event> event = m_renderWindow.pollEvent())
    {
        if (event->is<sf::Event::Closed>())
        {
            // Notify the event handler — it decides whether to close or not
            if (m_event_handler)
                m_event_handler->on_close();
            // If the handler called close() or request_quit(), m_open is now false
            if (!m_open)
                return false;
            // Otherwise, the application chose to keep the window open (e.g., logout countdown)
            continue;
        }

        if (const auto* resized = event->getIf<sf::Event::Resized>())
        {
            m_width = static_cast<int>(resized->size.x);
            m_height = static_cast<int>(resized->size.y);
            if (m_event_handler)
                m_event_handler->on_resize(m_width, m_height);
        }

        if (const auto* focusGained = event->getIf<sf::Event::FocusGained>())
        {
            m_active = true;
            // Reactivate OpenGL context when focus is regained
            (void)m_renderWindow.setActive(true);
            // Re-grab mouse cursor if capture is enabled
            m_renderWindow.setMouseCursorGrabbed(m_mouse_capture_enabled);
            if (m_event_handler)
                m_event_handler->on_activate(true);
        }

        if (const auto* focusLost = event->getIf<sf::Event::FocusLost>())
        {
            m_active = false;
            if (m_event_handler)
                m_event_handler->on_activate(false);
        }

        if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
        {
            if (m_event_handler)
                m_event_handler->on_key_down(SfmlKeyToKeyCode(keyPressed->code));
        }

        if (const auto* keyReleased = event->getIf<sf::Event::KeyReleased>())
        {
            if (m_event_handler)
                m_event_handler->on_key_up(SfmlKeyToKeyCode(keyReleased->code));
        }

        if (const auto* textEntered = event->getIf<sf::Event::TextEntered>())
        {
            if (m_event_handler)
            {
                // Call on_text_input with WM_CHAR-style parameters
                // The game's GetText() function expects Win32 message format
                m_event_handler->on_text_input(m_handle, 0x0102 /*WM_CHAR*/,
                    static_cast<uintptr_t>(textEntered->unicode), 0);
            }
        }

        if (const auto* mouseMoved = event->getIf<sf::Event::MouseMoved>())
        {
            if (m_event_handler)
            {
                int logicalX, logicalY;
                TransformMouseCoords(mouseMoved->position.x, mouseMoved->position.y, logicalX, logicalY);
                m_event_handler->on_mouse_move(logicalX, logicalY);
            }
        }

        if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>())
        {
            int button = MouseButton::Left;
            if (mousePressed->button == sf::Mouse::Button::Right)
                button = MouseButton::Right;
            else if (mousePressed->button == sf::Mouse::Button::Middle)
                button = MouseButton::Middle;

            if (m_event_handler)
            {
                int logicalX, logicalY;
                TransformMouseCoords(mousePressed->position.x, mousePressed->position.y, logicalX, logicalY);
                m_event_handler->on_mouse_button_down(button, logicalX, logicalY);
            }
        }

        if (const auto* mouseReleased = event->getIf<sf::Event::MouseButtonReleased>())
        {
            int button = MouseButton::Left;
            if (mouseReleased->button == sf::Mouse::Button::Right)
                button = MouseButton::Right;
            else if (mouseReleased->button == sf::Mouse::Button::Middle)
                button = MouseButton::Middle;

            if (m_event_handler)
            {
                int logicalX, logicalY;
                TransformMouseCoords(mouseReleased->position.x, mouseReleased->position.y, logicalX, logicalY);
                m_event_handler->on_mouse_button_up(button, logicalX, logicalY);
            }
        }

        if (const auto* mouseWheel = event->getIf<sf::Event::MouseWheelScrolled>())
        {
            if (m_event_handler)
            {
                int logicalX, logicalY;
                TransformMouseCoords(mouseWheel->position.x, mouseWheel->position.y, logicalX, logicalY);
                int delta = static_cast<int>(mouseWheel->delta * 120);  // Convert to Windows-style delta
                m_event_handler->on_mouse_wheel(delta, logicalX, logicalY);
            }
        }
    }

    return true;
}

void SFMLWindow::wait_for_message()
{
    // Wait for an event and process it (don't discard it)
    // This is critical for handling FocusGained when the window is inactive
    if (const std::optional<sf::Event> event = m_renderWindow.waitEvent())
    {
        // Process the event we received (same logic as process_messages)
        if (event->is<sf::Event::Closed>())
        {
            if (m_event_handler)
                m_event_handler->on_close();
            if (!m_open)
                return;
        }

        if (const auto* focusGained = event->getIf<sf::Event::FocusGained>())
        {
            m_active = true;
            (void)m_renderWindow.setActive(true);
            m_renderWindow.setMouseCursorGrabbed(m_mouse_capture_enabled);
            if (m_event_handler)
                m_event_handler->on_activate(true);
        }

        if (const auto* focusLost = event->getIf<sf::Event::FocusLost>())
        {
            m_active = false;
            if (m_event_handler)
                m_event_handler->on_activate(false);
        }

        // Other events will be handled in the next process_messages() call
        // but focus events are critical to handle here for proper resume
    }
}

void SFMLWindow::set_event_handler(hb::shared::render::IWindowEventHandler* handler)
{
    m_event_handler = handler;
}

hb::shared::render::IWindowEventHandler* SFMLWindow::get_event_handler() const
{
    return m_event_handler;
}

// Convert SFML key codes to abstract KeyCode enum values
KeyCode SFMLWindow::SfmlKeyToKeyCode(sf::Keyboard::Key key)
{
    switch (key)
    {
    // Letters
    case sf::Keyboard::Key::A: return KeyCode::A;
    case sf::Keyboard::Key::B: return KeyCode::B;
    case sf::Keyboard::Key::C: return KeyCode::C;
    case sf::Keyboard::Key::D: return KeyCode::D;
    case sf::Keyboard::Key::E: return KeyCode::E;
    case sf::Keyboard::Key::F: return KeyCode::F;
    case sf::Keyboard::Key::G: return KeyCode::G;
    case sf::Keyboard::Key::H: return KeyCode::H;
    case sf::Keyboard::Key::I: return KeyCode::I;
    case sf::Keyboard::Key::J: return KeyCode::J;
    case sf::Keyboard::Key::K: return KeyCode::K;
    case sf::Keyboard::Key::L: return KeyCode::L;
    case sf::Keyboard::Key::M: return KeyCode::M;
    case sf::Keyboard::Key::N: return KeyCode::N;
    case sf::Keyboard::Key::O: return KeyCode::O;
    case sf::Keyboard::Key::P: return KeyCode::P;
    case sf::Keyboard::Key::Q: return KeyCode::Q;
    case sf::Keyboard::Key::R: return KeyCode::R;
    case sf::Keyboard::Key::S: return KeyCode::S;
    case sf::Keyboard::Key::T: return KeyCode::T;
    case sf::Keyboard::Key::U: return KeyCode::U;
    case sf::Keyboard::Key::V: return KeyCode::V;
    case sf::Keyboard::Key::W: return KeyCode::W;
    case sf::Keyboard::Key::X: return KeyCode::X;
    case sf::Keyboard::Key::Y: return KeyCode::Y;
    case sf::Keyboard::Key::Z: return KeyCode::Z;

    // Numbers
    case sf::Keyboard::Key::Num0: return KeyCode::Num0;
    case sf::Keyboard::Key::Num1: return KeyCode::Num1;
    case sf::Keyboard::Key::Num2: return KeyCode::Num2;
    case sf::Keyboard::Key::Num3: return KeyCode::Num3;
    case sf::Keyboard::Key::Num4: return KeyCode::Num4;
    case sf::Keyboard::Key::Num5: return KeyCode::Num5;
    case sf::Keyboard::Key::Num6: return KeyCode::Num6;
    case sf::Keyboard::Key::Num7: return KeyCode::Num7;
    case sf::Keyboard::Key::Num8: return KeyCode::Num8;
    case sf::Keyboard::Key::Num9: return KeyCode::Num9;

    // Special keys
    case sf::Keyboard::Key::Escape: return KeyCode::Escape;
    case sf::Keyboard::Key::Space: return KeyCode::Space;
    case sf::Keyboard::Key::Enter: return KeyCode::Enter;
    case sf::Keyboard::Key::Backspace: return KeyCode::Backspace;
    case sf::Keyboard::Key::Tab: return KeyCode::Tab;

    // Modifiers
    case sf::Keyboard::Key::LControl: return KeyCode::LControl;
    case sf::Keyboard::Key::LShift: return KeyCode::LShift;
    case sf::Keyboard::Key::LAlt: return KeyCode::LAlt;
    case sf::Keyboard::Key::RControl: return KeyCode::RControl;
    case sf::Keyboard::Key::RShift: return KeyCode::RShift;
    case sf::Keyboard::Key::RAlt: return KeyCode::RAlt;

    // Navigation
    case sf::Keyboard::Key::PageUp: return KeyCode::PageUp;
    case sf::Keyboard::Key::PageDown: return KeyCode::PageDown;
    case sf::Keyboard::Key::End: return KeyCode::End;
    case sf::Keyboard::Key::Home: return KeyCode::Home;
    case sf::Keyboard::Key::Insert: return KeyCode::Insert;
    case sf::Keyboard::Key::Delete: return KeyCode::Delete;

    // Arrow keys
    case sf::Keyboard::Key::Left: return KeyCode::Left;
    case sf::Keyboard::Key::Right: return KeyCode::Right;
    case sf::Keyboard::Key::Up: return KeyCode::Up;
    case sf::Keyboard::Key::Down: return KeyCode::Down;

    // Function keys
    case sf::Keyboard::Key::F1: return KeyCode::F1;
    case sf::Keyboard::Key::F2: return KeyCode::F2;
    case sf::Keyboard::Key::F3: return KeyCode::F3;
    case sf::Keyboard::Key::F4: return KeyCode::F4;
    case sf::Keyboard::Key::F5: return KeyCode::F5;
    case sf::Keyboard::Key::F6: return KeyCode::F6;
    case sf::Keyboard::Key::F7: return KeyCode::F7;
    case sf::Keyboard::Key::F8: return KeyCode::F8;
    case sf::Keyboard::Key::F9: return KeyCode::F9;
    case sf::Keyboard::Key::F10: return KeyCode::F10;
    case sf::Keyboard::Key::F11: return KeyCode::F11;
    case sf::Keyboard::Key::F12: return KeyCode::F12;

    // Numpad numbers
    case sf::Keyboard::Key::Numpad0: return KeyCode::Numpad0;
    case sf::Keyboard::Key::Numpad1: return KeyCode::Numpad1;
    case sf::Keyboard::Key::Numpad2: return KeyCode::Numpad2;
    case sf::Keyboard::Key::Numpad3: return KeyCode::Numpad3;
    case sf::Keyboard::Key::Numpad4: return KeyCode::Numpad4;
    case sf::Keyboard::Key::Numpad5: return KeyCode::Numpad5;
    case sf::Keyboard::Key::Numpad6: return KeyCode::Numpad6;
    case sf::Keyboard::Key::Numpad7: return KeyCode::Numpad7;
    case sf::Keyboard::Key::Numpad8: return KeyCode::Numpad8;
    case sf::Keyboard::Key::Numpad9: return KeyCode::Numpad9;

    // Numpad operators
    case sf::Keyboard::Key::Add: return KeyCode::NumpadAdd;
    case sf::Keyboard::Key::Subtract: return KeyCode::NumpadSubtract;
    case sf::Keyboard::Key::Multiply: return KeyCode::NumpadMultiply;
    case sf::Keyboard::Key::Divide: return KeyCode::NumpadDivide;

    // OEM keys
    case sf::Keyboard::Key::Grave: return KeyCode::Grave;

    default: return KeyCode::Unknown;
    }
}

void SFMLWindow::TransformMouseCoords(int windowX, int windowY, int& logicalX, int& logicalY) const
{
    // get actual window size in physical pixels
    float windowWidth, windowHeight;

#ifdef _WIN32
    if (m_handle)
    {
        RECT clientRect;
        if (GetClientRect(m_handle, &clientRect))
        {
            windowWidth = static_cast<float>(clientRect.right - clientRect.left);
            windowHeight = static_cast<float>(clientRect.bottom - clientRect.top);
        }
        else
        {
            windowWidth = static_cast<float>(m_width);
            windowHeight = static_cast<float>(m_height);
        }
    }
    else
#endif
    {
        windowWidth = static_cast<float>(m_width);
        windowHeight = static_cast<float>(m_height);
    }

    float mouseX = static_cast<float>(windowX);
    float mouseY = static_cast<float>(windowY);

    if (m_fullscreen && !m_fullscreen_stretch)
    {
        // Fullscreen letterbox - account for uniform scale and offset
        float scaleX = windowWidth / static_cast<float>(RENDER_LOGICAL_WIDTH());
        float scaleY = windowHeight / static_cast<float>(RENDER_LOGICAL_HEIGHT());
        float scale = (scaleY < scaleX) ? scaleY : scaleX;

        float destWidth = static_cast<float>(RENDER_LOGICAL_WIDTH()) * scale;
        float destHeight = static_cast<float>(RENDER_LOGICAL_HEIGHT()) * scale;
        float offsetX = (windowWidth - destWidth) / 2.0f;
        float offsetY = (windowHeight - destHeight) / 2.0f;

        logicalX = static_cast<int>((mouseX - offsetX) / scale);
        logicalY = static_cast<int>((mouseY - offsetY) / scale);
    }
    else
    {
        // Windowed or fullscreen stretch - independent axis scaling
        float scaleX = windowWidth / static_cast<float>(RENDER_LOGICAL_WIDTH());
        float scaleY = windowHeight / static_cast<float>(RENDER_LOGICAL_HEIGHT());

        logicalX = static_cast<int>(mouseX / scaleX);
        logicalY = static_cast<int>(mouseY / scaleY);
    }

    // Clamp to valid range
    if (logicalX < 0) logicalX = 0;
    if (logicalX > LOGICAL_MAX_X()) logicalX = LOGICAL_MAX_X();
    if (logicalY < 0) logicalY = 0;
    if (logicalY > LOGICAL_MAX_Y()) logicalY = LOGICAL_MAX_Y();
}

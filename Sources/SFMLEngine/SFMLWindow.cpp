// SFMLWindow.cpp: Pure SFML window implementing IWindow interface
//
// Part of SFMLEngine static library
//////////////////////////////////////////////////////////////////////

#include "SFMLWindow.h"
#include "ConfigManager.h"
#include "RenderConstants.h"
#include <SFML/Window/Event.hpp>

#ifdef _WIN32
#include <windows.h>
#endif

SFMLWindow::SFMLWindow()
    : m_hWnd(nullptr)
    , m_pEventHandler(nullptr)
    , m_width(0)
    , m_height(0)
    , m_fullscreen(false)
    , m_active(true)
    , m_open(false)
{
}

SFMLWindow::~SFMLWindow()
{
    Destroy();
}

bool SFMLWindow::Create(const WindowParams& params)
{
    if (m_open)
        return false;

    m_width = params.width;
    m_height = params.height;
    m_fullscreen = params.fullscreen;

    // Create SFML window
    sf::VideoMode videoMode({static_cast<unsigned int>(m_width), static_cast<unsigned int>(m_height)});

    sf::State state = params.fullscreen ? sf::State::Fullscreen : sf::State::Windowed;

    m_renderWindow.create(videoMode, params.title ? params.title : "Window", sf::Style::None, state);

    if (!m_renderWindow.isOpen())
        return false;

    // Get native handle for anything that needs it
#ifdef _WIN32
    m_hWnd = static_cast<HWND>(m_renderWindow.getNativeHandle());
#endif

    // Disable VSync by default (game has its own frame timing)
    m_renderWindow.setVerticalSyncEnabled(false);

    // Hide the system mouse cursor (game draws its own cursor)
    // Also grab the cursor to ensure mouse events are tracked even when hidden
    m_renderWindow.setMouseCursorVisible(false);
    m_renderWindow.setMouseCursorGrabbed(true);

    m_open = true;
    m_active = true;

    return true;
}

void SFMLWindow::Destroy()
{
    m_pEventHandler = nullptr;

    if (m_renderWindow.isOpen())
    {
        m_renderWindow.close();
    }

    m_hWnd = nullptr;
    m_open = false;
    m_active = false;
}

bool SFMLWindow::IsOpen() const
{
    return m_open && m_renderWindow.isOpen();
}

void SFMLWindow::Close()
{
    if (m_pEventHandler)
        m_pEventHandler->OnClose();
    m_open = false;
    m_renderWindow.close();
}

HWND SFMLWindow::GetHandle() const
{
    return m_hWnd;
}

int SFMLWindow::GetWidth() const
{
    return m_width;
}

int SFMLWindow::GetHeight() const
{
    return m_height;
}

bool SFMLWindow::IsFullscreen() const
{
    return m_fullscreen;
}

bool SFMLWindow::IsActive() const
{
    return m_active;
}

void SFMLWindow::SetFullscreen(bool fullscreen)
{
    if (m_fullscreen == fullscreen)
        return;

    m_fullscreen = fullscreen;

    // Get display dimensions based on mode
    int windowWidth, windowHeight;
    if (fullscreen)
    {
        // Use screen resolution for fullscreen
        windowWidth = GetSystemMetrics(SM_CXSCREEN);
        windowHeight = GetSystemMetrics(SM_CYSCREEN);
    }
    else
    {
        // Use configured window size for windowed mode
        windowWidth = ConfigManager::Get().GetWindowWidth();
        windowHeight = ConfigManager::Get().GetWindowHeight();
        if (windowWidth <= 0) windowWidth = LOGICAL_WIDTH;
        if (windowHeight <= 0) windowHeight = LOGICAL_HEIGHT;
    }

    // Recreate window with new mode
    sf::VideoMode videoMode({static_cast<unsigned int>(windowWidth), static_cast<unsigned int>(windowHeight)});
    sf::State state = fullscreen ? sf::State::Fullscreen : sf::State::Windowed;

    m_renderWindow.create(videoMode, "Helbreath", sf::Style::None, state);

    // Update stored dimensions
    m_width = windowWidth;
    m_height = windowHeight;

#ifdef _WIN32
    m_hWnd = static_cast<HWND>(m_renderWindow.getNativeHandle());
#endif

    // Disable VSync (game has its own frame timing)
    m_renderWindow.setVerticalSyncEnabled(false);

    // Hide the system mouse cursor (game draws its own cursor)
    // Also grab the cursor to ensure mouse events are tracked even when hidden
    m_renderWindow.setMouseCursorVisible(false);
    m_renderWindow.setMouseCursorGrabbed(true);

    // If switching to windowed mode, center the window
    if (!fullscreen)
    {
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        int newX = (screenWidth - windowWidth) / 2;
        int newY = (screenHeight - windowHeight) / 2;
        SetWindowPos(m_hWnd, HWND_TOP, newX, newY, windowWidth, windowHeight, SWP_SHOWWINDOW);
    }
}

void SFMLWindow::SetSize(int width, int height, bool center)
{
    if (width <= 0 || height <= 0)
        return;

    m_width = width;
    m_height = height;

    // Update the SFML window size
    m_renderWindow.setSize({static_cast<unsigned int>(width), static_cast<unsigned int>(height)});

    // Update Win32 window position/size
    if (m_hWnd)
    {
        int posX = 0, posY = 0;
        if (center)
        {
            int screenWidth = GetSystemMetrics(SM_CXSCREEN);
            int screenHeight = GetSystemMetrics(SM_CYSCREEN);
            posX = (screenWidth - width) / 2;
            posY = (screenHeight - height) / 2;
            SetWindowPos(m_hWnd, HWND_TOP, posX, posY, width, height, SWP_SHOWWINDOW);
        }
        else
        {
            // Just resize, keep position
            SetWindowPos(m_hWnd, HWND_TOP, 0, 0, width, height, SWP_NOMOVE | SWP_SHOWWINDOW);
        }
    }
}

void SFMLWindow::Show()
{
    m_renderWindow.setVisible(true);
}

void SFMLWindow::Hide()
{
    m_renderWindow.setVisible(false);
}

void SFMLWindow::SetTitle(const char* title)
{
    if (title)
    {
        m_renderWindow.setTitle(title);
    }
}

bool SFMLWindow::ProcessMessages()
{
    // Check if window was closed programmatically (via Close() method)
    if (!m_open || !m_renderWindow.isOpen())
        return false;

    while (const std::optional<sf::Event> event = m_renderWindow.pollEvent())
    {
        if (event->is<sf::Event::Closed>())
        {
            if (m_pEventHandler)
                m_pEventHandler->OnClose();
            m_open = false;
            return false;
        }

        if (const auto* resized = event->getIf<sf::Event::Resized>())
        {
            m_width = static_cast<int>(resized->size.x);
            m_height = static_cast<int>(resized->size.y);
            if (m_pEventHandler)
                m_pEventHandler->OnResize(m_width, m_height);
        }

        if (const auto* focusGained = event->getIf<sf::Event::FocusGained>())
        {
            m_active = true;
            // Reactivate OpenGL context when focus is regained
            (void)m_renderWindow.setActive(true);
            if (m_pEventHandler)
                m_pEventHandler->OnActivate(true);
        }

        if (const auto* focusLost = event->getIf<sf::Event::FocusLost>())
        {
            m_active = false;
            if (m_pEventHandler)
                m_pEventHandler->OnActivate(false);
        }

        if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>())
        {
            if (m_pEventHandler)
                m_pEventHandler->OnKeyDown(SfmlKeyToVirtualKey(keyPressed->code));
        }

        if (const auto* keyReleased = event->getIf<sf::Event::KeyReleased>())
        {
            if (m_pEventHandler)
                m_pEventHandler->OnKeyUp(SfmlKeyToVirtualKey(keyReleased->code));
        }

        if (const auto* textEntered = event->getIf<sf::Event::TextEntered>())
        {
            if (m_pEventHandler)
            {
                // Call OnTextInput with WM_CHAR-style parameters
                // The game's GetText() function expects Win32 message format
                m_pEventHandler->OnTextInput(m_hWnd, WM_CHAR,
                    static_cast<WPARAM>(textEntered->unicode), 0);
            }
        }

        if (const auto* mouseMoved = event->getIf<sf::Event::MouseMoved>())
        {
            if (m_pEventHandler)
            {
                int logicalX, logicalY;
                TransformMouseCoords(mouseMoved->position.x, mouseMoved->position.y, logicalX, logicalY);
                m_pEventHandler->OnMouseMove(logicalX, logicalY);
            }
        }

        if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>())
        {
            int button = MOUSE_BUTTON_LEFT;
            if (mousePressed->button == sf::Mouse::Button::Right)
                button = MOUSE_BUTTON_RIGHT;
            else if (mousePressed->button == sf::Mouse::Button::Middle)
                button = MOUSE_BUTTON_MIDDLE;

            if (m_pEventHandler)
            {
                int logicalX, logicalY;
                TransformMouseCoords(mousePressed->position.x, mousePressed->position.y, logicalX, logicalY);
                m_pEventHandler->OnMouseButtonDown(button, logicalX, logicalY);
            }
        }

        if (const auto* mouseReleased = event->getIf<sf::Event::MouseButtonReleased>())
        {
            int button = MOUSE_BUTTON_LEFT;
            if (mouseReleased->button == sf::Mouse::Button::Right)
                button = MOUSE_BUTTON_RIGHT;
            else if (mouseReleased->button == sf::Mouse::Button::Middle)
                button = MOUSE_BUTTON_MIDDLE;

            if (m_pEventHandler)
            {
                int logicalX, logicalY;
                TransformMouseCoords(mouseReleased->position.x, mouseReleased->position.y, logicalX, logicalY);
                m_pEventHandler->OnMouseButtonUp(button, logicalX, logicalY);
            }
        }

        if (const auto* mouseWheel = event->getIf<sf::Event::MouseWheelScrolled>())
        {
            if (m_pEventHandler)
            {
                int logicalX, logicalY;
                TransformMouseCoords(mouseWheel->position.x, mouseWheel->position.y, logicalX, logicalY);
                int delta = static_cast<int>(mouseWheel->delta * 120);  // Convert to Windows-style delta
                m_pEventHandler->OnMouseWheel(delta, logicalX, logicalY);
            }
        }
    }

    return true;
}

void SFMLWindow::WaitForMessage()
{
    // Wait for an event and process it (don't discard it)
    // This is critical for handling FocusGained when the window is inactive
    if (const std::optional<sf::Event> event = m_renderWindow.waitEvent())
    {
        // Process the event we received (same logic as ProcessMessages)
        if (event->is<sf::Event::Closed>())
        {
            if (m_pEventHandler)
                m_pEventHandler->OnClose();
            m_open = false;
            return;
        }

        if (const auto* focusGained = event->getIf<sf::Event::FocusGained>())
        {
            m_active = true;
            (void)m_renderWindow.setActive(true);
            if (m_pEventHandler)
                m_pEventHandler->OnActivate(true);
        }

        if (const auto* focusLost = event->getIf<sf::Event::FocusLost>())
        {
            m_active = false;
            if (m_pEventHandler)
                m_pEventHandler->OnActivate(false);
        }

        // Other events will be handled in the next ProcessMessages() call
        // but focus events are critical to handle here for proper resume
    }
}

void SFMLWindow::SetEventHandler(IWindowEventHandler* handler)
{
    m_pEventHandler = handler;
}

IWindowEventHandler* SFMLWindow::GetEventHandler() const
{
    return m_pEventHandler;
}

// Convert SFML key codes to Windows virtual key codes
int SFMLWindow::SfmlKeyToVirtualKey(sf::Keyboard::Key key)
{
    switch (key)
    {
    case sf::Keyboard::Key::A: return 'A';
    case sf::Keyboard::Key::B: return 'B';
    case sf::Keyboard::Key::C: return 'C';
    case sf::Keyboard::Key::D: return 'D';
    case sf::Keyboard::Key::E: return 'E';
    case sf::Keyboard::Key::F: return 'F';
    case sf::Keyboard::Key::G: return 'G';
    case sf::Keyboard::Key::H: return 'H';
    case sf::Keyboard::Key::I: return 'I';
    case sf::Keyboard::Key::J: return 'J';
    case sf::Keyboard::Key::K: return 'K';
    case sf::Keyboard::Key::L: return 'L';
    case sf::Keyboard::Key::M: return 'M';
    case sf::Keyboard::Key::N: return 'N';
    case sf::Keyboard::Key::O: return 'O';
    case sf::Keyboard::Key::P: return 'P';
    case sf::Keyboard::Key::Q: return 'Q';
    case sf::Keyboard::Key::R: return 'R';
    case sf::Keyboard::Key::S: return 'S';
    case sf::Keyboard::Key::T: return 'T';
    case sf::Keyboard::Key::U: return 'U';
    case sf::Keyboard::Key::V: return 'V';
    case sf::Keyboard::Key::W: return 'W';
    case sf::Keyboard::Key::X: return 'X';
    case sf::Keyboard::Key::Y: return 'Y';
    case sf::Keyboard::Key::Z: return 'Z';
    case sf::Keyboard::Key::Num0: return '0';
    case sf::Keyboard::Key::Num1: return '1';
    case sf::Keyboard::Key::Num2: return '2';
    case sf::Keyboard::Key::Num3: return '3';
    case sf::Keyboard::Key::Num4: return '4';
    case sf::Keyboard::Key::Num5: return '5';
    case sf::Keyboard::Key::Num6: return '6';
    case sf::Keyboard::Key::Num7: return '7';
    case sf::Keyboard::Key::Num8: return '8';
    case sf::Keyboard::Key::Num9: return '9';
    case sf::Keyboard::Key::Escape: return VK_ESCAPE;
    case sf::Keyboard::Key::LControl: return VK_LCONTROL;
    case sf::Keyboard::Key::LShift: return VK_LSHIFT;
    case sf::Keyboard::Key::LAlt: return VK_LMENU;
    case sf::Keyboard::Key::RControl: return VK_RCONTROL;
    case sf::Keyboard::Key::RShift: return VK_RSHIFT;
    case sf::Keyboard::Key::RAlt: return VK_RMENU;
    case sf::Keyboard::Key::Space: return VK_SPACE;
    case sf::Keyboard::Key::Enter: return VK_RETURN;
    case sf::Keyboard::Key::Backspace: return VK_BACK;
    case sf::Keyboard::Key::Tab: return VK_TAB;
    case sf::Keyboard::Key::PageUp: return VK_PRIOR;
    case sf::Keyboard::Key::PageDown: return VK_NEXT;
    case sf::Keyboard::Key::End: return VK_END;
    case sf::Keyboard::Key::Home: return VK_HOME;
    case sf::Keyboard::Key::Insert: return VK_INSERT;
    case sf::Keyboard::Key::Delete: return VK_DELETE;
    case sf::Keyboard::Key::Left: return VK_LEFT;
    case sf::Keyboard::Key::Right: return VK_RIGHT;
    case sf::Keyboard::Key::Up: return VK_UP;
    case sf::Keyboard::Key::Down: return VK_DOWN;
    case sf::Keyboard::Key::F1: return VK_F1;
    case sf::Keyboard::Key::F2: return VK_F2;
    case sf::Keyboard::Key::F3: return VK_F3;
    case sf::Keyboard::Key::F4: return VK_F4;
    case sf::Keyboard::Key::F5: return VK_F5;
    case sf::Keyboard::Key::F6: return VK_F6;
    case sf::Keyboard::Key::F7: return VK_F7;
    case sf::Keyboard::Key::F8: return VK_F8;
    case sf::Keyboard::Key::F9: return VK_F9;
    case sf::Keyboard::Key::F10: return VK_F10;
    case sf::Keyboard::Key::F11: return VK_F11;
    case sf::Keyboard::Key::F12: return VK_F12;
    case sf::Keyboard::Key::Numpad0: return VK_NUMPAD0;
    case sf::Keyboard::Key::Numpad1: return VK_NUMPAD1;
    case sf::Keyboard::Key::Numpad2: return VK_NUMPAD2;
    case sf::Keyboard::Key::Numpad3: return VK_NUMPAD3;
    case sf::Keyboard::Key::Numpad4: return VK_NUMPAD4;
    case sf::Keyboard::Key::Numpad5: return VK_NUMPAD5;
    case sf::Keyboard::Key::Numpad6: return VK_NUMPAD6;
    case sf::Keyboard::Key::Numpad7: return VK_NUMPAD7;
    case sf::Keyboard::Key::Numpad8: return VK_NUMPAD8;
    case sf::Keyboard::Key::Numpad9: return VK_NUMPAD9;
    default: return 0;
    }
}

void SFMLWindow::TransformMouseCoords(int windowX, int windowY, int& logicalX, int& logicalY) const
{
    // Get actual window size in physical pixels
    float windowWidth, windowHeight;

#ifdef _WIN32
    if (m_hWnd)
    {
        RECT clientRect;
        if (GetClientRect(m_hWnd, &clientRect))
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

    if (m_fullscreen)
    {
        // Fullscreen with letterboxing - need to account for scale and offset
        float scaleX = windowWidth / static_cast<float>(RENDER_LOGICAL_WIDTH);
        float scaleY = windowHeight / static_cast<float>(RENDER_LOGICAL_HEIGHT);
        float scale = (scaleY < scaleX) ? scaleY : scaleX;

        float destWidth = static_cast<float>(RENDER_LOGICAL_WIDTH) * scale;
        float destHeight = static_cast<float>(RENDER_LOGICAL_HEIGHT) * scale;
        float offsetX = (windowWidth - destWidth) / 2.0f;
        float offsetY = (windowHeight - destHeight) / 2.0f;

        // Transform from window coords to logical coords
        logicalX = static_cast<int>((mouseX - offsetX) / scale);
        logicalY = static_cast<int>((mouseY - offsetY) / scale);
    }
    else
    {
        // Windowed mode - simple scaling
        float scaleX = windowWidth / static_cast<float>(RENDER_LOGICAL_WIDTH);
        float scaleY = windowHeight / static_cast<float>(RENDER_LOGICAL_HEIGHT);

        logicalX = static_cast<int>(mouseX / scaleX);
        logicalY = static_cast<int>(mouseY / scaleY);
    }

    // Clamp to valid range
    if (logicalX < 0) logicalX = 0;
    if (logicalX > LOGICAL_MAX_X) logicalX = LOGICAL_MAX_X;
    if (logicalY < 0) logicalY = 0;
    if (logicalY > LOGICAL_MAX_Y) logicalY = LOGICAL_MAX_Y;
}

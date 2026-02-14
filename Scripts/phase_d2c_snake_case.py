#!/usr/bin/env python3
"""
Phase D2c: Snake-case rename of IWindow, IWindowEventHandler, Window static,
           IInput, and hb::shared::input:: APIs.

Mode 2 justified: touches 30+ files with mechanical renames.

This script handles ALL renames across ALL files (interfaces + consumers).
Structural changes (staging/realize) are done manually afterward.
"""

import os
import sys
import subprocess

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))  # Z:\Helbreath-3.82
BAK_PY = os.path.join(ROOT, "Scripts", "bak.py")
PYTHON = "/c/Python314/python"

SEARCH_DIRS = [
    os.path.join(ROOT, "Sources", "Client"),
    os.path.join(ROOT, "Sources", "SFMLEngine"),
    os.path.join(ROOT, "Sources", "Dependencies", "Shared", "Render"),
]

SKIP_SUFFIXES = [".bak_", ".bak"]

# ============================================================================
# Replacement rules, ordered longest-match-first within each category.
# Each tuple: (old_string, new_string)
# Applied as simple str.replace() — no regex needed for these.
# ============================================================================

REPLACEMENTS = [
    # ======== hb::shared::input:: fully-qualified namespace functions ========
    # (Longest first to prevent partial matches)
    ("hb::shared::input::IsRightClickInRect(", "hb::shared::input::is_right_click_in_rect("),
    ("hb::shared::input::IsMouseButtonReleased(", "hb::shared::input::is_mouse_button_released("),
    ("hb::shared::input::IsMouseButtonPressed(", "hb::shared::input::is_mouse_button_pressed("),
    ("hb::shared::input::IsMouseButtonDown(", "hb::shared::input::is_mouse_button_down("),
    ("hb::shared::input::ResetMouseWheelDelta(", "hb::shared::input::reset_mouse_wheel_delta("),
    ("hb::shared::input::GetMouseWheelDelta(", "hb::shared::input::get_mouse_wheel_delta("),
    ("hb::shared::input::IsMouseInRect(", "hb::shared::input::is_mouse_in_rect("),
    ("hb::shared::input::IsClickInRect(", "hb::shared::input::is_click_in_rect("),
    ("hb::shared::input::IsWindowActive(", "hb::shared::input::is_window_active("),
    ("hb::shared::input::IsKeyReleased(", "hb::shared::input::is_key_released("),
    ("hb::shared::input::SetSuppressed(", "hb::shared::input::set_suppressed("),
    ("hb::shared::input::IsKeyPressed(", "hb::shared::input::is_key_pressed("),
    ("hb::shared::input::IsSuppressed(", "hb::shared::input::is_suppressed("),
    ("hb::shared::input::PointInRect(", "hb::shared::input::point_in_rect("),
    ("hb::shared::input::BeginFrame(", "hb::shared::input::begin_frame("),
    ("hb::shared::input::IsShiftDown(", "hb::shared::input::is_shift_down("),
    ("hb::shared::input::IsKeyDown(", "hb::shared::input::is_key_down("),
    ("hb::shared::input::IsCtrlDown(", "hb::shared::input::is_ctrl_down("),
    ("hb::shared::input::IsAltDown(", "hb::shared::input::is_alt_down("),
    ("hb::shared::input::GetMouseX(", "hb::shared::input::get_mouse_x("),
    ("hb::shared::input::GetMouseY(", "hb::shared::input::get_mouse_y("),
    ("hb::shared::input::Destroy(", "hb::shared::input::destroy("),
    ("hb::shared::input::Create(", "hb::shared::input::create("),
    ("hb::shared::input::Get(", "hb::shared::input::get("),

    # ======== IInput virtual method calls via get()-> ========
    # These must come AFTER the Get→get rename above.
    # After renaming, the code reads: hb::shared::input::get()->OnKeyDown(
    # We need to also rename the method on the returned IInput*.
    # Use get()-> prefix for safety (only IInput is accessed this way).
    ("get()->SetWindowActive(", "get()->set_window_active("),
    ("get()->OnMouseWheel(", "get()->on_mouse_wheel("),
    ("get()->OnMouseMove(", "get()->on_mouse_move("),
    ("get()->OnMouseDown(", "get()->on_mouse_down("),
    ("get()->OnMouseUp(", "get()->on_mouse_up("),
    ("get()->OnKeyDown(", "get()->on_key_down("),
    ("get()->OnKeyUp(", "get()->on_key_up("),
    ("get()->BeginFrame(", "get()->begin_frame("),

    # ======== IInput convenience function declarations (inside namespace block in IInput.h) ========
    # These are unqualified: "inline bool IsKeyDown(" etc.
    # Only match in IInput.h since they're inside the namespace block.
    # We handle these with a separate per-file approach below.

    # ======== Window:: static class methods ========
    ("Window::ShowError(", "Window::show_error("),
    ("Window::GetHandle(", "Window::get_handle("),
    ("Window::SetBorderless(", "Window::set_borderless("),
    ("Window::IsActive(", "Window::is_active("),
    ("Window::SetSize(", "Window::set_size("),
    ("Window::Destroy(", "Window::destroy("),
    ("Window::Create(", "Window::create("),
    ("Window::Close(", "Window::close("),
    ("Window::Get(", "Window::get("),

    # ======== WindowParams struct ========
    ("WindowParams", "window_params"),
    # Struct member renames (only camelCase ones need changing)
    ("mouseCaptureEnabled", "mouse_capture_enabled"),
    ("nativeInstance", "native_instance"),
    ("iconResourceId", "icon_resource_id"),
]

# ============================================================================
# IWindow/IWindowEventHandler method renames — applied via TARGETED patterns
# to avoid renaming CGame's OnKeyDown/OnKeyUp/etc.
# These patterns are unique enough when using specific prefixes.
# ============================================================================

# IWindowEventHandler dispatch calls in SFMLWindow (m_pEventHandler->OnXxx)
EVENT_HANDLER_REPLACEMENTS = [
    ("m_pEventHandler->OnClose(", "m_pEventHandler->on_close("),
    ("m_pEventHandler->OnDestroy(", "m_pEventHandler->on_destroy("),
    ("m_pEventHandler->OnActivate(", "m_pEventHandler->on_activate("),
    ("m_pEventHandler->OnResize(", "m_pEventHandler->on_resize("),
    ("m_pEventHandler->OnKeyDown(", "m_pEventHandler->on_key_down("),
    ("m_pEventHandler->OnKeyUp(", "m_pEventHandler->on_key_up("),
    ("m_pEventHandler->OnChar(", "m_pEventHandler->on_char("),
    ("m_pEventHandler->OnMouseMove(", "m_pEventHandler->on_mouse_move("),
    ("m_pEventHandler->OnMouseButtonDown(", "m_pEventHandler->on_mouse_button_down("),
    ("m_pEventHandler->OnMouseButtonUp(", "m_pEventHandler->on_mouse_button_up("),
    ("m_pEventHandler->OnMouseWheel(", "m_pEventHandler->on_mouse_wheel("),
    ("m_pEventHandler->OnCustomMessage(", "m_pEventHandler->on_custom_message("),
    ("m_pEventHandler->OnTextInput(", "m_pEventHandler->on_text_input("),
]

# IWindow instance method renames via Window::get()-> chain
# These are applied AFTER the Window::Get→get rename.
WINDOW_INSTANCE_REPLACEMENTS = [
    # Methods called via Window::get()->Method
    ("Window::get()->SetFullscreenStretch(", "Window::get()->set_fullscreen_stretch("),
    ("Window::get()->IsFullscreenStretch(", "Window::get()->is_fullscreen_stretch("),
    ("Window::get()->SetMouseCursorVisible(", "Window::get()->set_mouse_cursor_visible("),
    ("Window::get()->SetMouseCaptureEnabled(", "Window::get()->set_mouse_capture_enabled("),
    ("Window::get()->SetEventHandler(", "Window::get()->set_event_handler("),
    ("Window::get()->GetEventHandler(", "Window::get()->get_event_handler("),
    ("Window::get()->SetFramerateLimit(", "Window::get()->set_framerate_limit("),
    ("Window::get()->GetFramerateLimit(", "Window::get()->get_framerate_limit("),
    ("Window::get()->SetVSyncEnabled(", "Window::get()->set_vsync_enabled("),
    ("Window::get()->IsVSyncEnabled(", "Window::get()->is_vsync_enabled("),
    ("Window::get()->ProcessMessages(", "Window::get()->process_messages("),
    ("Window::get()->ShowMessageBox(", "Window::get()->show_message_box("),
    ("Window::get()->WaitForMessage(", "Window::get()->wait_for_message("),
    ("Window::get()->SetBorderless(", "Window::get()->set_borderless("),
    ("Window::get()->SetFullscreen(", "Window::get()->set_fullscreen("),
    ("Window::get()->IsFullscreen(", "Window::get()->is_fullscreen("),
    ("Window::get()->IsBorderless(", "Window::get()->is_borderless("),
    ("Window::get()->GetHandle(", "Window::get()->get_handle("),
    ("Window::get()->GetWidth(", "Window::get()->get_width("),
    ("Window::get()->GetHeight(", "Window::get()->get_height("),
    ("Window::get()->IsActive(", "Window::get()->is_active("),
    ("Window::get()->SetSize(", "Window::get()->set_size("),
    ("Window::get()->SetTitle(", "Window::get()->set_title("),
    ("Window::get()->Show(", "Window::get()->show("),
    ("Window::get()->Hide(", "Window::get()->hide("),
    ("Window::get()->Close(", "Window::get()->close("),
    ("Window::get()->IsOpen(", "Window::get()->is_open("),
    ("Window::get()->Create(", "Window::get()->create("),
    ("Window::get()->Destroy(", "Window::get()->destroy("),

    # Methods called via local IWindow* variable (window->Method)
    # These are used in Wmain.cpp and RendererFactory.cpp
    ("window->ProcessMessages(", "window->process_messages("),
    ("window->SetEventHandler(", "window->set_event_handler("),
    ("window->SetVSyncEnabled(", "window->set_vsync_enabled("),
    ("window->SetFramerateLimit(", "window->set_framerate_limit("),
    ("window->SetFullscreenStretch(", "window->set_fullscreen_stretch("),
    ("window->Show(", "window->show("),

    # Methods called via s_pWindow-> in RendererFactory.cpp
    ("s_pWindow->Create(", "s_pWindow->create("),
    ("s_pWindow->Close(", "s_pWindow->close("),
    ("s_pWindow->GetHandle(", "s_pWindow->get_handle("),
    ("s_pWindow->IsActive(", "s_pWindow->is_active("),
    ("s_pWindow->SetSize(", "s_pWindow->set_size("),
    ("s_pWindow->SetBorderless(", "s_pWindow->set_borderless("),
    ("s_pWindow->ShowMessageBox(", "s_pWindow->show_message_box("),

    # Also handle: pWindow-> in RendererFactory.cpp Renderer::Set()
    # "SFMLWindow* sfmlWindow = ..." then sfmlWindow->GetRenderWindow() — this stays (SFML-specific, not IWindow)
]

# ============================================================================
# IWindow.h interface + IWindowEventHandler declarations
# These are the virtual method declarations — rename in-place.
# ============================================================================

IWINDOW_VIRTUAL_REPLACEMENTS = [
    # IWindow virtual methods
    ("virtual bool Create(const hb::shared::render::window_params& params) = 0;",
     "virtual bool create(const hb::shared::render::window_params& params) = 0;"),
    ("virtual void Destroy() = 0;", "virtual void destroy() = 0;"),
    ("virtual bool IsOpen() const = 0;", "virtual bool is_open() const = 0;"),
    ("virtual void Close() = 0;", "virtual void close() = 0;"),
    ("virtual hb::shared::types::NativeWindowHandle GetHandle() const = 0;",
     "virtual hb::shared::types::NativeWindowHandle get_handle() const = 0;"),
    ("virtual int GetWidth() const = 0;", "virtual int get_width() const = 0;"),
    ("virtual int GetHeight() const = 0;", "virtual int get_height() const = 0;"),
    ("virtual bool IsFullscreen() const = 0;", "virtual bool is_fullscreen() const = 0;"),
    ("virtual bool IsActive() const = 0;", "virtual bool is_active() const = 0;"),
    ("virtual void SetFullscreen(bool fullscreen) = 0;", "virtual void set_fullscreen(bool fullscreen) = 0;"),
    ("virtual void SetBorderless(bool borderless) = 0;", "virtual void set_borderless(bool borderless) = 0;"),
    ("virtual bool IsBorderless() const = 0;", "virtual bool is_borderless() const = 0;"),
    ("virtual void SetSize(int width, int height, bool center = true) = 0;",
     "virtual void set_size(int width, int height, bool center = true) = 0;"),
    ("virtual void Show() = 0;", "virtual void show() = 0;"),
    ("virtual void Hide() = 0;", "virtual void hide() = 0;"),
    ("virtual void SetTitle(const char* title) = 0;", "virtual void set_title(const char* title) = 0;"),
    ("virtual void SetFramerateLimit(int limit) = 0;", "virtual void set_framerate_limit(int limit) = 0;"),
    ("virtual int GetFramerateLimit() const = 0;", "virtual int get_framerate_limit() const = 0;"),
    ("virtual void SetVSyncEnabled(bool enabled) = 0;", "virtual void set_vsync_enabled(bool enabled) = 0;"),
    ("virtual bool IsVSyncEnabled() const = 0;", "virtual bool is_vsync_enabled() const = 0;"),
    ("virtual void SetFullscreenStretch(bool stretch) = 0;", "virtual void set_fullscreen_stretch(bool stretch) = 0;"),
    ("virtual bool IsFullscreenStretch() const = 0;", "virtual bool is_fullscreen_stretch() const = 0;"),
    ("virtual void SetMouseCursorVisible(bool visible) = 0;", "virtual void set_mouse_cursor_visible(bool visible) = 0;"),
    ("virtual void SetMouseCaptureEnabled(bool enabled) = 0;", "virtual void set_mouse_capture_enabled(bool enabled) = 0;"),
    ("virtual void ShowMessageBox(const char* title, const char* message) = 0;",
     "virtual void show_message_box(const char* title, const char* message) = 0;"),
    ("virtual bool ProcessMessages() = 0;", "virtual bool process_messages() = 0;"),
    ("virtual void WaitForMessage() = 0;", "virtual void wait_for_message() = 0;"),
    ("virtual void SetEventHandler(IWindowEventHandler* handler) = 0;",
     "virtual void set_event_handler(IWindowEventHandler* handler) = 0;"),
    ("virtual IWindowEventHandler* GetEventHandler() const = 0;",
     "virtual IWindowEventHandler* get_event_handler() const = 0;"),

    # IWindowEventHandler virtual methods
    ("virtual void OnClose() = 0;", "virtual void on_close() = 0;"),
    ("virtual void OnDestroy() = 0;", "virtual void on_destroy() = 0;"),
    ("virtual void OnActivate(bool active) = 0;", "virtual void on_activate(bool active) = 0;"),
    ("virtual void OnResize(int width, int height) = 0;", "virtual void on_resize(int width, int height) = 0;"),
    ("virtual void OnKeyDown(KeyCode keyCode) = 0;", "virtual void on_key_down(KeyCode key) = 0;"),
    ("virtual void OnKeyUp(KeyCode keyCode) = 0;", "virtual void on_key_up(KeyCode key) = 0;"),
    ("virtual void OnChar(char character) = 0;", "virtual void on_char(char character) = 0;"),
    ("virtual void OnMouseMove(int x, int y) = 0;", "virtual void on_mouse_move(int x, int y) = 0;"),
    ("virtual void OnMouseButtonDown(int button, int x, int y) = 0;",
     "virtual void on_mouse_button_down(int button, int x, int y) = 0;"),
    ("virtual void OnMouseButtonUp(int button, int x, int y) = 0;",
     "virtual void on_mouse_button_up(int button, int x, int y) = 0;"),
    ("virtual void OnMouseWheel(int delta, int x, int y) = 0;",
     "virtual void on_mouse_wheel(int delta, int x, int y) = 0;"),
    ("virtual bool OnCustomMessage(uint32_t message, uintptr_t wParam, intptr_t lParam) = 0;",
     "virtual bool on_custom_message(uint32_t message, uintptr_t wparam, intptr_t lparam) = 0;"),
    ("virtual bool OnTextInput(hb::shared::types::NativeWindowHandle hWnd, uint32_t message, uintptr_t wParam, intptr_t lParam) = 0;",
     "virtual bool on_text_input(hb::shared::types::NativeWindowHandle hwnd, uint32_t message, uintptr_t wparam, intptr_t lparam) = 0;"),
]

# ============================================================================
# SFMLWindow.h override declarations
# ============================================================================

SFMLWINDOW_H_REPLACEMENTS = [
    ("bool Create(const hb::shared::render::window_params& params) override;",
     "bool create(const hb::shared::render::window_params& params) override;"),
    ("void Destroy() override;", "void destroy() override;"),
    ("bool IsOpen() const override;", "bool is_open() const override;"),
    ("void Close() override;", "void close() override;"),
    ("hb::shared::types::NativeWindowHandle GetHandle() const override;",
     "hb::shared::types::NativeWindowHandle get_handle() const override;"),
    ("int GetWidth() const override;", "int get_width() const override;"),
    ("int GetHeight() const override;", "int get_height() const override;"),
    ("bool IsFullscreen() const override;", "bool is_fullscreen() const override;"),
    ("bool IsActive() const override;", "bool is_active() const override;"),
    ("void SetFullscreen(bool fullscreen) override;", "void set_fullscreen(bool fullscreen) override;"),
    ("void SetBorderless(bool borderless) override;", "void set_borderless(bool borderless) override;"),
    ("bool IsBorderless() const override;", "bool is_borderless() const override;"),
    ("void SetSize(int width, int height, bool center = true) override;",
     "void set_size(int width, int height, bool center = true) override;"),
    ("void Show() override;", "void show() override;"),
    ("void Hide() override;", "void hide() override;"),
    ("void SetTitle(const char* title) override;", "void set_title(const char* title) override;"),
    ("void SetFramerateLimit(int limit) override;", "void set_framerate_limit(int limit) override;"),
    ("int GetFramerateLimit() const override;", "int get_framerate_limit() const override;"),
    ("void SetVSyncEnabled(bool enabled) override;", "void set_vsync_enabled(bool enabled) override;"),
    ("bool IsVSyncEnabled() const override;", "bool is_vsync_enabled() const override;"),
    ("void SetFullscreenStretch(bool stretch) override;", "void set_fullscreen_stretch(bool stretch) override;"),
    ("bool IsFullscreenStretch() const override;", "bool is_fullscreen_stretch() const override;"),
    ("void SetMouseCursorVisible(bool visible) override;", "void set_mouse_cursor_visible(bool visible) override;"),
    ("void SetMouseCaptureEnabled(bool enabled) override;", "void set_mouse_capture_enabled(bool enabled) override;"),
    ("void ShowMessageBox(const char* title, const char* message) override;",
     "void show_message_box(const char* title, const char* message) override;"),
    ("bool ProcessMessages() override;", "bool process_messages() override;"),
    ("void WaitForMessage() override;", "void wait_for_message() override;"),
    ("void SetEventHandler(hb::shared::render::IWindowEventHandler* handler) override;",
     "void set_event_handler(hb::shared::render::IWindowEventHandler* handler) override;"),
    ("hb::shared::render::IWindowEventHandler* GetEventHandler() const override;",
     "hb::shared::render::IWindowEventHandler* get_event_handler() const override;"),
    # Destructor calls
    ("Destroy();", "destroy();"),
]

# ============================================================================
# SFMLWindow.cpp method implementations (SFMLWindow::MethodName)
# ============================================================================

SFMLWINDOW_CPP_REPLACEMENTS = [
    ("SFMLWindow::Create(", "SFMLWindow::create("),
    ("SFMLWindow::Destroy(", "SFMLWindow::destroy("),
    ("SFMLWindow::IsOpen(", "SFMLWindow::is_open("),
    ("SFMLWindow::Close(", "SFMLWindow::close("),
    ("SFMLWindow::GetHandle(", "SFMLWindow::get_handle("),
    ("SFMLWindow::GetWidth(", "SFMLWindow::get_width("),
    ("SFMLWindow::GetHeight(", "SFMLWindow::get_height("),
    ("SFMLWindow::IsFullscreen(", "SFMLWindow::is_fullscreen("),
    ("SFMLWindow::IsActive(", "SFMLWindow::is_active("),
    ("SFMLWindow::SetFullscreen(", "SFMLWindow::set_fullscreen("),
    ("SFMLWindow::SetBorderless(", "SFMLWindow::set_borderless("),
    ("SFMLWindow::IsBorderless(", "SFMLWindow::is_borderless("),
    ("SFMLWindow::SetSize(", "SFMLWindow::set_size("),
    ("SFMLWindow::Show(", "SFMLWindow::show("),
    ("SFMLWindow::Hide(", "SFMLWindow::hide("),
    ("SFMLWindow::SetTitle(", "SFMLWindow::set_title("),
    ("SFMLWindow::SetFramerateLimit(", "SFMLWindow::set_framerate_limit("),
    ("SFMLWindow::GetFramerateLimit(", "SFMLWindow::get_framerate_limit("),
    ("SFMLWindow::SetVSyncEnabled(", "SFMLWindow::set_vsync_enabled("),
    ("SFMLWindow::IsVSyncEnabled(", "SFMLWindow::is_vsync_enabled("),
    ("SFMLWindow::SetFullscreenStretch(", "SFMLWindow::set_fullscreen_stretch("),
    ("SFMLWindow::IsFullscreenStretch(", "SFMLWindow::is_fullscreen_stretch("),
    ("SFMLWindow::SetMouseCursorVisible(", "SFMLWindow::set_mouse_cursor_visible("),
    ("SFMLWindow::SetMouseCaptureEnabled(", "SFMLWindow::set_mouse_capture_enabled("),
    ("SFMLWindow::ShowMessageBox(", "SFMLWindow::show_message_box("),
    ("SFMLWindow::ProcessMessages(", "SFMLWindow::process_messages("),
    ("SFMLWindow::WaitForMessage(", "SFMLWindow::wait_for_message("),
    ("SFMLWindow::SetEventHandler(", "SFMLWindow::set_event_handler("),
    ("SFMLWindow::GetEventHandler(", "SFMLWindow::get_event_handler("),
]

# ============================================================================
# SFMLInput.h override declarations
# ============================================================================

SFMLINPUT_H_REPLACEMENTS = [
    ("virtual void BeginFrame() override;", "virtual void begin_frame() override;"),
    ("virtual bool IsKeyDown(KeyCode key) const override;", "virtual bool is_key_down(KeyCode key) const override;"),
    ("virtual bool IsKeyPressed(KeyCode key) const override;", "virtual bool is_key_pressed(KeyCode key) const override;"),
    ("virtual bool IsKeyReleased(KeyCode key) const override;", "virtual bool is_key_released(KeyCode key) const override;"),
    ("virtual bool IsMouseButtonDown(int button) const override;", "virtual bool is_mouse_button_down(int button) const override;"),
    ("virtual bool IsMouseButtonPressed(int button) const override;", "virtual bool is_mouse_button_pressed(int button) const override;"),
    ("virtual bool IsMouseButtonReleased(int button) const override;", "virtual bool is_mouse_button_released(int button) const override;"),
    ("virtual int GetMouseX() const override;", "virtual int get_mouse_x() const override;"),
    ("virtual int GetMouseY() const override;", "virtual int get_mouse_y() const override;"),
    ("virtual int GetMouseWheelDelta() const override;", "virtual int get_mouse_wheel_delta() const override;"),
    ("virtual void ResetMouseWheelDelta() override;", "virtual void reset_mouse_wheel_delta() override;"),
    ("virtual bool IsShiftDown() const override;", "virtual bool is_shift_down() const override;"),
    ("virtual bool IsCtrlDown() const override;", "virtual bool is_ctrl_down() const override;"),
    ("virtual bool IsAltDown() const override;", "virtual bool is_alt_down() const override;"),
    ("virtual bool IsWindowActive() const override;", "virtual bool is_window_active() const override;"),
    ("virtual void SetWindowActive(bool active) override;", "virtual void set_window_active(bool active) override;"),
    ("virtual void SetSuppressed(bool suppressed) override;", "virtual void set_suppressed(bool suppressed) override;"),
    ("virtual bool IsSuppressed() const override;", "virtual bool is_suppressed() const override;"),
    ("virtual void OnKeyDown(KeyCode key) override;", "virtual void on_key_down(KeyCode key) override;"),
    ("virtual void OnKeyUp(KeyCode key) override;", "virtual void on_key_up(KeyCode key) override;"),
    ("virtual void OnMouseMove(int x, int y) override;", "virtual void on_mouse_move(int x, int y) override;"),
    ("virtual void OnMouseDown(int button) override;", "virtual void on_mouse_down(int button) override;"),
    ("virtual void OnMouseUp(int button) override;", "virtual void on_mouse_up(int button) override;"),
    ("virtual void OnMouseWheel(int delta) override;", "virtual void on_mouse_wheel(int delta) override;"),
]

# ============================================================================
# SFMLInput.cpp method implementations (SFMLInput::MethodName)
# ============================================================================

SFMLINPUT_CPP_REPLACEMENTS = [
    ("SFMLInput::BeginFrame(", "SFMLInput::begin_frame("),
    ("SFMLInput::IsKeyDown(", "SFMLInput::is_key_down("),
    ("SFMLInput::IsKeyPressed(", "SFMLInput::is_key_pressed("),
    ("SFMLInput::IsKeyReleased(", "SFMLInput::is_key_released("),
    ("SFMLInput::IsMouseButtonDown(", "SFMLInput::is_mouse_button_down("),
    ("SFMLInput::IsMouseButtonPressed(", "SFMLInput::is_mouse_button_pressed("),
    ("SFMLInput::IsMouseButtonReleased(", "SFMLInput::is_mouse_button_released("),
    ("SFMLInput::GetMouseX(", "SFMLInput::get_mouse_x("),
    ("SFMLInput::GetMouseY(", "SFMLInput::get_mouse_y("),
    ("SFMLInput::GetMouseWheelDelta(", "SFMLInput::get_mouse_wheel_delta("),
    ("SFMLInput::ResetMouseWheelDelta(", "SFMLInput::reset_mouse_wheel_delta("),
    ("SFMLInput::IsShiftDown(", "SFMLInput::is_shift_down("),
    ("SFMLInput::IsCtrlDown(", "SFMLInput::is_ctrl_down("),
    ("SFMLInput::IsAltDown(", "SFMLInput::is_alt_down("),
    ("SFMLInput::IsWindowActive(", "SFMLInput::is_window_active("),
    ("SFMLInput::SetWindowActive(", "SFMLInput::set_window_active("),
    ("SFMLInput::SetSuppressed(", "SFMLInput::set_suppressed("),
    ("SFMLInput::IsSuppressed(", "SFMLInput::is_suppressed("),
    ("SFMLInput::OnKeyDown(", "SFMLInput::on_key_down("),
    ("SFMLInput::OnKeyUp(", "SFMLInput::on_key_up("),
    ("SFMLInput::OnMouseMove(", "SFMLInput::on_mouse_move("),
    ("SFMLInput::OnMouseDown(", "SFMLInput::on_mouse_down("),
    ("SFMLInput::OnMouseUp(", "SFMLInput::on_mouse_up("),
    ("SFMLInput::OnMouseWheel(", "SFMLInput::on_mouse_wheel("),
]

# ============================================================================
# IInput.h convenience functions (declared inside namespace hb::shared::input {})
# These are unqualified since they're inside the namespace block.
# ============================================================================

IINPUT_CONVENIENCE_REPLACEMENTS = [
    # Function declarations/definitions (inside namespace block)
    ("void Create();", "void create();"),
    ("void Destroy();", "void destroy();"),
    ("IInput* Get();", "IInput* get();"),
    ("inline void BeginFrame() { Get()->BeginFrame(); }", "inline void begin_frame() { get()->begin_frame(); }"),
    # Virtual method declarations in IInput class
    ("virtual void BeginFrame() = 0;", "virtual void begin_frame() = 0;"),
    ("virtual bool IsKeyDown(KeyCode key) const = 0;", "virtual bool is_key_down(KeyCode key) const = 0;"),
    ("virtual bool IsKeyPressed(KeyCode key) const = 0;", "virtual bool is_key_pressed(KeyCode key) const = 0;"),
    ("virtual bool IsKeyReleased(KeyCode key) const = 0;", "virtual bool is_key_released(KeyCode key) const = 0;"),
    ("virtual bool IsMouseButtonDown(int button) const = 0;", "virtual bool is_mouse_button_down(int button) const = 0;"),
    ("virtual bool IsMouseButtonPressed(int button) const = 0;", "virtual bool is_mouse_button_pressed(int button) const = 0;"),
    ("virtual bool IsMouseButtonReleased(int button) const = 0;", "virtual bool is_mouse_button_released(int button) const = 0;"),
    ("virtual int GetMouseX() const = 0;", "virtual int get_mouse_x() const = 0;"),
    ("virtual int GetMouseY() const = 0;", "virtual int get_mouse_y() const = 0;"),
    ("virtual int GetMouseWheelDelta() const = 0;", "virtual int get_mouse_wheel_delta() const = 0;"),
    ("virtual void ResetMouseWheelDelta() = 0;", "virtual void reset_mouse_wheel_delta() = 0;"),
    ("virtual bool IsShiftDown() const = 0;", "virtual bool is_shift_down() const = 0;"),
    ("virtual bool IsCtrlDown() const = 0;", "virtual bool is_ctrl_down() const = 0;"),
    ("virtual bool IsAltDown() const = 0;", "virtual bool is_alt_down() const = 0;"),
    ("virtual bool IsWindowActive() const = 0;", "virtual bool is_window_active() const = 0;"),
    ("virtual void SetWindowActive(bool active) = 0;", "virtual void set_window_active(bool active) = 0;"),
    ("virtual void SetSuppressed(bool suppressed) = 0;", "virtual void set_suppressed(bool suppressed) = 0;"),
    ("virtual bool IsSuppressed() const = 0;", "virtual bool is_suppressed() const = 0;"),
    ("virtual void OnKeyDown(KeyCode key) = 0;", "virtual void on_key_down(KeyCode key) = 0;"),
    ("virtual void OnKeyUp(KeyCode key) = 0;", "virtual void on_key_up(KeyCode key) = 0;"),
    ("virtual void OnMouseMove(int x, int y) = 0;", "virtual void on_mouse_move(int x, int y) = 0;"),
    ("virtual void OnMouseDown(int button) = 0;", "virtual void on_mouse_down(int button) = 0;"),
    ("virtual void OnMouseUp(int button) = 0;", "virtual void on_mouse_up(int button) = 0;"),
    ("virtual void OnMouseWheel(int delta) = 0;", "virtual void on_mouse_wheel(int delta) = 0;"),
    # Convenience inline functions (match exact patterns in IInput.h)
    ("inline bool IsKeyDown(KeyCode key) { return Get()->IsKeyDown(key); }",
     "inline bool is_key_down(KeyCode key) { return get()->is_key_down(key); }"),
    ("inline bool IsKeyPressed(KeyCode key) { return Get()->IsKeyPressed(key); }",
     "inline bool is_key_pressed(KeyCode key) { return get()->is_key_pressed(key); }"),
    ("inline bool IsKeyReleased(KeyCode key) { return Get()->IsKeyReleased(key); }",
     "inline bool is_key_released(KeyCode key) { return get()->is_key_released(key); }"),
    ("inline bool IsMouseButtonDown(int btn) { return Get()->IsMouseButtonDown(btn); }",
     "inline bool is_mouse_button_down(int btn) { return get()->is_mouse_button_down(btn); }"),
    ("inline bool IsMouseButtonPressed(int btn) { return Get()->IsMouseButtonPressed(btn); }",
     "inline bool is_mouse_button_pressed(int btn) { return get()->is_mouse_button_pressed(btn); }"),
    ("inline bool IsMouseButtonReleased(int btn) { return Get()->IsMouseButtonReleased(btn); }",
     "inline bool is_mouse_button_released(int btn) { return get()->is_mouse_button_released(btn); }"),
    ("inline int GetMouseX() { return Get()->GetMouseX(); }",
     "inline int get_mouse_x() { return get()->get_mouse_x(); }"),
    ("inline int GetMouseY() { return Get()->GetMouseY(); }",
     "inline int get_mouse_y() { return get()->get_mouse_y(); }"),
    ("inline int GetMouseWheelDelta() { return Get()->GetMouseWheelDelta(); }",
     "inline int get_mouse_wheel_delta() { return get()->get_mouse_wheel_delta(); }"),
    ("inline void ResetMouseWheelDelta() { Get()->ResetMouseWheelDelta(); }",
     "inline void reset_mouse_wheel_delta() { get()->reset_mouse_wheel_delta(); }"),
    ("inline bool IsShiftDown() { return Get()->IsShiftDown(); }",
     "inline bool is_shift_down() { return get()->is_shift_down(); }"),
    ("inline bool IsCtrlDown() { return Get()->IsCtrlDown(); }",
     "inline bool is_ctrl_down() { return get()->is_ctrl_down(); }"),
    ("inline bool IsAltDown() { return Get()->IsAltDown(); }",
     "inline bool is_alt_down() { return get()->is_alt_down(); }"),
    ("inline bool IsWindowActive() { return Get()->IsWindowActive(); }",
     "inline bool is_window_active() { return get()->is_window_active(); }"),
    ("inline void SetSuppressed(bool suppressed) { Get()->SetSuppressed(suppressed); }",
     "inline void set_suppressed(bool suppressed) { get()->set_suppressed(suppressed); }"),
    ("inline bool IsSuppressed() { return Get()->IsSuppressed(); }",
     "inline bool is_suppressed() { return get()->is_suppressed(); }"),
    # Hit-testing helpers
    ("inline bool PointInRect(", "inline bool point_in_rect("),
    ("inline bool IsMouseInRect(", "inline bool is_mouse_in_rect("),
    ("return PointInRect(GetMouseX(), GetMouseY()",
     "return point_in_rect(get_mouse_x(), get_mouse_y()"),
    ("inline bool IsClickInRect(", "inline bool is_click_in_rect("),
    ("return IsMouseButtonPressed(hb::shared::input::MouseButton::Left) && IsMouseInRect(",
     "return is_mouse_button_pressed(hb::shared::input::MouseButton::Left) && is_mouse_in_rect("),
    ("inline bool IsRightClickInRect(", "inline bool is_right_click_in_rect("),
    ("return IsMouseButtonPressed(hb::shared::input::MouseButton::Right) && IsMouseInRect(",
     "return is_mouse_button_pressed(hb::shared::input::MouseButton::Right) && is_mouse_in_rect("),
]

# ============================================================================
# GameWindowHandler.h/cpp — IWindowEventHandler overrides
# ============================================================================

GAMEWINDOWHANDLER_H_REPLACEMENTS = [
    ("void OnClose() override;", "void on_close() override;"),
    ("void OnDestroy() override;", "void on_destroy() override;"),
    ("void OnActivate(bool active) override;", "void on_activate(bool active) override;"),
    ("void OnResize(int width, int height) override;", "void on_resize(int width, int height) override;"),
    ("void OnKeyDown(KeyCode keyCode) override;", "void on_key_down(KeyCode key) override;"),
    ("void OnKeyUp(KeyCode keyCode) override;", "void on_key_up(KeyCode key) override;"),
    ("void OnChar(char character) override;", "void on_char(char character) override;"),
    ("void OnMouseMove(int x, int y) override;", "void on_mouse_move(int x, int y) override;"),
    ("void OnMouseButtonDown(int button, int x, int y) override;",
     "void on_mouse_button_down(int button, int x, int y) override;"),
    ("void OnMouseButtonUp(int button, int x, int y) override;",
     "void on_mouse_button_up(int button, int x, int y) override;"),
    ("void OnMouseWheel(int delta, int x, int y) override;",
     "void on_mouse_wheel(int delta, int x, int y) override;"),
    ("bool OnCustomMessage(uint32_t message, uintptr_t wParam, intptr_t lParam) override;",
     "bool on_custom_message(uint32_t message, uintptr_t wparam, intptr_t lparam) override;"),
    ("bool OnTextInput(hb::shared::types::NativeWindowHandle hWnd, uint32_t message, uintptr_t wParam, intptr_t lParam) override;",
     "bool on_text_input(hb::shared::types::NativeWindowHandle hwnd, uint32_t message, uintptr_t wparam, intptr_t lparam) override;"),
]

GAMEWINDOWHANDLER_CPP_REPLACEMENTS = [
    ("GameWindowHandler::OnClose(", "GameWindowHandler::on_close("),
    ("GameWindowHandler::OnDestroy(", "GameWindowHandler::on_destroy("),
    ("GameWindowHandler::OnActivate(", "GameWindowHandler::on_activate("),
    ("GameWindowHandler::OnResize(", "GameWindowHandler::on_resize("),
    ("GameWindowHandler::OnKeyDown(", "GameWindowHandler::on_key_down("),
    ("GameWindowHandler::OnKeyUp(", "GameWindowHandler::on_key_up("),
    ("GameWindowHandler::OnChar(", "GameWindowHandler::on_char("),
    ("GameWindowHandler::OnMouseMove(", "GameWindowHandler::on_mouse_move("),
    ("GameWindowHandler::OnMouseButtonDown(", "GameWindowHandler::on_mouse_button_down("),
    ("GameWindowHandler::OnMouseButtonUp(", "GameWindowHandler::on_mouse_button_up("),
    ("GameWindowHandler::OnMouseWheel(", "GameWindowHandler::on_mouse_wheel("),
    ("GameWindowHandler::OnCustomMessage(", "GameWindowHandler::on_custom_message("),
    ("GameWindowHandler::OnTextInput(", "GameWindowHandler::on_text_input("),
]

# ============================================================================
# RendererFactory.h Window static method declarations
# ============================================================================

RENDERERFACTORY_H_REPLACEMENTS = [
    ("static bool Create(const window_params& params);", "static bool create(const window_params& params);"),
    ("static IWindow* Get();", "static IWindow* get();"),
    ("static void Destroy();", "static void destroy();"),
    ("static hb::shared::types::NativeWindowHandle GetHandle();", "static hb::shared::types::NativeWindowHandle get_handle();"),
    ("static bool IsActive();", "static bool is_active();"),
    ("static void Close();", "static void close();"),
    ("static void ShowError(const char* title, const char* message);", "static void show_error(const char* title, const char* message);"),
    ("static void SetSize(int width, int height, bool center = true);", "static void set_size(int width, int height, bool center = true);"),
    ("static void SetBorderless(bool borderless);", "static void set_borderless(bool borderless);"),
]

# ============================================================================
# RendererFactory.cpp Window method implementations
# ============================================================================

RENDERERFACTORY_CPP_REPLACEMENTS = [
    ("Window::Create(", "Window::create("),
    ("Window::Get(", "Window::get("),
    ("Window::Destroy(", "Window::destroy("),
    ("Window::GetHandle(", "Window::get_handle("),
    ("Window::IsActive(", "Window::is_active("),
    ("Window::Close(", "Window::close("),
    ("Window::ShowError(", "Window::show_error("),
    ("Window::SetSize(", "Window::set_size("),
    ("Window::SetBorderless(", "Window::set_borderless("),
]


def collect_files():
    """Collect all .h and .cpp files from search directories, excluding backups."""
    files = []
    for search_dir in SEARCH_DIRS:
        for root, dirs, filenames in os.walk(search_dir):
            for fn in filenames:
                if not (fn.endswith(".h") or fn.endswith(".cpp")):
                    continue
                if any(skip in fn for skip in SKIP_SUFFIXES):
                    continue
                files.append(os.path.join(root, fn))
    return sorted(files)


def apply_replacements(content, replacements):
    """Apply a list of (old, new) string replacements to content."""
    for old, new in replacements:
        content = content.replace(old, new)
    return content


def get_file_specific_replacements(filepath):
    """Get additional replacements specific to certain files."""
    basename = os.path.basename(filepath)
    extra = []

    if basename == "IWindow.h":
        extra.extend(IWINDOW_VIRTUAL_REPLACEMENTS)
    elif basename == "IInput.h":
        extra.extend(IINPUT_CONVENIENCE_REPLACEMENTS)
    elif basename == "SFMLWindow.h":
        extra.extend(SFMLWINDOW_H_REPLACEMENTS)
    elif basename == "SFMLWindow.cpp":
        extra.extend(SFMLWINDOW_CPP_REPLACEMENTS)
        extra.extend(EVENT_HANDLER_REPLACEMENTS)
    elif basename == "SFMLInput.h":
        extra.extend(SFMLINPUT_H_REPLACEMENTS)
    elif basename == "SFMLInput.cpp":
        extra.extend(SFMLINPUT_CPP_REPLACEMENTS)
    elif basename == "GameWindowHandler.h":
        extra.extend(GAMEWINDOWHANDLER_H_REPLACEMENTS)
    elif basename == "GameWindowHandler.cpp":
        extra.extend(GAMEWINDOWHANDLER_CPP_REPLACEMENTS)
    elif basename == "RendererFactory.h":
        extra.extend(RENDERERFACTORY_H_REPLACEMENTS)
    elif basename == "RendererFactory.cpp":
        extra.extend(RENDERERFACTORY_CPP_REPLACEMENTS)

    return extra


def main():
    dry_run = "--dry-run" in sys.argv

    files = collect_files()
    print(f"Scanning {len(files)} files...")

    modified_files = []

    for filepath in files:
        with open(filepath, "r", encoding="utf-8-sig") as f:
            original = f.read()

        content = original

        # Apply file-specific replacements FIRST (they match more specific patterns)
        file_specific = get_file_specific_replacements(filepath)
        if file_specific:
            content = apply_replacements(content, file_specific)

        # Apply global replacements
        content = apply_replacements(content, REPLACEMENTS)

        # Apply IWindow instance method renames (via Window::get()-> and local vars)
        content = apply_replacements(content, WINDOW_INSTANCE_REPLACEMENTS)

        if content != original:
            modified_files.append(filepath)
            if not dry_run:
                with open(filepath, "w", encoding="utf-8") as f:
                    f.write(content)

    # Report
    print(f"\n{'[DRY RUN] Would modify' if dry_run else 'Modified'} {len(modified_files)} files:")
    for f in modified_files:
        # Show relative path
        rel = os.path.relpath(f, ROOT)
        print(f"  {rel}")

    if not dry_run and modified_files:
        print(f"\nDone. {len(modified_files)} files updated.")

    return 0


if __name__ == "__main__":
    sys.exit(main())

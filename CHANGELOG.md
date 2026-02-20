# CControls UI library

## Text Input Migration — Legacy to CControls
- Rewrote `text_input_manager` internals to use `cc::control_collection` + `cc::textbox` instead of raw WM_CHAR handling — all in-game text input now has cursor movement, selection, Home/End, and Ctrl+C/X/V clipboard support
- Added `clear()` to `cc::control_collection` — safely defocuses, destroys all children, and resets all internal state (repeat, tooltip, edge detection)
- Added `update(uint32_t time_ms)` to `text_input_manager` — pumps CControls input each frame, called from `Screen_OnGame::on_update()` before Enter key handling
- Simplified `CGame::on_text_input()` — removed `handle_char()` path entirely, all WM_CHAR now flows through `hb::shared::input::on_text_char()` for CControls consumption
- Added character filters to dialog call sites: guild name uses `character_name_allowed_chars`, item drop amount uses `digits_only`
- Added `digits_only` constant to `TextFieldRenderer.h`
- Added `shows_chat_background()` flag to `text_input_manager` — replaces fragile dialog-exclusion check in rendering with explicit positive flag set by chat callers only
- Removed 4 dead wrapper methods from `IGameScreen` (`start_input_string`, `end_input_string`, `clear_input_string`, `show_received_string`) and their `#include "TextInputManager.h"`
- Removed redundant `text_input_manager::get().end_input()` call from `Screen_MainMenu::on_initialize()`
- Client version bumped to 0.2.40

## CControls — Integrated Text Input
- Added `cc::text_input` base class — owns text buffer, cursor, selection, UTF-8 character handling, clipboard data ops. Engine-agnostic (no rendering, no OS calls)
- Rebased `cc::textbox` to extend `text_input` — thin validation-only layer, removed duplicate buffer/hidden/display logic
- Expanded `cc::input_state` with typed character buffer (`typed_chars[]`), modifier keys (`key_ctrl`), and editing keys (`key_backspace`, `key_delete`, `key_home`, `key_end`, `key_a/c/x/v`)
- Added `set_clipboard_provider()` to `control_collection` — screens wire clipboard via engine abstraction, CControls handles Ctrl+C/X/V routing
- Added text input key routing in `control_collection::update()` — backspace, delete, arrows (with key repeat), home/end, Ctrl+A/C/X/V, all handled internally
- Added `set_character_filter(std::string_view)` to `text_input` — whitelist of allowed characters, rejects anything not in the set
- Added character filter preset constants in `hb::client` namespace: `username_allowed_chars`, `password_allowed_chars`, `email_allowed_chars`
- Created `InputStateHelper.h/cpp` — `hb::client::fill_input_state()` fills `cc::input_state` from engine input in one call, minimizing screen boilerplate
- Created `TextFieldRenderer.h/cpp` — `hb::client::draw_text_field()` renders text, cursor blink, and selection highlight for any `text_input` control
- Added clipboard and typed char buffering to `IInput` interface — `get_clipboard_text()`, `set_clipboard_text()`, `on_text_char()`, `take_typed_chars()`
- Implemented clipboard and char buffering in `SFMLInput` via `sf::Clipboard`
- Fixed text input routing: `CGame::on_text_input()` now buffers unhandled chars for CControls via `hb::shared::input::on_text_char()`
- Fixed cursor blink reset: `text_input::update_blink()` stores frame time, `mark_activity()` uses it so cursor stays visible after typing
- Removed dead `GameWindowHandler.h/cpp` — `application` (CGame's base) handles all window events directly via `IWindowEventHandler`
- Removed textbox focus bridge (`set_textbox_focus_handler`, `handle_textbox_focus_change`) — text input is now fully integrated
- Updated Screen_Login and Screen_CreateAccount to use integrated text input: clipboard provider, `fill_input_state()`, `draw_text_field()`, character filters
- Client version bumped to 0.2.37

## CControls — Alignment Support
- Added `set_opacity(float)` / `opacity()` on controls for fade effects (clamped 0.0-1.0)
- Converted Screen_Splash to use CControls: credit labels use alignment + opacity for fade, render handlers draw text
- Converted Screen_MainMenu to use CControls: 3 buttons with click handlers, render handlers, focus order. Removed manual m_cur_focus/m_max_focus, manual hit-testing, manual tab/arrow/enter handling
- Added `set_enter_advances_focus(bool)` to `control_collection` — Enter on a focused textbox advances to next control in focus order (form-style navigation)
- Converted Screen_CreateAccount to use CControls: 4 textboxes with validators + textbox focus bridge, 3 buttons with click handlers, enter-advances-focus for form navigation. Removed all manual focus/input/validation tracking
- Fixed enter-advances-focus: textbox focus bridge now fires after Enter-triggered advance (was skipped because bridge ran before Enter handler)
- Added Space key support: Space activates focused button (same as Enter). Added `key_space` to `cc::input_state`
- Changed hover-focus default to `false` — focus only changes on mouse click, not passive hover. Screens can opt in with `set_hover_focus(true)`
- Added `is_highlighted()` to `control` — returns `is_hovered() || is_focused() || is_held()`. One check instead of three in render handlers
- Updated all button render handlers in Screen_MainMenu and Screen_CreateAccount to use `is_highlighted()`
- Added `cc::align::horizontal` and `cc::align::vertical` enums (`none`, `left`/`top`, `center`, `right`/`bottom`)
- Added `set_horizontal_alignment()` and `set_vertical_alignment()` on all controls with optional margin parameter
- Added `set_screen_size()` on `control_collection` for root-level alignment
- Alignment propagates through the parent chain — `screen_bounds()` and `screen_offset()` use resolved positions
- Added `flow_direction` enum (`none`, `horizontal`, `vertical`) and `set_flow()` on controls for auto-stacking children
- Added `set_padding(int)` for inter-child spacing in flow layout (margin = offset from parent, padding = gap after this control)
- Added `set_allow_overlap(bool)` to opt individual controls out of parent's flow
- Added `set_default_button(int id)` to `control_collection` — auto-fires a button when Enter-advance lands on it, or fires directly from any textbox when `enter_advances_focus` is off
- Converted Screen_Login to use CControls: 2 textboxes with validators + textbox focus bridge, 2 buttons with click handlers, default button for Enter-to-submit, login box fade-in animation preserved. Removed all manual focus/input/hit-testing
- Added `set_default_button(BTN_CREATE)` to Screen_CreateAccount — Enter on email field now submits the form
- Client version bumped to 0.2.35

Created the CControls static library for declarative menu screen UI. Parent/child control tree with automatic focus management, render handlers, click sounds, tooltips, hover-to-focus, key repeat, and internal edge detection. Integrated into both Windows and Linux build systems.

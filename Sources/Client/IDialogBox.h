#pragma once

#include "DialogBoxInfo.h"
#include "DialogBoxIDs.h"
#include "CommonTypes.h"
#include <cstdint>

class CGame;
class DialogBoxManager;

// Result of on_press - determines how the click is handled
enum class PressResult
{
	Normal = 0,        // Normal click, allow dialog dragging
	ItemSelected = 1,  // Item/equipment selected, dialog handles CursorTarget
	ScrollClaimed = -1 // Scroll/slider region claimed, prevent dragging
};

class IDialogBox
{
public:
	IDialogBox(DialogBoxId::Type id, CGame* game);
	virtual ~IDialogBox() = default;

	// Core virtual methods - must be implemented by derived classes
	virtual void on_draw(short mouse_x, short mouse_y, short z, char lb) = 0;
	virtual bool on_click(short mouse_x, short mouse_y) = 0;

	// Optional virtual methods - override as needed
	virtual void on_update() {}  // Called once per frame for enabled dialogs
	virtual bool on_double_click(short mouse_x, short mouse_y) { return false; }

	// Called on mouse button down within dialog bounds
	virtual PressResult on_press(short mouse_x, short mouse_y) { return PressResult::Normal; }

	virtual bool on_item_drop(short mouse_x, short mouse_y) { return false; }  // Item dropped on dialog
	virtual void on_enable(int type, int v1, int v2, char* string) {}
	virtual void on_disable() {}

	// Accessors
	DialogBoxId::Type get_id() const { return m_id; }
	DialogBoxInfo& Info();
	const DialogBoxInfo& Info() const;
	bool is_enabled() const;

protected:
	// Helper methods - delegate to CGame
	void draw_new_dialog_box(char type, int sX, int sY, int frame, bool is_no_color_key = false, bool is_trans = false);
	void put_string(int iX, int iY, const char* string, const hb::shared::render::Color& color);
	void put_aligned_string(int x1, int x2, int iY, const char* string, const hb::shared::render::Color& color = GameColors::UIBlack);
	void play_sound_effect(char type, int num, int dist, long lPan = 0);
	void add_event_list(const char* txt, char color = 0, bool dup_allow = true);
	bool send_command(uint32_t msg_id, uint16_t command, char dir, int v1, int v2, int v3, const char* string, int v4 = 0);
	void set_default_rect(short sX, short sY, short size_x, short size_y);

	// Dialog management helpers
	void enable_dialog_box(DialogBoxId::Type id, int type = 0, int v1 = 0, int v2 = 0, char* string = nullptr);
	void disable_dialog_box(DialogBoxId::Type id);
	void disable_this_dialog();
	void set_can_close_on_right_click(bool can_close);

	// Inter-dialog communication
	IDialogBox* get_dialog_box(DialogBoxId::Type id);
	DialogBoxInfo& info_of(DialogBoxId::Type id);
	template<typename T>
	T* get_dialog_box_as(DialogBoxId::Type id) { return static_cast<T*>(get_dialog_box(id)); }

	// Direct access to game - use m_game->member for all game state
	CGame* m_game;
	DialogBoxId::Type m_id;
};

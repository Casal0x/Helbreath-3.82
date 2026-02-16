#pragma once

#include "DialogBoxIDs.h"
#include "DialogBoxInfo.h"
#include "IDialogBox.h"
#include <cstdint>
#include <memory>

class CGame;

class DialogBoxManager
{
public:
	explicit DialogBoxManager(CGame* game = nullptr);
	~DialogBoxManager() = default;  // unique_ptr handles cleanup automatically

	void initialize(CGame* game);
	void initialize_dialog_boxes();
	void register_dialog_box(std::unique_ptr<IDialogBox> dialog_box);
	IDialogBox* get_dialog_box(DialogBoxId::Type id) const;
	IDialogBox* get_dialog_box(int box_id) const;
	void update_dialog_boxs();
	void draw_dialog_boxs(short mouse_x, short mouse_y, short z, char lb);
	void enable_dialog_box(int box_id, int type, int64_t v1, int v2, char* string = nullptr);
	void enable_dialog_box(DialogBoxId::Type id, int type, int64_t v1, int v2, char* string = nullptr);
	void disable_dialog_box(int box_id);
	void disable_dialog_box(DialogBoxId::Type id);
	void toggle_dialog_box(DialogBoxId::Type id, int type = 0, int64_t v1 = 0, int v2 = 0, char* string = nullptr);
	int get_top_dialog_box_index() const;

	void draw_all(short mouse_x, short mouse_y, short z, char lb);
	bool handle_click(short mouse_x, short mouse_y);
	bool handle_double_click(short mouse_x, short mouse_y);
	PressResult handle_press(int dlg_id, short mouse_x, short mouse_y);
	bool handle_item_drop(int dlg_id, short mouse_x, short mouse_y);
	bool handle_dragging_item_release(short mouse_x, short mouse_y);

	// Mouse down handling - replaces _iCheckDlgBoxFocus from Game.cpp
	// Returns: 1 = dialog hit, 0 = no dialog hit, -1 = scroll region claimed
	int handle_mouse_down(short mouse_x, short mouse_y);

	// Right-click to close dialogs
	// Returns: true if a dialog was under the mouse (and potentially closed)
	bool handle_right_click(short mouse_x, short mouse_y, uint32_t time);
	void enable(DialogBoxId::Type id, int type, int64_t v1, int v2, char* string = nullptr);
	void disable(DialogBoxId::Type id);
	void toggle(DialogBoxId::Type id, int type = 0, int64_t v1 = 0, int v2 = 0, char* string = nullptr);
	int get_top_id() const;
	bool is_enabled(DialogBoxId::Type id) const;
	bool is_enabled(int box_id) const;
	void set_enabled(DialogBoxId::Type id, bool enabled);
	void set_enabled(int box_id, bool enabled);
	DialogBoxInfo& Info(DialogBoxId::Type id);
	const DialogBoxInfo& Info(DialogBoxId::Type id) const;
	DialogBoxInfo& Info(int box_id);
	const DialogBoxInfo& Info(int box_id) const;
	uint8_t order_at(int index) const;
	void set_order_at(int index, uint8_t value);

private:
	CGame* m_game;
	DialogBoxInfo m_info[61]{};
	uint8_t m_order[61]{};
	bool m_enabled[61]{};
	std::unique_ptr<IDialogBox> m_pDialogBoxes[61];
	uint32_t m_dwDialogCloseTime = 0;
};

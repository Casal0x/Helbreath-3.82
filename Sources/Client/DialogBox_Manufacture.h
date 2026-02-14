#pragma once
#include "IDialogBox.h"

class DialogBox_Manufacture : public IDialogBox
{
public:
	DialogBox_Manufacture(CGame* game);
	~DialogBox_Manufacture() override = default;

	void on_draw(short mouse_x, short mouse_y, short z, char lb) override;
	bool on_click(short mouse_x, short mouse_y) override;
	PressResult on_press(short mouse_x, short mouse_y) override;
	bool on_item_drop(short mouse_x, short mouse_y) override;

private:
	void draw_alchemy_waiting(short sX, short sY, short mouse_x, short mouse_y);
	void draw_alchemy_creating(short sX, short sY);
	void draw_manufacture_list(short sX, short sY, short mouse_x, short mouse_y, short z, char lb);
	void draw_manufacture_waiting(short sX, short sY, short mouse_x, short mouse_y);
	void draw_manufacture_in_progress(short sX, short sY);
	void draw_manufacture_done(short sX, short sY, short mouse_x, short mouse_y);
	void draw_crafting_waiting(short sX, short sY, short mouse_x, short mouse_y);
	void draw_crafting_in_progress(short sX, short sY);
	void reset_item_slots();

	// Press helpers
	bool check_slot_item_click(int slotIndex, int itemIdx, int drawX, int drawY, short mouse_x, short mouse_y);

	// Item drop helper: Tries to add item to first empty slot, returns true if successful
	bool try_add_item_to_slot(int item_id, bool updateBuildStatus);
};

#pragma once
#include "IDialogBox.h"

class DialogBox_SellList : public IDialogBox
{
public:
	DialogBox_SellList(CGame* game);
	~DialogBox_SellList() override = default;

	void on_draw(short mouse_x, short mouse_y, short z, char lb) override;
	bool on_click(short mouse_x, short mouse_y) override;
	bool on_item_drop(short mouse_x, short mouse_y) override;

private:
	void draw_item_list(short sX, short sY, short size_x, short mouse_x, short mouse_y, int& empty_count);
	void draw_empty_list_message(short sX, short sY, short size_x);
	void draw_buttons(short sX, short sY, short mouse_x, short mouse_y, bool has_items);
};

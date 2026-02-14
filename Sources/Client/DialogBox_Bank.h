#pragma once
#include "IDialogBox.h"

class DialogBox_Bank : public IDialogBox
{
public:
	DialogBox_Bank(CGame* game);
	~DialogBox_Bank() override = default;

	void on_draw(short mouse_x, short mouse_y, short z, char lb) override;
	bool on_click(short mouse_x, short mouse_y) override;
	bool on_double_click(short mouse_x, short mouse_y) override;
	PressResult on_press(short mouse_x, short mouse_y) override;
	bool on_item_drop(short mouse_x, short mouse_y) override;

private:
	void draw_item_list(short sX, short sY, short size_x, short mouse_x, short mouse_y, short z, char lb);
	void draw_item_details(short sX, short sY, short size_x, int item_index, int yPos);
	void draw_scrollbar(short sX, short sY, int total_lines, short mouse_x, short mouse_y, short z, char lb);
};

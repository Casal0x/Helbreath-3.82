#pragma once
#include "IDialogBox.h"

class DialogBox_Exchange : public IDialogBox
{
public:
	DialogBox_Exchange(CGame* game);
	~DialogBox_Exchange() override = default;

	void on_draw(short mouse_x, short mouse_y, short z, char lb) override;
	bool on_click(short mouse_x, short mouse_y) override;
	bool on_item_drop(short mouse_x, short mouse_y) override;

private:
	void draw_items(short sX, short sY, short mouse_x, short mouse_y, int start_index, int end_index);
	void draw_item_info(short sX, short sY, short size_x, short mouse_x, short mouse_y, int item_index, short xadd);
};

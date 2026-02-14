#pragma once

#include "IDialogBox.h"

class DialogBox_MagicShop : public IDialogBox
{
public:
	DialogBox_MagicShop(CGame* game);
	~DialogBox_MagicShop() override = default;

	void on_draw(short mouse_x, short mouse_y, short z, char lb) override;
	bool on_click(short mouse_x, short mouse_y) override;

private:
	void draw_spell_list(short sX, short sY, short mouse_x, short mouse_y);
	void draw_page_indicator(short sX, short sY);
	bool handle_spell_click(short sX, short sY, short mouse_x, short mouse_y);
	void handle_page_click(short sX, short sY, short mouse_x, short mouse_y);
};

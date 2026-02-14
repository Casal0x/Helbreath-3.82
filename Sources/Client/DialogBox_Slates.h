#pragma once
#include "IDialogBox.h"

class DialogBox_Slates : public IDialogBox
{
public:
	DialogBox_Slates(CGame* game);
	~DialogBox_Slates() override = default;

	void on_draw(short mouse_x, short mouse_y, short z, char lb) override;
	bool on_click(short mouse_x, short mouse_y) override;
	bool on_item_drop(short mouse_x, short mouse_y) override;
};

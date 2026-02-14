#pragma once
#include "IDialogBox.h"

class DialogBox_SellOrRepair : public IDialogBox
{
public:
	DialogBox_SellOrRepair(CGame* game);
	~DialogBox_SellOrRepair() override = default;

	void on_draw(short mouse_x, short mouse_y, short z, char lb) override;
	bool on_click(short mouse_x, short mouse_y) override;
};

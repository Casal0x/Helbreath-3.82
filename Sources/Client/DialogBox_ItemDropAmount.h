#pragma once
#include "IDialogBox.h"

class DialogBox_ItemDropAmount : public IDialogBox
{
public:
	DialogBox_ItemDropAmount(CGame* game);
	~DialogBox_ItemDropAmount() override = default;

	void on_draw(short mouse_x, short mouse_y, short z, char lb) override;
	bool on_click(short mouse_x, short mouse_y) override;
};

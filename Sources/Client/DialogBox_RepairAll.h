#pragma once
#include "IDialogBox.h"

class DialogBox_RepairAll : public IDialogBox
{
public:
	DialogBox_RepairAll(CGame* game);
	~DialogBox_RepairAll() override = default;

	void on_draw(short mouse_x, short mouse_y, short z, char lb) override;
	bool on_click(short mouse_x, short mouse_y) override;
};

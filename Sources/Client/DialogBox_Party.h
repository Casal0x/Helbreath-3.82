#pragma once
#include "IDialogBox.h"

class DialogBox_Party : public IDialogBox
{
public:
	DialogBox_Party(CGame* game);
	~DialogBox_Party() override = default;

	void on_draw(short mouse_x, short mouse_y, short z, char lb) override;
	bool on_click(short mouse_x, short mouse_y) override;
};

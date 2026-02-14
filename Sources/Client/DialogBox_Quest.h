#pragma once
#include "IDialogBox.h"

class DialogBox_Quest : public IDialogBox
{
public:
	DialogBox_Quest(CGame* game);
	~DialogBox_Quest() override = default;

	void on_draw(short mouse_x, short mouse_y, short z, char lb) override;
	bool on_click(short mouse_x, short mouse_y) override;
};

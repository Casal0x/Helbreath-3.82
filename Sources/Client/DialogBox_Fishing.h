#pragma once
#include "IDialogBox.h"

class DialogBox_Fishing : public IDialogBox
{
public:
	DialogBox_Fishing(CGame* game);
	~DialogBox_Fishing() override = default;

	void on_draw(short mouse_x, short mouse_y, short z, char lb) override;
	bool on_click(short mouse_x, short mouse_y) override;
};

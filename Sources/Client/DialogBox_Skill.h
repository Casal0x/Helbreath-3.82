#pragma once
#include "IDialogBox.h"

class DialogBox_Skill : public IDialogBox
{
public:
	DialogBox_Skill(CGame* game);
	~DialogBox_Skill() override = default;

	void on_draw(short mouse_x, short mouse_y, short z, char lb) override;
	bool on_click(short mouse_x, short mouse_y) override;
	PressResult on_press(short mouse_x, short mouse_y) override;
};

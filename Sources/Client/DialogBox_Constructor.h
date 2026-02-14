#pragma once
#include "IDialogBox.h"

class DialogBox_Constructor : public IDialogBox
{
public:
	DialogBox_Constructor(CGame* game);
	~DialogBox_Constructor() override = default;

	void on_update() override;
	void on_draw(short mouse_x, short mouse_y, short z, char lb) override;
	bool on_click(short mouse_x, short mouse_y) override;
};

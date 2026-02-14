#pragma once
#include "IDialogBox.h"

class DialogBox_ConfirmExchange : public IDialogBox
{
public:
	DialogBox_ConfirmExchange(CGame* game);
	~DialogBox_ConfirmExchange() override = default;

	void on_draw(short mouse_x, short mouse_y, short z, char lb) override;
	bool on_click(short mouse_x, short mouse_y) override;
};

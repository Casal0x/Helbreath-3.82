#pragma once

#include "IDialogBox.h"

class DialogBox_Noticement : public IDialogBox
{
public:
	explicit DialogBox_Noticement(CGame* game);

	void on_draw(short mouse_x, short mouse_y, short z, char lb) override;
	bool on_click(short mouse_x, short mouse_y) override;
};

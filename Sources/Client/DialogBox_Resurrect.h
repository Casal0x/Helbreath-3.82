#pragma once

#include "IDialogBox.h"

class DialogBox_Resurrect : public IDialogBox
{
public:
	explicit DialogBox_Resurrect(CGame* game);

	void on_draw(short mouse_x, short mouse_y, short z, char lb) override;
	bool on_click(short mouse_x, short mouse_y) override;
};

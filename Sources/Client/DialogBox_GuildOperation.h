#pragma once
#include "IDialogBox.h"

class DialogBox_GuildOperation : public IDialogBox
{
public:
	DialogBox_GuildOperation(CGame* game);
	~DialogBox_GuildOperation() override = default;

	void on_draw(short mouse_x, short mouse_y, short z, char lb) override;
	bool on_click(short mouse_x, short mouse_y) override;

private:
	void draw_join_request(short sX, short sY, short mouse_x, short mouse_y);
	void draw_dismiss_request(short sX, short sY, short mouse_x, short mouse_y);
	void draw_info_message(short sX, short sY, short mouse_x, short mouse_y, int mode);
};

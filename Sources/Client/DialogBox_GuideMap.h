#pragma once
#include "IDialogBox.h"

class DialogBox_GuideMap : public IDialogBox
{
public:
	DialogBox_GuideMap(CGame* game);
	~DialogBox_GuideMap() override = default;

	void on_update() override;
	void on_draw(short mouse_x, short mouse_y, short z, char lb) override;
	bool on_click(short mouse_x, short mouse_y) override;
	bool on_double_click(short mouse_x, short mouse_y) override;

private:
	void draw_border(short sX, short sY);
	void draw_zoomed_map(short sX, short sY);
	void draw_full_map(short sX, short sY);
	void draw_location_tooltip(short mouse_x, short mouse_y, short sX, short sY);
};

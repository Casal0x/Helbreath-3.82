#pragma once
#include "IDialogBox.h"

class DialogBox_NpcActionQuery : public IDialogBox
{
public:
	DialogBox_NpcActionQuery(CGame* game);
	~DialogBox_NpcActionQuery() override = default;

	void on_draw(short mouse_x, short mouse_y, short z, char lb) override;
	bool on_click(short mouse_x, short mouse_y) override;

private:
	void DrawMode0_NpcMenu(short sX, short sY, short mouse_x, short mouse_y);
	void DrawMode1_GiveToPlayer(short sX, short sY, short mouse_x, short mouse_y);
	void DrawMode2_SellToShop(short sX, short sY, short mouse_x, short mouse_y);
	void DrawMode3_DepositToWarehouse(short sX, short sY, short mouse_x, short mouse_y);
	void DrawMode4_TalkToNpcOrUnicorn(short sX, short sY, short mouse_x, short mouse_y);
	void DrawMode5_ShopWithSell(short sX, short sY, short mouse_x, short mouse_y);
	void DrawMode6_Gail(short sX, short sY, short mouse_x, short mouse_y);

	void draw_highlighted_text(short sX, short sY, const char* text, short mouse_x, short mouse_y, short hitX1, short hitX2, short hitY1, short hitY2);
};

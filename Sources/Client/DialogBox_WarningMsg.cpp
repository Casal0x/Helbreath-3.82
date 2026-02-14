#include "DialogBox_WarningMsg.h"
#include "Game.h"
#include "lan_eng.h"
#include "GameFonts.h"
#include "TextLibExt.h"
using namespace hb::client::sprite_id;

DialogBox_WarningMsg::DialogBox_WarningMsg(CGame* game)
	: IDialogBox(DialogBoxId::WarningBattleArea, game)
{
	set_default_rect(0 , 0 , 310, 170);
}

void DialogBox_WarningMsg::on_draw(short mouse_x, short mouse_y, short z, char lb)
{
	short sX = Info().m_x;
	short sY = Info().m_y;

	draw_new_dialog_box(InterfaceNdGame4, sX, sY, 2);

	hb::shared::text::draw_text(GameFont::Default, sX + 63, sY + 35, DEF_MSG_WARNING1, hb::shared::text::TextStyle::with_shadow(GameColors::UIYellow));
	put_string(sX + 30, sY + 57, DEF_MSG_WARNING2, GameColors::UIOrange);
	put_string(sX + 30, sY + 74, DEF_MSG_WARNING3, GameColors::UIOrange);
	put_string(sX + 30, sY + 92, DEF_MSG_WARNING4, GameColors::UIOrange);
	put_string(sX + 30, sY + 110, DEF_MSG_WARNING5, GameColors::UIOrange);

	// OK button
	if ((mouse_x >= sX + 122) && (mouse_x <= sX + 125 + ui_layout::btn_size_x) && (mouse_y >= sY + 127) && (mouse_y <= sY + 127 + ui_layout::btn_size_y))
		draw_new_dialog_box(InterfaceNdButton, sX + 122, sY + 127, 1);
	else
		draw_new_dialog_box(InterfaceNdButton, sX + 122, sY + 127, 0);
}

bool DialogBox_WarningMsg::on_click(short mouse_x, short mouse_y)
{
	short sX = Info().m_x;
	short sY = Info().m_y;

	// OK button click
	if ((mouse_x >= sX + 122) && (mouse_x <= sX + 125 + ui_layout::btn_size_x) && (mouse_y >= sY + 127) && (mouse_y <= sY + 127 + ui_layout::btn_size_y))
	{
		disable_this_dialog();
		return true;
	}

	return false;
}

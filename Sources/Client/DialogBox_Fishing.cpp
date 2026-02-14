#include "DialogBox_Fishing.h"
#include "Game.h"
#include "ItemNameFormatter.h"
#include "GameFonts.h"
#include "GlobalDef.h"
#include "TextLibExt.h"
#include "lan_eng.h"
#include <format>
#include <string>

using namespace hb::shared::net;
using namespace hb::client::sprite_id;
DialogBox_Fishing::DialogBox_Fishing(CGame* game)
	: IDialogBox(DialogBoxId::Fishing, game)
{
	set_default_rect(193 , 241 , 263, 100);
}

void DialogBox_Fishing::on_draw(short mouse_x, short mouse_y, short z, char lb)
{
	short sX = Info().m_x;
	short sY = Info().m_y;
	uint32_t time = m_game->m_cur_time;
	std::string txt;

	draw_new_dialog_box(InterfaceNdGame1, sX, sY, 2);

	auto itemInfo = item_name_formatter::get().format(m_game->find_item_id_by_name(Info().m_str),  0);

	switch (Info().m_mode)
	{
	case 0:
		m_game->m_sprite[ItemPackPivotPoint + Info().m_v3]->draw(sX + 18 + 35, sY + 18 + 17, Info().m_v4);

		txt = itemInfo.name.c_str();
		put_string(sX + 98, sY + 14, txt.c_str(), GameColors::UIWhite);

		txt = std::format(DRAW_DIALOGBOX_FISHING1, Info().m_v2);
		put_string(sX + 98, sY + 28, txt.c_str(), GameColors::UIBlack);

		put_string(sX + 97, sY + 43, DRAW_DIALOGBOX_FISHING2, GameColors::UIBlack);

		txt = std::format("{} %", Info().m_v1);
		hb::shared::text::draw_text(GameFont::Bitmap1, sX + 157, sY + 40, txt.c_str(), hb::shared::text::TextStyle::with_highlight(GameColors::BmpBtnFishRed));

		// "Try Now!" button
		if ((mouse_x >= sX + 160) && (mouse_x <= sX + 253) && (mouse_y >= sY + 70) && (mouse_y <= sY + 90))
			hb::shared::text::draw_text(GameFont::Bitmap1, sX + 160, sY + 70, "Try Now!", hb::shared::text::TextStyle::with_highlight(GameColors::UIMagicBlue));
		else
			hb::shared::text::draw_text(GameFont::Bitmap1, sX + 160, sY + 70, "Try Now!", hb::shared::text::TextStyle::with_highlight(GameColors::BmpBtnNormal));
		break;
	}
}

bool DialogBox_Fishing::on_click(short mouse_x, short mouse_y)
{
	short sX = Info().m_x;
	short sY = Info().m_y;

	switch (Info().m_mode)
	{
	case 0:
		if ((mouse_x >= sX + 160) && (mouse_x <= sX + 253) && (mouse_y >= sY + 70) && (mouse_y <= sY + 90))
		{
			m_game->send_command(MsgId::CommandCommon, CommonType::ReqGetFishThisTime, 0, 0, 0, 0, 0);
			m_game->add_event_list(DLGBOX_CLICK_FISH1, 10);
			disable_dialog_box(DialogBoxId::Fishing);
			m_game->play_game_sound('E', 14, 5);
			return true;
		}
		break;
	}

	return false;
}

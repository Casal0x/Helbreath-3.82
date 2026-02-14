#include "DialogBox_ItemDropAmount.h"
#include "Game.h"
#include "ItemNameFormatter.h"
#include "GlobalDef.h"
#include "lan_eng.h"
#include "GameFonts.h"
#include "TextLibExt.h"
#include <format>
#include <string>
using namespace hb::client::sprite_id;

DialogBox_ItemDropAmount::DialogBox_ItemDropAmount(CGame* game)
	: IDialogBox(DialogBoxId::ItemDropExternal, game)
{
	set_default_rect(0 , 0 , 215, 87);
}

void DialogBox_ItemDropAmount::on_draw(short mouse_x, short mouse_y, short z, char lb)
{
	short sX = Info().m_x;
	short sY = Info().m_y;
	std::string txt;


	draw_new_dialog_box(InterfaceNdGame2, sX, sY, 5);

	switch (Info().m_mode)
	{
	case 1:
	{
		auto itemInfo = item_name_formatter::get().format(m_game->m_item_list[Info().m_view].get());

		if (Info().m_str[0] == '\0')
			txt = std::format(DRAW_DIALOGBOX_QUERY_DROP_ITEM_AMOUNT1, itemInfo.name.c_str());
		else
			txt = std::format(DRAW_DIALOGBOX_QUERY_DROP_ITEM_AMOUNT2, itemInfo.name.c_str(), Info().m_str);

		if (Info().m_v3 < 1000)
			put_string(sX + 30, sY + 20, txt.c_str(), GameColors::UILabel);

		put_string(sX + 30, sY + 35, DRAW_DIALOGBOX_QUERY_DROP_ITEM_AMOUNT3, GameColors::UILabel);

		if (m_game->m_dialog_box_manager.get_top_dialog_box_index() != DialogBoxId::ItemDropExternal)
			hb::shared::text::draw_text(GameFont::Default, sX + 40, sY + 57, m_game->m_amount_string.c_str(), hb::shared::text::TextStyle::from_color(GameColors::UIWhite));

		txt = std::format("__________ (0 ~ {})", m_game->m_item_list[Info().m_view]->m_count);
		put_string(sX + 38, sY + 62, txt.c_str(), GameColors::UILabel);
		break;
	}

	case 20:
	{
		auto itemInfo2 = item_name_formatter::get().format(m_game->m_item_list[Info().m_view].get());

		if (Info().m_str[0] == '\0')
			txt = std::format(DRAW_DIALOGBOX_QUERY_DROP_ITEM_AMOUNT1, itemInfo2.name.c_str());
		else
			txt = std::format(DRAW_DIALOGBOX_QUERY_DROP_ITEM_AMOUNT2, itemInfo2.name.c_str(), Info().m_str);

		if (Info().m_v3 < 1000)
			put_string(sX + 30, sY + 20, txt.c_str(), GameColors::UILabel);

		put_string(sX + 30, sY + 35, DRAW_DIALOGBOX_QUERY_DROP_ITEM_AMOUNT3, GameColors::UILabel);
		hb::shared::text::draw_text(GameFont::Default, sX + 40, sY + 57, m_game->m_amount_string.c_str(), hb::shared::text::TextStyle::from_color(GameColors::UIWhite));

		txt = std::format("__________ (0 ~ {})", m_game->m_item_list[Info().m_view]->m_count);
		put_string(sX + 38, sY + 62, txt.c_str(), GameColors::UILabel);
		break;
	}
	}
}

bool DialogBox_ItemDropAmount::on_click(short mouse_x, short mouse_y)
{
	// Click handling for this dialog is done elsewhere (text input handling)
	return false;
}

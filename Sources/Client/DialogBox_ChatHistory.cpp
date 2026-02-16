#include "DialogBox_ChatHistory.h"
#include "ChatManager.h"
#include "ConfigManager.h"
#include "Game.h"
#include "IInput.h"
#include "GameFonts.h"
#include "TextLibExt.h"
using namespace hb::client::sprite_id;

#define DEF_CHAT_VISIBLE_LINES 8
#define DEF_CHAT_SCROLLBAR_HEIGHT 105

DialogBox_ChatHistory::DialogBox_ChatHistory(CGame* game)
	: IDialogBox(DialogBoxId::ChatHistory, game)
{
	set_default_rect(218 , 385 , 364, 162);
}

void DialogBox_ChatHistory::on_draw(short mouse_x, short mouse_y, short z, char lb)
{
	short sX = m_game->m_dialog_box_manager.Info(DialogBoxId::ChatHistory).m_x;
	short sY = m_game->m_dialog_box_manager.Info(DialogBoxId::ChatHistory).m_y;

	const bool dialogTrans = config_manager::get().is_dialog_transparency_enabled();
	m_game->draw_new_dialog_box(InterfaceNdGame2, sX, sY, 4, false, dialogTrans);
	m_game->draw_new_dialog_box(InterfaceNdText, sX, sY, 22, false, dialogTrans);

	handle_scroll_input(sX, sY, mouse_x, mouse_y, z, lb);
	draw_scroll_bar(sX, sY);
	draw_chat_messages(sX, sY);
}

void DialogBox_ChatHistory::handle_scroll_input(short sX, short sY, short mouse_x, short mouse_y, short z, char lb)
{
	auto& info = m_game->m_dialog_box_manager.Info(DialogBoxId::ChatHistory);

	// Mouse wheel scrolling
	if (m_game->m_dialog_box_manager.get_top_dialog_box_index() == DialogBoxId::ChatHistory)
	{
		short wheel_delta = hb::shared::input::get_mouse_wheel_delta();
		if (wheel_delta != 0)
		{
			info.m_view += wheel_delta / 30;
		}
	}

	// Scroll bar dragging
	if ((lb != 0) && (m_game->m_dialog_box_manager.get_top_dialog_box_index() == DialogBoxId::ChatHistory))
	{
		// Drag scrollbar track
		if ((mouse_x >= sX + 336) && (mouse_x <= sX + 361) && (mouse_y >= sY + 28) && (mouse_y <= sY + 140))
		{
			double d1 = static_cast<double>(mouse_y - (sY + 28));
			double d2 = ((game_limits::max_chat_scroll_msgs - DEF_CHAT_VISIBLE_LINES) * d1) / static_cast<double>(DEF_CHAT_SCROLLBAR_HEIGHT);
			info.m_view = game_limits::max_chat_scroll_msgs - DEF_CHAT_VISIBLE_LINES - static_cast<int>(d2);
		}

		// Scroll to top button
		if ((mouse_x >= sX + 336) && (mouse_x <= sX + 361) && (mouse_y > sY + 18) && (mouse_y < sY + 28))
			info.m_view = game_limits::max_chat_scroll_msgs - DEF_CHAT_VISIBLE_LINES;

		// Scroll to bottom button
		if ((mouse_x >= sX + 336) && (mouse_x <= sX + 361) && (mouse_y > sY + 140) && (mouse_y < sY + 163))
			info.m_view = 0;
	}
	else
	{
		info.m_is_scroll_selected = false;
	}

	// clamp scroll view (must be after all scroll modifications)
	if (info.m_view < 0)
		info.m_view = 0;
	if (info.m_view > game_limits::max_chat_scroll_msgs - DEF_CHAT_VISIBLE_LINES)
		info.m_view = game_limits::max_chat_scroll_msgs - DEF_CHAT_VISIBLE_LINES;
}

void DialogBox_ChatHistory::draw_scroll_bar(short sX, short sY)
{
	double d1 = static_cast<double>(m_game->m_dialog_box_manager.Info(DialogBoxId::ChatHistory).m_view);
	double d2 = static_cast<double>(DEF_CHAT_SCROLLBAR_HEIGHT);
	double d3 = (d1 * d2) / (game_limits::max_chat_scroll_msgs - DEF_CHAT_VISIBLE_LINES);
	int pointer_loc = static_cast<int>(d3);
	pointer_loc = DEF_CHAT_SCROLLBAR_HEIGHT - pointer_loc;
	m_game->draw_new_dialog_box(InterfaceNdGame2, sX + 346, sY + 33 + pointer_loc, 7);
}

void DialogBox_ChatHistory::draw_chat_messages(short sX, short sY)
{
	short view = m_game->m_dialog_box_manager.Info(DialogBoxId::ChatHistory).m_view;

	for (int i = 0; i < DEF_CHAT_VISIBLE_LINES; i++)
	{
		int index = i + view;
		if (index < 0 || index >= game_limits::max_chat_scroll_msgs) continue;
		CMsg* chat_msg = ChatManager::get().get_message(index);
		if (chat_msg != nullptr)
		{
			int y_pos = sY + 127 - i * 13;
			char* pMsg = chat_msg->m_pMsg;

			switch (chat_msg->m_time)
			{
			case 0:  hb::shared::text::draw_text(GameFont::Default, sX + 25, y_pos, pMsg, hb::shared::text::TextStyle::with_shadow(GameColors::UINearWhite)); break; // Normal
			case 1:  hb::shared::text::draw_text(GameFont::Default, sX + 25, y_pos, pMsg, hb::shared::text::TextStyle::with_shadow(GameColors::UIGuildGreen)); break; // Green
			case 2:  hb::shared::text::draw_text(GameFont::Default, sX + 25, y_pos, pMsg, hb::shared::text::TextStyle::with_shadow(GameColors::UIWorldChat)); break; // Red
			case 3:  hb::shared::text::draw_text(GameFont::Default, sX + 25, y_pos, pMsg, hb::shared::text::TextStyle::with_shadow(GameColors::UIFactionChat)); break; // Blue
			case 4:  hb::shared::text::draw_text(GameFont::Default, sX + 25, y_pos, pMsg, hb::shared::text::TextStyle::with_shadow(GameColors::UIPartyChat)); break; // Yellow
			case 10: hb::shared::text::draw_text(GameFont::Default, sX + 25, y_pos, pMsg, hb::shared::text::TextStyle::with_shadow(GameColors::UIGameMasterChat)); break; // Light green
			case 20: hb::shared::text::draw_text(GameFont::Default, sX + 25, y_pos, pMsg, hb::shared::text::TextStyle::with_shadow(GameColors::UINormalChat)); break; // Gray
			}
		}
	}
}

bool DialogBox_ChatHistory::on_click(short mouse_x, short mouse_y)
{
	// Chat history dialog has no click actions - scrolling is handled in on_draw
	return false;
}

PressResult DialogBox_ChatHistory::on_press(short mouse_x, short mouse_y)
{
	short sX = Info().m_x;
	short sY = Info().m_y;

	// Check if click is in scroll bar region (track + buttons)
	if ((mouse_x >= sX + 336) && (mouse_x <= sX + 361) && (mouse_y >= sY + 18) && (mouse_y <= sY + 163))
	{
		return PressResult::ScrollClaimed;
	}

	return PressResult::Normal;
}


#include "DialogBox_CrusadeJob.h"
#include "Game.h"
#include "GameFonts.h"
#include "GlobalDef.h"
#include "TextLibExt.h"
#include "lan_eng.h"

using namespace hb::shared::net;
using namespace hb::client::sprite_id;
DialogBox_CrusadeJob::DialogBox_CrusadeJob(CGame* game)
	: IDialogBox(DialogBoxId::CrusadeJob, game)
{
	set_default_rect(520 , 65 , 258, 339);
}

void DialogBox_CrusadeJob::draw_mode_select_job(short sX, short sY, short mouse_x, short mouse_y)
{
	put_aligned_string(sX + 24, sX + 246, sY + 45 + 20, DRAWDIALOGBOX_CRUSADEJOB1);
	put_aligned_string(sX + 24, sX + 246, sY + 60 + 20, DRAWDIALOGBOX_CRUSADEJOB2);
	put_aligned_string(sX + 24, sX + 246, sY + 75 + 20, DRAWDIALOGBOX_CRUSADEJOB3);
	put_aligned_string(sX + 24, sX + 246, sY + 90 + 20, DRAWDIALOGBOX_CRUSADEJOB4);

	if (m_game->m_player->m_citizen)
	{
		if (m_game->m_player->m_guild_rank == 0)
		{
			// Guild master can be Commander
			if ((mouse_x > sX + 24) && (mouse_x < sX + 246) && (mouse_y > sY + 150) && (mouse_y < sY + 165))
				put_aligned_string(sX + 24, sX + 246, sY + 150, DRAWDIALOGBOX_CRUSADEJOB5, GameColors::UIWhite);
			else
				put_aligned_string(sX + 24, sX + 246, sY + 150, DRAWDIALOGBOX_CRUSADEJOB5, GameColors::UIMagicBlue);
		}
		else
		{
			// Non-guild masters can be Soldier
			if ((mouse_x > sX + 24) && (mouse_x < sX + 246) && (mouse_y > sY + 150) && (mouse_y < sY + 165))
				put_aligned_string(sX + 24, sX + 246, sY + 150, DRAWDIALOGBOX_CRUSADEJOB7, GameColors::UIWhite);
			else
				put_aligned_string(sX + 24, sX + 246, sY + 150, DRAWDIALOGBOX_CRUSADEJOB7, GameColors::UIMagicBlue);

			// Guild members can also be Constructor
			if (m_game->m_player->m_guild_rank != -1)
			{
				if ((mouse_x > sX + 24) && (mouse_x < sX + 246) && (mouse_y > sY + 175) && (mouse_y < sY + 190))
					put_aligned_string(sX + 24, sX + 246, sY + 175, DRAWDIALOGBOX_CRUSADEJOB9, GameColors::UIWhite);
				else
					put_aligned_string(sX + 24, sX + 246, sY + 175, DRAWDIALOGBOX_CRUSADEJOB9, GameColors::UIMagicBlue);
			}
		}
	}

	put_aligned_string(sX + 24, sX + 246, sY + 290 - 40, DRAWDIALOGBOX_CRUSADEJOB10);
	put_aligned_string(sX + 24, sX + 246, sY + 305 - 40, DRAWDIALOGBOX_CRUSADEJOB17);

	// Help button
	if ((mouse_x > sX + 210) && (mouse_x < sX + 260) && (mouse_y >= sY + 296) && (mouse_y <= sY + 316))
		hb::shared::text::draw_text(GameFont::Bitmap1, sX + 50 + 160, sY + 296, "Help", hb::shared::text::TextStyle::with_highlight(GameColors::UIMagicBlue));
	else
		hb::shared::text::draw_text(GameFont::Bitmap1, sX + 50 + 160, sY + 296, "Help", hb::shared::text::TextStyle::with_highlight(GameColors::BmpBtnNormal));
}

void DialogBox_CrusadeJob::draw_mode_confirm(short sX, short sY, short mouse_x, short mouse_y)
{
	put_aligned_string(sX + 24, sX + 246, sY + 90 + 20, DRAWDIALOGBOX_CRUSADEJOB18);

	switch (m_game->m_player->m_crusade_duty)
	{
	case 1: put_aligned_string(sX + 24, sX + 246, sY + 125, DRAWDIALOGBOX_CRUSADEJOB19); break;
	case 2: put_aligned_string(sX + 24, sX + 246, sY + 125, DRAWDIALOGBOX_CRUSADEJOB20); break;
	case 3: put_aligned_string(sX + 24, sX + 246, sY + 125, DRAWDIALOGBOX_CRUSADEJOB21); break;
	}

	put_aligned_string(sX + 24, sX + 246, sY + 145, DRAWDIALOGBOX_CRUSADEJOB22);

	// "View details" link
	if ((mouse_x > sX + 24) && (mouse_x < sX + 246) && (mouse_y > sY + 160) && (mouse_y < sY + 175))
		put_aligned_string(sX + 24, sX + 246, sY + 160, DRAWDIALOGBOX_CRUSADEJOB23, GameColors::UIWhite);
	else
		put_aligned_string(sX + 24, sX + 246, sY + 160, DRAWDIALOGBOX_CRUSADEJOB23, GameColors::UIMagicBlue);

	put_aligned_string(sX + 24, sX + 246, sY + 175, DRAWDIALOGBOX_CRUSADEJOB25);
	put_aligned_string(sX + 24, sX + 246, sY + 190, DRAWDIALOGBOX_CRUSADEJOB26);

	// OK button
	if ((mouse_x >= sX + ui_layout::right_btn_x) && (mouse_x <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) &&
		(mouse_y > sY + ui_layout::btn_y) && (mouse_y < sY + ui_layout::btn_y + ui_layout::btn_size_y))
		draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 1);
	else
		draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 0);
}

void DialogBox_CrusadeJob::on_draw(short mouse_x, short mouse_y, short z, char lb)
{
	short sX = Info().m_x;
	short sY = Info().m_y;

	draw_new_dialog_box(InterfaceNdGame2, sX, sY, 0);

	switch (Info().m_mode)
	{
	case 1:
		draw_mode_select_job(sX, sY, mouse_x, mouse_y);
		break;
	case 2:
		draw_mode_confirm(sX, sY, mouse_x, mouse_y);
		break;
	}
}

bool DialogBox_CrusadeJob::on_click(short mouse_x, short mouse_y)
{
	short sX = Info().m_x;
	short sY = Info().m_y;

	switch (Info().m_mode)
	{
	case 1:
		if (!m_game->m_player->m_citizen)
		{
			disable_dialog_box(DialogBoxId::CrusadeJob);
			m_game->play_game_sound('E', 14, 5);
			return true;
		}

		if (m_game->m_player->m_guild_rank == 0)
		{
			// Guild master - Commander option
			if ((mouse_x > sX + 24) && (mouse_x < sX + 246) && (mouse_y > sY + 150) && (mouse_y < sY + 165))
			{
				m_game->send_command(MsgId::CommandCommon, CommonType::RequestSelectCrusadeDuty, 0, 3, 0, 0, 0);
				disable_dialog_box(DialogBoxId::CrusadeJob);
				m_game->play_game_sound('E', 14, 5);
				return true;
			}
		}
		else
		{
			// Soldier option
			if ((mouse_x > sX + 24) && (mouse_x < sX + 246) && (mouse_y > sY + 150) && (mouse_y < sY + 165))
			{
				m_game->send_command(MsgId::CommandCommon, CommonType::RequestSelectCrusadeDuty, 0, 1, 0, 0, 0);
				disable_dialog_box(DialogBoxId::CrusadeJob);
				m_game->play_game_sound('E', 14, 5);
				return true;
			}

			// Constructor option (guild members only)
			if (m_game->m_player->m_guild_rank != -1)
			{
				if ((mouse_x > sX + 24) && (mouse_x < sX + 246) && (mouse_y > sY + 175) && (mouse_y < sY + 190))
				{
					m_game->send_command(MsgId::CommandCommon, CommonType::RequestSelectCrusadeDuty, 0, 2, 0, 0, 0);
					disable_dialog_box(DialogBoxId::CrusadeJob);
					m_game->play_game_sound('E', 14, 5);
					return true;
				}
			}
		}

		// Help button
		if ((mouse_x > sX + 210) && (mouse_x < sX + 260) && (mouse_y >= sY + 296) && (mouse_y <= sY + 316))
		{
			m_game->m_dialog_box_manager.disable_dialog_box(DialogBoxId::Text);
			m_game->m_dialog_box_manager.enable_dialog_box(DialogBoxId::Text, 813, 0, 0);
			m_game->play_game_sound('E', 14, 5);
			return true;
		}
		break;

	case 2:
		// View details link
		if ((mouse_x > sX + 24) && (mouse_x < sX + 246) && (mouse_y > sY + 160) && (mouse_y < sY + 175))
		{
			switch (m_game->m_player->m_crusade_duty)
			{
			case 1: m_game->m_dialog_box_manager.enable_dialog_box(DialogBoxId::Text, 803, 0, 0); break;
			case 2: m_game->m_dialog_box_manager.enable_dialog_box(DialogBoxId::Text, 805, 0, 0); break;
			case 3: m_game->m_dialog_box_manager.enable_dialog_box(DialogBoxId::Text, 808, 0, 0); break;
			}
			return true;
		}

		// OK button
		if ((mouse_x >= sX + ui_layout::right_btn_x) && (mouse_x <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) &&
			(mouse_y > sY + ui_layout::btn_y) && (mouse_y < sY + ui_layout::btn_y + ui_layout::btn_size_y))
		{
			disable_dialog_box(DialogBoxId::CrusadeJob);
			m_game->play_game_sound('E', 14, 5);
			return true;
		}
		break;
	}

	return false;
}

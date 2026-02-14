#include "DialogBox_Party.h"
#include "Game.h"
#include "lan_eng.h"
#include <format>
#include <string>

using namespace hb::shared::net;
using namespace hb::client::sprite_id;
DialogBox_Party::DialogBox_Party(CGame* game)
	: IDialogBox(DialogBoxId::Party, game)
{
	set_default_rect(0 , 0 , 258, 339);
}

void DialogBox_Party::on_draw(short mouse_x, short mouse_y, short z, char lb)
{
	short sX = Info().m_x;
	short sY = Info().m_y;
	short size_x = Info().m_size_x;

	m_game->draw_new_dialog_box(InterfaceNdGame2, sX, sY, 0);
	m_game->draw_new_dialog_box(InterfaceNdText, sX, sY, 3);

	switch (Info().m_mode) {
	case 0:
		if (m_game->m_party_status == 0) {
			if ((mouse_x > sX + 80) && (mouse_x < sX + 195) && (mouse_y > sY + 80) && (mouse_y < sY + 100))
				put_aligned_string(sX, sX + size_x, sY + 85, DRAW_DIALOGBOX_PARTY1, GameColors::UIWhite);
			else
				put_aligned_string(sX, sX + size_x, sY + 85, DRAW_DIALOGBOX_PARTY1, GameColors::UIMagicBlue);
		}
		else {
			put_aligned_string(sX, sX + size_x, sY + 85, DRAW_DIALOGBOX_PARTY1, GameColors::UIDisabled);
		}

		if (m_game->m_party_status != 0) {
			if ((mouse_x > sX + 80) && (mouse_x < sX + 195) && (mouse_y > sY + 100) && (mouse_y < sY + 120))
				put_aligned_string(sX, sX + size_x, sY + 105, DRAW_DIALOGBOX_PARTY4, GameColors::UIWhite);
			else
				put_aligned_string(sX, sX + size_x, sY + 105, DRAW_DIALOGBOX_PARTY4, GameColors::UIMagicBlue);
		}
		else {
			put_aligned_string(sX, sX + size_x, sY + 105, DRAW_DIALOGBOX_PARTY4, GameColors::UIDisabled);
		}

		if (m_game->m_party_status != 0) {
			if ((mouse_x > sX + 80) && (mouse_x < sX + 195) && (mouse_y > sY + 120) && (mouse_y < sY + 140))
				put_aligned_string(sX, sX + size_x, sY + 125, DRAW_DIALOGBOX_PARTY7, GameColors::UIWhite);
			else
				put_aligned_string(sX, sX + size_x, sY + 125, DRAW_DIALOGBOX_PARTY7, GameColors::UIMagicBlue);
		}
		else {
			put_aligned_string(sX, sX + size_x, sY + 125, DRAW_DIALOGBOX_PARTY7, GameColors::UIDisabled);
		}

		switch (m_game->m_party_status) {
		case 0:
			put_aligned_string(sX, sX + size_x, sY + 155, DRAW_DIALOGBOX_PARTY10);
			put_aligned_string(sX, sX + size_x, sY + 170, DRAW_DIALOGBOX_PARTY11);
			put_aligned_string(sX, sX + size_x, sY + 185, DRAW_DIALOGBOX_PARTY12);
			break;
		case 1:
		case 2:
			put_aligned_string(sX, sX + size_x, sY + 155, DRAW_DIALOGBOX_PARTY13);
			put_aligned_string(sX, sX + size_x, sY + 170, DRAW_DIALOGBOX_PARTY14);
			put_aligned_string(sX, sX + size_x, sY + 185, DRAW_DIALOGBOX_PARTY15);
			break;
		}

		if ((mouse_x >= sX + ui_layout::right_btn_x) && (mouse_x <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (mouse_y > sY + ui_layout::btn_y) && (mouse_y < sY + ui_layout::btn_y + ui_layout::btn_size_y))
			m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 1);
		else
			m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 0);
		break;

	case 1: {
		std::string partyBuf;
		partyBuf = std::format(DRAW_DIALOGBOX_PARTY16, Info().m_str);
		put_aligned_string(sX, sX + size_x, sY + 95, partyBuf.c_str());
		put_aligned_string(sX, sX + size_x, sY + 110, DRAW_DIALOGBOX_PARTY17);
		put_aligned_string(sX, sX + size_x, sY + 125, DRAW_DIALOGBOX_PARTY18);
		put_aligned_string(sX, sX + size_x, sY + 140, DRAW_DIALOGBOX_PARTY19);
		put_aligned_string(sX, sX + size_x, sY + 155, DRAW_DIALOGBOX_PARTY20);
		put_aligned_string(sX, sX + size_x, sY + 175, DRAW_DIALOGBOX_PARTY21);

		if ((mouse_x >= sX + ui_layout::left_btn_x) && (mouse_x <= sX + ui_layout::left_btn_x + ui_layout::btn_size_x) && (mouse_y >= sY + ui_layout::btn_y) && (mouse_y <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
			m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 19);
		else
			m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 18);

		if ((mouse_x >= sX + ui_layout::right_btn_x) && (mouse_x <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (mouse_y >= sY + ui_layout::btn_y) && (mouse_y <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
			m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 3);
		else
			m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 2);
		break;
	}

	case 2:
		put_aligned_string(sX, sX + size_x, sY + 95, DRAW_DIALOGBOX_PARTY22);
		put_aligned_string(sX, sX + size_x, sY + 110, DRAW_DIALOGBOX_PARTY23);
		put_aligned_string(sX, sX + size_x, sY + 125, DRAW_DIALOGBOX_PARTY24);
		put_aligned_string(sX, sX + size_x, sY + 140, DRAW_DIALOGBOX_PARTY25);

		if ((mouse_x >= sX + ui_layout::right_btn_x) && (mouse_x <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (mouse_y > sY + ui_layout::btn_y) && (mouse_y < sY + ui_layout::btn_y + ui_layout::btn_size_y))
			m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 17);
		else
			m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 16);
		break;

	case 3: {
		std::string partyBuf;
		partyBuf = std::format(DRAW_DIALOGBOX_PARTY26, Info().m_str);
		put_aligned_string(sX, sX + size_x, sY + 95, partyBuf.c_str());
		put_aligned_string(sX, sX + size_x, sY + 110, DRAW_DIALOGBOX_PARTY27);
		put_aligned_string(sX, sX + size_x, sY + 125, DRAW_DIALOGBOX_PARTY28);
		put_aligned_string(sX, sX + size_x, sY + 140, DRAW_DIALOGBOX_PARTY29);
		put_aligned_string(sX, sX + size_x, sY + 155, DRAW_DIALOGBOX_PARTY30);

		if ((mouse_x >= sX + ui_layout::right_btn_x) && (mouse_x <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (mouse_y > sY + ui_layout::btn_y) && (mouse_y < sY + ui_layout::btn_y + ui_layout::btn_size_y))
			m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 17);
		else
			m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 16);
		break;
	}

	case 4: {
		put_aligned_string(sX, sX + size_x, sY + 95, DRAW_DIALOGBOX_PARTY31);
		put_aligned_string(sX, sX + size_x, sY + 110, DRAW_DIALOGBOX_PARTY32);

		int nth = 0;
		for (int i = 0; i <= hb::shared::limits::MaxPartyMembers; i++) {
			if (m_game->m_party_member_name_list[i].name.size() != 0) {
				put_aligned_string(sX + 17, sX + 270, sY + 140 + 15 * nth, m_game->m_party_member_name_list[i].name.c_str());
				nth++;
			}
		}

		if ((mouse_x >= sX + ui_layout::right_btn_x) && (mouse_x <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (mouse_y > sY + ui_layout::btn_y) && (mouse_y < sY + ui_layout::btn_y + ui_layout::btn_size_y))
			m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 1);
		else
			m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 0);
		break;
	}

	case 5:
		put_aligned_string(sX, sX + size_x, sY + 95, DRAW_DIALOGBOX_PARTY33);
		put_aligned_string(sX, sX + size_x, sY + 110, DRAW_DIALOGBOX_PARTY34);
		break;

	case 6:
		put_aligned_string(sX, sX + size_x, sY + 95, DRAW_DIALOGBOX_PARTY35);
		if ((mouse_x >= sX + ui_layout::right_btn_x) && (mouse_x <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (mouse_y > sY + ui_layout::btn_y) && (mouse_y < sY + ui_layout::btn_y + ui_layout::btn_size_y))
			m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 1);
		else
			m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 0);
		break;

	case 7:
		put_aligned_string(sX, sX + size_x, sY + 95, DRAW_DIALOGBOX_PARTY36);
		put_aligned_string(sX, sX + size_x, sY + 110, DRAW_DIALOGBOX_PARTY37);
		put_aligned_string(sX, sX + size_x, sY + 125, DRAW_DIALOGBOX_PARTY38);
		put_aligned_string(sX, sX + size_x, sY + 140, DRAW_DIALOGBOX_PARTY39);
		if ((mouse_x >= sX + ui_layout::right_btn_x) && (mouse_x <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (mouse_y > sY + ui_layout::btn_y) && (mouse_y < sY + ui_layout::btn_y + ui_layout::btn_size_y))
			m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 1);
		else
			m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 0);
		break;

	case 8:
		put_aligned_string(sX, sX + size_x, sY + 95, DRAW_DIALOGBOX_PARTY40);
		put_aligned_string(sX, sX + size_x, sY + 110, DRAW_DIALOGBOX_PARTY41);
		put_aligned_string(sX, sX + size_x, sY + 125, DRAW_DIALOGBOX_PARTY42);
		put_aligned_string(sX, sX + size_x, sY + 140, DRAW_DIALOGBOX_PARTY43);
		put_aligned_string(sX, sX + size_x, sY + 155, DRAW_DIALOGBOX_PARTY44);
		put_aligned_string(sX, sX + size_x, sY + 170, DRAW_DIALOGBOX_PARTY45);

		if ((mouse_x >= sX + ui_layout::right_btn_x) && (mouse_x <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (mouse_y > sY + ui_layout::btn_y) && (mouse_y < sY + ui_layout::btn_y + ui_layout::btn_size_y))
			m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 1);
		else
			m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 0);
		break;

	case 9:
		put_aligned_string(sX, sX + size_x, sY + 95, DRAW_DIALOGBOX_PARTY46);
		put_aligned_string(sX, sX + size_x, sY + 110, DRAW_DIALOGBOX_PARTY47);
		put_aligned_string(sX, sX + size_x, sY + 125, DRAW_DIALOGBOX_PARTY48);
		put_aligned_string(sX, sX + size_x, sY + 140, DRAW_DIALOGBOX_PARTY49);
		put_aligned_string(sX, sX + size_x, sY + 155, DRAW_DIALOGBOX_PARTY50);
		put_aligned_string(sX, sX + size_x, sY + 170, DRAW_DIALOGBOX_PARTY51);

		if ((mouse_x >= sX + ui_layout::right_btn_x) && (mouse_x <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (mouse_y > sY + ui_layout::btn_y) && (mouse_y < sY + ui_layout::btn_y + ui_layout::btn_size_y))
			m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 1);
		else
			m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 0);
		break;

	case 10:
		put_aligned_string(sX, sX + size_x, sY + 95, DRAW_DIALOGBOX_PARTY52);
		put_aligned_string(sX, sX + size_x, sY + 110, DRAW_DIALOGBOX_PARTY53);
		put_aligned_string(sX, sX + size_x, sY + 125, DRAW_DIALOGBOX_PARTY54);
		if ((mouse_x >= sX + ui_layout::right_btn_x) && (mouse_x <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (mouse_y > sY + ui_layout::btn_y) && (mouse_y < sY + ui_layout::btn_y + ui_layout::btn_size_y))
			m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 1);
		else
			m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 0);
		break;

	case 11:
		put_aligned_string(sX, sX + size_x, sY + 95, DRAW_DIALOGBOX_PARTY55);
		if ((mouse_x >= sX + ui_layout::left_btn_x) && (mouse_x <= sX + ui_layout::left_btn_x + ui_layout::btn_size_x) && (mouse_y >= sY + ui_layout::btn_y) && (mouse_y <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
			m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 19);
		else
			m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 18);

		if ((mouse_x >= sX + ui_layout::right_btn_x) && (mouse_x <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (mouse_y >= sY + ui_layout::btn_y) && (mouse_y <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
			m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 3);
		else
			m_game->draw_new_dialog_box(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 2);
		break;
	}
}

bool DialogBox_Party::on_click(short mouse_x, short mouse_y)
{
	short sX = Info().m_x;
	short sY = Info().m_y;

	switch (Info().m_mode) {
	case 0:
		if (m_game->m_party_status == 0) {
			if ((mouse_x > sX + 80) && (mouse_x < sX + 195) && (mouse_y > sY + 80) && (mouse_y < sY + 100)) {
				Info().m_mode = 2;
				m_game->m_is_get_pointing_mode = true;
				m_game->m_point_command_type = 200;
				play_sound_effect('E', 14, 5);
				return true;
			}
		}

		if (m_game->m_party_status != 0) {
			if ((mouse_x > sX + 80) && (mouse_x < sX + 195) && (mouse_y > sY + 100) && (mouse_y < sY + 120)) {
				Info().m_mode = 11;
				play_sound_effect('E', 14, 5);
				return true;
			}
		}

		if (m_game->m_party_status != 0) {
			if ((mouse_x > sX + 80) && (mouse_x < sX + 195) && (mouse_y > sY + 120) && (mouse_y < sY + 140)) {
				send_command(MsgId::CommandCommon, CommonType::RequestJoinParty, 0, 2, 0, 0, m_game->m_mc_name.c_str());
				Info().m_mode = 4;
				play_sound_effect('E', 14, 5);
				return true;
			}
		}

		if ((mouse_x >= sX + ui_layout::right_btn_x) && (mouse_x <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (mouse_y > sY + ui_layout::btn_y) && (mouse_y < sY + ui_layout::btn_y + ui_layout::btn_size_y)) {
			m_game->m_dialog_box_manager.disable_dialog_box(DialogBoxId::Party);
			return true;
		}
		break;

	case 1:
		if ((mouse_x >= sX + ui_layout::left_btn_x) && (mouse_x <= sX + ui_layout::left_btn_x + ui_layout::btn_size_x) && (mouse_y >= sY + ui_layout::btn_y) && (mouse_y <= sY + ui_layout::btn_y + ui_layout::btn_size_y)) {
			send_command(MsgId::CommandCommon, CommonType::RequestAcceptJoinParty, 0, 1, 0, 0, Info().m_str);
			m_game->m_dialog_box_manager.disable_dialog_box(DialogBoxId::Party);
			play_sound_effect('E', 14, 5);
			return true;
		}

		if ((mouse_x >= sX + ui_layout::right_btn_x) && (mouse_x <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (mouse_y >= sY + ui_layout::btn_y) && (mouse_y <= sY + ui_layout::btn_y + ui_layout::btn_size_y)) {
			send_command(MsgId::CommandCommon, CommonType::RequestAcceptJoinParty, 0, 0, 0, 0, Info().m_str);
			m_game->m_dialog_box_manager.disable_dialog_box(DialogBoxId::Party);
			play_sound_effect('E', 14, 5);
			return true;
		}
		break;

	case 2:
		if ((mouse_x >= sX + ui_layout::right_btn_x) && (mouse_x <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (mouse_y > sY + ui_layout::btn_y) && (mouse_y < sY + ui_layout::btn_y + ui_layout::btn_size_y)) {
			Info().m_mode = 0;
			play_sound_effect('E', 14, 5);
			return true;
		}
		break;

	case 3:
		if ((mouse_x >= sX + ui_layout::right_btn_x) && (mouse_x <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (mouse_y > sY + ui_layout::btn_y) && (mouse_y < sY + ui_layout::btn_y + ui_layout::btn_size_y)) {
			Info().m_mode = 0;
			send_command(MsgId::CommandCommon, CommonType::RequestAcceptJoinParty, 0, 2, 0, 0, Info().m_str);
			m_game->m_dialog_box_manager.disable_dialog_box(DialogBoxId::Party);
			play_sound_effect('E', 14, 5);
			return true;
		}
		break;

	case 4:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
		if ((mouse_x >= sX + ui_layout::right_btn_x) && (mouse_x <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (mouse_y > sY + ui_layout::btn_y) && (mouse_y < sY + ui_layout::btn_y + ui_layout::btn_size_y)) {
			Info().m_mode = 0;
			play_sound_effect('E', 14, 5);
			return true;
		}
		break;

	case 11:
		if ((mouse_x >= sX + ui_layout::left_btn_x) && (mouse_x <= sX + ui_layout::left_btn_x + ui_layout::btn_size_x) && (mouse_y >= sY + ui_layout::btn_y) && (mouse_y <= sY + ui_layout::btn_y + ui_layout::btn_size_y)) {
			send_command(MsgId::CommandCommon, CommonType::RequestJoinParty, 0, 0, 0, 0, m_game->m_mc_name.c_str());
			Info().m_mode = 5;
			play_sound_effect('E', 14, 5);
			return true;
		}

		if ((mouse_x >= sX + ui_layout::right_btn_x) && (mouse_x <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (mouse_y >= sY + ui_layout::btn_y) && (mouse_y <= sY + ui_layout::btn_y + ui_layout::btn_size_y)) {
			Info().m_mode = 0;
			play_sound_effect('E', 14, 5);
			return true;
		}
		break;
	}

	return false;
}

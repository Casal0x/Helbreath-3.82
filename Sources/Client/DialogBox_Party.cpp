#include "DialogBox_Party.h"
#include "Game.h"
#include "lan_eng.h"
#include <format>
#include <string>

using namespace hb::shared::net;
using namespace hb::client::sprite_id;
DialogBox_Party::DialogBox_Party(CGame* pGame)
	: IDialogBox(DialogBoxId::Party, pGame)
{
	SetDefaultRect(0 , 0 , 258, 339);
}

void DialogBox_Party::OnDraw(short msX, short msY, short msZ, char cLB)
{
	short sX = Info().sX;
	short sY = Info().sY;
	short szX = Info().sSizeX;

	m_pGame->DrawNewDialogBox(InterfaceNdGame2, sX, sY, 0);
	m_pGame->DrawNewDialogBox(InterfaceNdText, sX, sY, 3);

	switch (Info().cMode) {
	case 0:
		if (m_pGame->m_iPartyStatus == 0) {
			if ((msX > sX + 80) && (msX < sX + 195) && (msY > sY + 80) && (msY < sY + 100))
				PutAlignedString(sX, sX + szX, sY + 85, DRAW_DIALOGBOX_PARTY1, GameColors::UIWhite);
			else
				PutAlignedString(sX, sX + szX, sY + 85, DRAW_DIALOGBOX_PARTY1, GameColors::UIMagicBlue);
		}
		else {
			PutAlignedString(sX, sX + szX, sY + 85, DRAW_DIALOGBOX_PARTY1, GameColors::UIDisabled);
		}

		if (m_pGame->m_iPartyStatus != 0) {
			if ((msX > sX + 80) && (msX < sX + 195) && (msY > sY + 100) && (msY < sY + 120))
				PutAlignedString(sX, sX + szX, sY + 105, DRAW_DIALOGBOX_PARTY4, GameColors::UIWhite);
			else
				PutAlignedString(sX, sX + szX, sY + 105, DRAW_DIALOGBOX_PARTY4, GameColors::UIMagicBlue);
		}
		else {
			PutAlignedString(sX, sX + szX, sY + 105, DRAW_DIALOGBOX_PARTY4, GameColors::UIDisabled);
		}

		if (m_pGame->m_iPartyStatus != 0) {
			if ((msX > sX + 80) && (msX < sX + 195) && (msY > sY + 120) && (msY < sY + 140))
				PutAlignedString(sX, sX + szX, sY + 125, DRAW_DIALOGBOX_PARTY7, GameColors::UIWhite);
			else
				PutAlignedString(sX, sX + szX, sY + 125, DRAW_DIALOGBOX_PARTY7, GameColors::UIMagicBlue);
		}
		else {
			PutAlignedString(sX, sX + szX, sY + 125, DRAW_DIALOGBOX_PARTY7, GameColors::UIDisabled);
		}

		switch (m_pGame->m_iPartyStatus) {
		case 0:
			PutAlignedString(sX, sX + szX, sY + 155, DRAW_DIALOGBOX_PARTY10);
			PutAlignedString(sX, sX + szX, sY + 170, DRAW_DIALOGBOX_PARTY11);
			PutAlignedString(sX, sX + szX, sY + 185, DRAW_DIALOGBOX_PARTY12);
			break;
		case 1:
		case 2:
			PutAlignedString(sX, sX + szX, sY + 155, DRAW_DIALOGBOX_PARTY13);
			PutAlignedString(sX, sX + szX, sY + 170, DRAW_DIALOGBOX_PARTY14);
			PutAlignedString(sX, sX + szX, sY + 185, DRAW_DIALOGBOX_PARTY15);
			break;
		}

		if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY > sY + ui_layout::btn_y) && (msY < sY + ui_layout::btn_y + ui_layout::btn_size_y))
			m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 1);
		else
			m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 0);
		break;

	case 1: {
		std::string partyBuf;
		partyBuf = std::format(DRAW_DIALOGBOX_PARTY16, Info().cStr);
		PutAlignedString(sX, sX + szX, sY + 95, partyBuf.c_str());
		PutAlignedString(sX, sX + szX, sY + 110, DRAW_DIALOGBOX_PARTY17);
		PutAlignedString(sX, sX + szX, sY + 125, DRAW_DIALOGBOX_PARTY18);
		PutAlignedString(sX, sX + szX, sY + 140, DRAW_DIALOGBOX_PARTY19);
		PutAlignedString(sX, sX + szX, sY + 155, DRAW_DIALOGBOX_PARTY20);
		PutAlignedString(sX, sX + szX, sY + 175, DRAW_DIALOGBOX_PARTY21);

		if ((msX >= sX + ui_layout::left_btn_x) && (msX <= sX + ui_layout::left_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
			m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 19);
		else
			m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 18);

		if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
			m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 3);
		else
			m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 2);
		break;
	}

	case 2:
		PutAlignedString(sX, sX + szX, sY + 95, DRAW_DIALOGBOX_PARTY22);
		PutAlignedString(sX, sX + szX, sY + 110, DRAW_DIALOGBOX_PARTY23);
		PutAlignedString(sX, sX + szX, sY + 125, DRAW_DIALOGBOX_PARTY24);
		PutAlignedString(sX, sX + szX, sY + 140, DRAW_DIALOGBOX_PARTY25);

		if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY > sY + ui_layout::btn_y) && (msY < sY + ui_layout::btn_y + ui_layout::btn_size_y))
			m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 17);
		else
			m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 16);
		break;

	case 3: {
		std::string partyBuf;
		partyBuf = std::format(DRAW_DIALOGBOX_PARTY26, Info().cStr);
		PutAlignedString(sX, sX + szX, sY + 95, partyBuf.c_str());
		PutAlignedString(sX, sX + szX, sY + 110, DRAW_DIALOGBOX_PARTY27);
		PutAlignedString(sX, sX + szX, sY + 125, DRAW_DIALOGBOX_PARTY28);
		PutAlignedString(sX, sX + szX, sY + 140, DRAW_DIALOGBOX_PARTY29);
		PutAlignedString(sX, sX + szX, sY + 155, DRAW_DIALOGBOX_PARTY30);

		if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY > sY + ui_layout::btn_y) && (msY < sY + ui_layout::btn_y + ui_layout::btn_size_y))
			m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 17);
		else
			m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 16);
		break;
	}

	case 4: {
		PutAlignedString(sX, sX + szX, sY + 95, DRAW_DIALOGBOX_PARTY31);
		PutAlignedString(sX, sX + szX, sY + 110, DRAW_DIALOGBOX_PARTY32);

		int iNth = 0;
		for (int i = 0; i <= hb::shared::limits::MaxPartyMembers; i++) {
			if (m_pGame->m_stPartyMemberNameList[i].cName.size() != 0) {
				PutAlignedString(sX + 17, sX + 270, sY + 140 + 15 * iNth, m_pGame->m_stPartyMemberNameList[i].cName.c_str());
				iNth++;
			}
		}

		if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY > sY + ui_layout::btn_y) && (msY < sY + ui_layout::btn_y + ui_layout::btn_size_y))
			m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 1);
		else
			m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 0);
		break;
	}

	case 5:
		PutAlignedString(sX, sX + szX, sY + 95, DRAW_DIALOGBOX_PARTY33);
		PutAlignedString(sX, sX + szX, sY + 110, DRAW_DIALOGBOX_PARTY34);
		break;

	case 6:
		PutAlignedString(sX, sX + szX, sY + 95, DRAW_DIALOGBOX_PARTY35);
		if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY > sY + ui_layout::btn_y) && (msY < sY + ui_layout::btn_y + ui_layout::btn_size_y))
			m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 1);
		else
			m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 0);
		break;

	case 7:
		PutAlignedString(sX, sX + szX, sY + 95, DRAW_DIALOGBOX_PARTY36);
		PutAlignedString(sX, sX + szX, sY + 110, DRAW_DIALOGBOX_PARTY37);
		PutAlignedString(sX, sX + szX, sY + 125, DRAW_DIALOGBOX_PARTY38);
		PutAlignedString(sX, sX + szX, sY + 140, DRAW_DIALOGBOX_PARTY39);
		if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY > sY + ui_layout::btn_y) && (msY < sY + ui_layout::btn_y + ui_layout::btn_size_y))
			m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 1);
		else
			m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 0);
		break;

	case 8:
		PutAlignedString(sX, sX + szX, sY + 95, DRAW_DIALOGBOX_PARTY40);
		PutAlignedString(sX, sX + szX, sY + 110, DRAW_DIALOGBOX_PARTY41);
		PutAlignedString(sX, sX + szX, sY + 125, DRAW_DIALOGBOX_PARTY42);
		PutAlignedString(sX, sX + szX, sY + 140, DRAW_DIALOGBOX_PARTY43);
		PutAlignedString(sX, sX + szX, sY + 155, DRAW_DIALOGBOX_PARTY44);
		PutAlignedString(sX, sX + szX, sY + 170, DRAW_DIALOGBOX_PARTY45);

		if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY > sY + ui_layout::btn_y) && (msY < sY + ui_layout::btn_y + ui_layout::btn_size_y))
			m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 1);
		else
			m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 0);
		break;

	case 9:
		PutAlignedString(sX, sX + szX, sY + 95, DRAW_DIALOGBOX_PARTY46);
		PutAlignedString(sX, sX + szX, sY + 110, DRAW_DIALOGBOX_PARTY47);
		PutAlignedString(sX, sX + szX, sY + 125, DRAW_DIALOGBOX_PARTY48);
		PutAlignedString(sX, sX + szX, sY + 140, DRAW_DIALOGBOX_PARTY49);
		PutAlignedString(sX, sX + szX, sY + 155, DRAW_DIALOGBOX_PARTY50);
		PutAlignedString(sX, sX + szX, sY + 170, DRAW_DIALOGBOX_PARTY51);

		if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY > sY + ui_layout::btn_y) && (msY < sY + ui_layout::btn_y + ui_layout::btn_size_y))
			m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 1);
		else
			m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 0);
		break;

	case 10:
		PutAlignedString(sX, sX + szX, sY + 95, DRAW_DIALOGBOX_PARTY52);
		PutAlignedString(sX, sX + szX, sY + 110, DRAW_DIALOGBOX_PARTY53);
		PutAlignedString(sX, sX + szX, sY + 125, DRAW_DIALOGBOX_PARTY54);
		if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY > sY + ui_layout::btn_y) && (msY < sY + ui_layout::btn_y + ui_layout::btn_size_y))
			m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 1);
		else
			m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 0);
		break;

	case 11:
		PutAlignedString(sX, sX + szX, sY + 95, DRAW_DIALOGBOX_PARTY55);
		if ((msX >= sX + ui_layout::left_btn_x) && (msX <= sX + ui_layout::left_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
			m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 19);
		else
			m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 18);

		if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
			m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 3);
		else
			m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 2);
		break;
	}
}

bool DialogBox_Party::OnClick(short msX, short msY)
{
	short sX = Info().sX;
	short sY = Info().sY;

	switch (Info().cMode) {
	case 0:
		if (m_pGame->m_iPartyStatus == 0) {
			if ((msX > sX + 80) && (msX < sX + 195) && (msY > sY + 80) && (msY < sY + 100)) {
				Info().cMode = 2;
				m_pGame->m_bIsGetPointingMode = true;
				m_pGame->m_iPointCommandType = 200;
				PlaySoundEffect('E', 14, 5);
				return true;
			}
		}

		if (m_pGame->m_iPartyStatus != 0) {
			if ((msX > sX + 80) && (msX < sX + 195) && (msY > sY + 100) && (msY < sY + 120)) {
				Info().cMode = 11;
				PlaySoundEffect('E', 14, 5);
				return true;
			}
		}

		if (m_pGame->m_iPartyStatus != 0) {
			if ((msX > sX + 80) && (msX < sX + 195) && (msY > sY + 120) && (msY < sY + 140)) {
				bSendCommand(MsgId::CommandCommon, CommonType::RequestJoinParty, 0, 2, 0, 0, m_pGame->m_cMCName.c_str());
				Info().cMode = 4;
				PlaySoundEffect('E', 14, 5);
				return true;
			}
		}

		if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY > sY + ui_layout::btn_y) && (msY < sY + ui_layout::btn_y + ui_layout::btn_size_y)) {
			m_pGame->m_dialogBoxManager.DisableDialogBox(DialogBoxId::Party);
			return true;
		}
		break;

	case 1:
		if ((msX >= sX + ui_layout::left_btn_x) && (msX <= sX + ui_layout::left_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y)) {
			bSendCommand(MsgId::CommandCommon, CommonType::RequestAcceptJoinParty, 0, 1, 0, 0, Info().cStr);
			m_pGame->m_dialogBoxManager.DisableDialogBox(DialogBoxId::Party);
			PlaySoundEffect('E', 14, 5);
			return true;
		}

		if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y)) {
			bSendCommand(MsgId::CommandCommon, CommonType::RequestAcceptJoinParty, 0, 0, 0, 0, Info().cStr);
			m_pGame->m_dialogBoxManager.DisableDialogBox(DialogBoxId::Party);
			PlaySoundEffect('E', 14, 5);
			return true;
		}
		break;

	case 2:
		if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY > sY + ui_layout::btn_y) && (msY < sY + ui_layout::btn_y + ui_layout::btn_size_y)) {
			Info().cMode = 0;
			PlaySoundEffect('E', 14, 5);
			return true;
		}
		break;

	case 3:
		if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY > sY + ui_layout::btn_y) && (msY < sY + ui_layout::btn_y + ui_layout::btn_size_y)) {
			Info().cMode = 0;
			bSendCommand(MsgId::CommandCommon, CommonType::RequestAcceptJoinParty, 0, 2, 0, 0, Info().cStr);
			m_pGame->m_dialogBoxManager.DisableDialogBox(DialogBoxId::Party);
			PlaySoundEffect('E', 14, 5);
			return true;
		}
		break;

	case 4:
	case 6:
	case 7:
	case 8:
	case 9:
	case 10:
		if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY > sY + ui_layout::btn_y) && (msY < sY + ui_layout::btn_y + ui_layout::btn_size_y)) {
			Info().cMode = 0;
			PlaySoundEffect('E', 14, 5);
			return true;
		}
		break;

	case 11:
		if ((msX >= sX + ui_layout::left_btn_x) && (msX <= sX + ui_layout::left_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y)) {
			bSendCommand(MsgId::CommandCommon, CommonType::RequestJoinParty, 0, 0, 0, 0, m_pGame->m_cMCName.c_str());
			Info().cMode = 5;
			PlaySoundEffect('E', 14, 5);
			return true;
		}

		if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y)) {
			Info().cMode = 0;
			PlaySoundEffect('E', 14, 5);
			return true;
		}
		break;
	}

	return false;
}

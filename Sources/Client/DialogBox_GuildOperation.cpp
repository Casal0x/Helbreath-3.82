#include "DialogBox_GuildOperation.h"
#include "Game.h"
#include "lan_eng.h"
#include <string>

using namespace hb::shared::net;
using namespace hb::client::sprite_id;
DialogBox_GuildOperation::DialogBox_GuildOperation(CGame* pGame)
	: IDialogBox(DialogBoxId::GuildOperation, pGame)
{
	SetDefaultRect(337 , 57 , 295, 346);
}

void DialogBox_GuildOperation::OnDraw(short msX, short msY, short msZ, char cLB)
{
	short sX = Info().sX;
	short sY = Info().sY;

	m_pGame->DrawNewDialogBox(InterfaceNdGame2, sX, sY, 0);
	m_pGame->DrawNewDialogBox(InterfaceNdText, sX, sY, 19);

	switch (m_pGame->m_stGuildOpList[0].cOpMode) {
	case 1:
		DrawJoinRequest(sX, sY, msX, msY);
		break;
	case 2:
		DrawDismissRequest(sX, sY, msX, msY);
		break;
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
		DrawInfoMessage(sX, sY, msX, msY, m_pGame->m_stGuildOpList[0].cOpMode);
		break;
	}
}

void DialogBox_GuildOperation::DrawJoinRequest(short sX, short sY, short msX, short msY)
{
	PutAlignedString(sX + 24, sX + 248, sY + 50, DRAW_DIALOGBOX_GUILD_OPERATION1);
	PutAlignedString(sX + 24, sX + 248, sY + 65, m_pGame->m_stGuildOpList[0].cName.c_str(), GameColors::UILabel);
	PutAlignedString(sX + 24, sX + 248, sY + 69, "____________________", GameColors::UIBlack);
	PutAlignedString(sX + 24, sX + 248, sY + 90, DRAW_DIALOGBOX_GUILD_OPERATION2);
	PutAlignedString(sX + 24, sX + 248, sY + 105, DRAW_DIALOGBOX_GUILD_OPERATION3);
	PutAlignedString(sX + 24, sX + 248, sY + 120, DRAW_DIALOGBOX_GUILD_OPERATION4);
	PutAlignedString(sX + 24, sX + 248, sY + 160, DRAW_DIALOGBOX_GUILD_OPERATION5, GameColors::UILabel);

	if ((msX >= sX + ui_layout::left_btn_x) && (msX <= sX + ui_layout::left_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
		m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 33);
	else m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 32);

	if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
		m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 35);
	else m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 34);
}

void DialogBox_GuildOperation::DrawDismissRequest(short sX, short sY, short msX, short msY)
{
	PutAlignedString(sX + 24, sX + 248, sY + 50, DRAW_DIALOGBOX_GUILD_OPERATION6);
	PutAlignedString(sX + 24, sX + 248, sY + 65, m_pGame->m_stGuildOpList[0].cName.c_str(), GameColors::UILabel);
	PutAlignedString(sX + 24, sX + 248, sY + 69, "____________________", GameColors::UIBlack);
	PutAlignedString(sX + 24, sX + 248, sY + 90, DRAW_DIALOGBOX_GUILD_OPERATION7);
	PutAlignedString(sX + 24, sX + 248, sY + 105, DRAW_DIALOGBOX_GUILD_OPERATION8);
	PutAlignedString(sX + 24, sX + 248, sY + 120, DRAW_DIALOGBOX_GUILD_OPERATION9);
	PutAlignedString(sX + 24, sX + 248, sY + 160, DRAW_DIALOGBOX_GUILD_OPERATION10, GameColors::UILabel);

	if ((msX >= sX + ui_layout::left_btn_x) && (msX <= sX + ui_layout::left_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
		m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 33);
	else m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 32);

	if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
		m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 35);
	else m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 34);
}

void DialogBox_GuildOperation::DrawInfoMessage(short sX, short sY, short msX, short msY, int mode)
{
	switch (mode) {
	case 3:
		PutAlignedString(sX + 24, sX + 248, sY + 50, DRAW_DIALOGBOX_GUILD_OPERATION11);
		PutAlignedString(sX + 24, sX + 248, sY + 65, m_pGame->m_stGuildOpList[0].cName.c_str(), GameColors::UILabel);
		PutAlignedString(sX + 24, sX + 248, sY + 69, "____________________", GameColors::UIBlack);
		PutAlignedString(sX + 24, sX + 248, sY + 90, DRAW_DIALOGBOX_GUILD_OPERATION12);
		PutAlignedString(sX + 24, sX + 248, sY + 105, DRAW_DIALOGBOX_GUILD_OPERATION13);
		break;
	case 4:
		PutAlignedString(sX + 24, sX + 248, sY + 50, DRAW_DIALOGBOX_GUILD_OPERATION14);
		PutAlignedString(sX + 24, sX + 248, sY + 65, m_pGame->m_stGuildOpList[0].cName.c_str(), GameColors::UILabel);
		PutAlignedString(sX + 24, sX + 248, sY + 69, "____________________", GameColors::UIBlack);
		PutAlignedString(sX + 24, sX + 248, sY + 90, DRAW_DIALOGBOX_GUILD_OPERATION15);
		PutAlignedString(sX + 24, sX + 248, sY + 105, DRAW_DIALOGBOX_GUILD_OPERATION16);
		break;
	case 5:
		PutAlignedString(sX + 24, sX + 248, sY + 50, DRAW_DIALOGBOX_GUILD_OPERATION17);
		PutAlignedString(sX + 24, sX + 248, sY + 65, m_pGame->m_stGuildOpList[0].cName.c_str(), GameColors::UILabel);
		PutAlignedString(sX + 24, sX + 248, sY + 69, "____________________", GameColors::UIBlack);
		PutAlignedString(sX + 24, sX + 248, sY + 90, DRAW_DIALOGBOX_GUILD_OPERATION18);
		PutAlignedString(sX + 24, sX + 248, sY + 105, DRAW_DIALOGBOX_GUILD_OPERATION19);
		PutAlignedString(sX + 24, sX + 248, sY + 120, DRAW_DIALOGBOX_GUILD_OPERATION20);
		break;
	case 6:
		PutAlignedString(sX + 24, sX + 248, sY + 50, DRAW_DIALOGBOX_GUILD_OPERATION21);
		PutAlignedString(sX + 24, sX + 248, sY + 65, m_pGame->m_stGuildOpList[0].cName.c_str(), GameColors::UILabel);
		PutAlignedString(sX + 24, sX + 248, sY + 69, "____________________", GameColors::UIBlack);
		PutAlignedString(sX + 24, sX + 248, sY + 90, DRAW_DIALOGBOX_GUILD_OPERATION22);
		PutAlignedString(sX + 24, sX + 248, sY + 105, DRAW_DIALOGBOX_GUILD_OPERATION23);
		break;
	case 7:
		PutAlignedString(sX + 24, sX + 248, sY + 50, DRAW_DIALOGBOX_GUILD_OPERATION24);
		PutAlignedString(sX + 24, sX + 248, sY + 90, DRAW_DIALOGBOX_GUILD_OPERATION25);
		PutAlignedString(sX + 24, sX + 248, sY + 105, DRAW_DIALOGBOX_GUILD_OPERATION26);
		PutAlignedString(sX + 24, sX + 248, sY + 120, DRAW_DIALOGBOX_GUILD_OPERATION27);
		break;
	}

	if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY > sY + ui_layout::btn_y) && (msY < sY + ui_layout::btn_y + ui_layout::btn_size_y))
		m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 1);
	else m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 0);
}

bool DialogBox_GuildOperation::OnClick(short msX, short msY)
{
	short sX = Info().sX;
	short sY = Info().sY;
	std::string cName20;


	switch (m_pGame->m_stGuildOpList[0].cOpMode) {
	case 3:
	case 4:
	case 5:
	case 6:
	case 7:
		if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY > sY + ui_layout::btn_y) && (msY < sY + ui_layout::btn_y + ui_layout::btn_size_y)) {
			m_pGame->_ShiftGuildOperationList();
			if (m_pGame->m_stGuildOpList[0].cOpMode == 0)
				m_pGame->m_dialogBoxManager.DisableDialogBox(DialogBoxId::GuildOperation);
			return true;
		}
		return false;
	}

	// Approve button
	if ((msX >= sX + ui_layout::left_btn_x) && (msX <= sX + ui_layout::left_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y)) {
		PlaySoundEffect('E', 14, 5);

		switch (m_pGame->m_stGuildOpList[0].cOpMode) {
		case 1:
			cName20 = m_pGame->m_stGuildOpList[0].cName;
			bSendCommand(MsgId::CommandCommon, CommonType::JoinGuildApprove, 0, 0, 0, 0, cName20.c_str());
			break;
		case 2:
			cName20 = m_pGame->m_stGuildOpList[0].cName;
			bSendCommand(MsgId::CommandCommon, CommonType::DismissGuildApprove, 0, 0, 0, 0, cName20.c_str());
			break;
		}
		m_pGame->_ShiftGuildOperationList();
		if (m_pGame->m_stGuildOpList[0].cOpMode == 0)
			m_pGame->m_dialogBoxManager.DisableDialogBox(DialogBoxId::GuildOperation);
		return true;
	}

	// Reject button
	if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) && (msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y)) {
		PlaySoundEffect('E', 14, 5);

		switch (m_pGame->m_stGuildOpList[0].cOpMode) {
		case 1:
			cName20 = m_pGame->m_stGuildOpList[0].cName;
			bSendCommand(MsgId::CommandCommon, CommonType::JoinGuildReject, 0, 0, 0, 0, cName20.c_str());
			break;
		case 2:
			cName20 = m_pGame->m_stGuildOpList[0].cName;
			bSendCommand(MsgId::CommandCommon, CommonType::DismissGuildReject, 0, 0, 0, 0, cName20.c_str());
			break;
		}
		m_pGame->_ShiftGuildOperationList();
		if (m_pGame->m_stGuildOpList[0].cOpMode == 0)
			m_pGame->m_dialogBoxManager.DisableDialogBox(DialogBoxId::GuildOperation);
		return true;
	}

	return false;
}

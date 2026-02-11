#include "DialogBox_NpcTalk.h"
#include "Game.h"
#include "GameFonts.h"
#include "TextLibExt.h"


using namespace hb::shared::net;
DialogBox_NpcTalk::DialogBox_NpcTalk(CGame* pGame)
	: IDialogBox(DialogBoxId::NpcTalk, pGame)
{
	SetDefaultRect(337 , 57 , 258, 339);
}

int DialogBox_NpcTalk::GetTotalLines() const
{
	int iTotalLines = 0;
	for (int i = 0; i < game_limits::max_text_dlg_lines; i++)
	{
		if (m_pGame->m_pMsgTextList2[i] != nullptr)
			iTotalLines++;
	}
	return iTotalLines;
}

void DialogBox_NpcTalk::OnDraw(short msX, short msY, short msZ, char cLB)
{
	short sX = m_pGame->m_dialogBoxManager.Info(DialogBoxId::NpcTalk).sX;
	short sY = m_pGame->m_dialogBoxManager.Info(DialogBoxId::NpcTalk).sY;

	m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME2, sX, sY, 2);

	DrawButtons(sX, sY, msX, msY);
	DrawTextContent(sX, sY);

	int iTotalLines = GetTotalLines();
	DrawScrollBar(sX, sY, iTotalLines);
	HandleScrollBarDrag(sX, sY, msX, msY, iTotalLines, cLB);
}

void DialogBox_NpcTalk::DrawButtons(short sX, short sY, short msX, short msY)
{
	switch (m_pGame->m_dialogBoxManager.Info(DialogBoxId::NpcTalk).cMode)
	{
	case 0: // OK button only
		if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) &&
			(msY > sY + ui_layout::btn_y) && (msY < sY + ui_layout::btn_y + ui_layout::btn_size_y))
			m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 1);
		else
			m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 0);
		break;

	case 1: // Accept / Decline buttons
		if ((msX >= sX + ui_layout::left_btn_x) && (msX <= sX + ui_layout::left_btn_x + ui_layout::btn_size_x) &&
			(msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
			m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 33);
		else
			m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + ui_layout::left_btn_x, sY + ui_layout::btn_y, 32);

		if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) &&
			(msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
			m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 41);
		else
			m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 40);
		break;

	case 2: // Next button
		if ((msX >= sX + 190) && (msX <= sX + 278) &&
			(msY >= sY + 296) && (msY <= sY + 316))
			hb::shared::text::DrawText(GameFont::Bitmap1, sX + 190, sY + 270, "Next", hb::shared::text::TextStyle::WithHighlight(GameColors::UIMagicBlue));
		else
			hb::shared::text::DrawText(GameFont::Bitmap1, sX + 190, sY + 270, "Next", hb::shared::text::TextStyle::WithHighlight(GameColors::BmpBtnNormal));
		break;
	}
}

void DialogBox_NpcTalk::DrawTextContent(short sX, short sY)
{
	short sView = m_pGame->m_dialogBoxManager.Info(DialogBoxId::NpcTalk).sView;
	short sSizeX = m_pGame->m_dialogBoxManager.Info(DialogBoxId::NpcTalk).sSizeX;

	for (int i = 0; i < 17; i++)
	{
		if ((i < game_limits::max_text_dlg_lines) && (m_pGame->m_pMsgTextList2[i + sView] != nullptr))
		{
			hb::shared::text::DrawTextAligned(GameFont::Default, sX, sY + 57 + i * 15, sX + sSizeX - sX, 15,
				m_pGame->m_pMsgTextList2[i + sView]->m_pMsg, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);
		}
	}
}

void DialogBox_NpcTalk::DrawScrollBar(short sX, short sY, int iTotalLines)
{
	if (iTotalLines > 17)
	{
		double d1 = (double)m_pGame->m_dialogBoxManager.Info(DialogBoxId::NpcTalk).sView;
		double d2 = (double)(iTotalLines - 17);
		double d3 = (274.0 * d1) / d2;
		int iPointerLoc = (int)d3;
		m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME2, sX, sY, 3);
	}
}

void DialogBox_NpcTalk::HandleScrollBarDrag(short sX, short sY, short msX, short msY, int iTotalLines, char cLB)
{
	if (cLB != 0 && iTotalLines > 17)
	{
		if (m_pGame->m_dialogBoxManager.iGetTopDialogBoxIndex() == DialogBoxId::NpcTalk)
		{
			if ((msX >= sX + 240) && (msX <= sX + 260) &&
				(msY >= sY + 40) && (msY <= sY + 320))
			{
				double d1 = (double)(msY - (sY + 40));
				double d2 = (double)(iTotalLines - 17);
				double d3 = (d1 * d2) / 274.0;
				int iPointerLoc = (int)d3;

				if (iPointerLoc > iTotalLines)
					iPointerLoc = iTotalLines;
				m_pGame->m_dialogBoxManager.Info(DialogBoxId::NpcTalk).sView = iPointerLoc;
			}
		}
	}
	else
	{
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::NpcTalk).bIsScrollSelected = false;
	}
}

bool DialogBox_NpcTalk::OnClick(short msX, short msY)
{
	short sX = m_pGame->m_dialogBoxManager.Info(DialogBoxId::NpcTalk).sX;
	short sY = m_pGame->m_dialogBoxManager.Info(DialogBoxId::NpcTalk).sY;

	switch (m_pGame->m_dialogBoxManager.Info(DialogBoxId::NpcTalk).cMode)
	{
	case 0: // OK button
		if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) &&
			(msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
		{
			m_pGame->m_dialogBoxManager.DisableDialogBox(DialogBoxId::NpcTalk);
			m_pGame->PlayGameSound('E', 14, 5);
			return true;
		}
		break;

	case 1: // Accept / Decline buttons
		if ((msX >= sX + ui_layout::left_btn_x) && (msX <= sX + ui_layout::left_btn_x + ui_layout::btn_size_x) &&
			(msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
		{
			// Accept
			m_pGame->bSendCommand(MsgId::CommandCommon, CommonType::QuestAccepted, 0, 0, 0, 0, 0);
			m_pGame->m_dialogBoxManager.DisableDialogBox(DialogBoxId::NpcTalk);
			m_pGame->PlayGameSound('E', 14, 5);
			return true;
		}
		if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) &&
			(msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
		{
			// Decline
			m_pGame->m_dialogBoxManager.DisableDialogBox(DialogBoxId::NpcTalk);
			m_pGame->PlayGameSound('E', 14, 5);
			return true;
		}
		break;

	case 2: // Next button
		if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) &&
			(msY >= sY + ui_layout::btn_y) && (msY <= sY + ui_layout::btn_y + ui_layout::btn_size_y))
		{
			m_pGame->m_dialogBoxManager.DisableDialogBox(DialogBoxId::NpcTalk);
			m_pGame->PlayGameSound('E', 14, 5);
			return true;
		}
		break;
	}

	return false;
}

PressResult DialogBox_NpcTalk::OnPress(short msX, short msY)
{
	short sX = Info().sX;
	short sY = Info().sY;

	// Scroll bar region
	if ((msX >= sX + 240) && (msX <= sX + 260) && (msY >= sY + 40) && (msY <= sY + 320))
	{
		return PressResult::ScrollClaimed;
	}

	return PressResult::Normal;
}

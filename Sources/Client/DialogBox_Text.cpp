#include "DialogBox_Text.h"
#include "ConfigManager.h"
#include "Game.h"
#include "IInput.h"
#include "GameFonts.h"
#include "TextLibExt.h"
using namespace hb::client::sprite_id;
// game_limits::max_text_dlg_lines is in GameConstants.h (via Game.h)

DialogBox_Text::DialogBox_Text(CGame* pGame)
	: IDialogBox(DialogBoxId::Text, pGame)
{
	SetDefaultRect(20 , 65 , 258, 339);
}

int DialogBox_Text::GetTotalLines() const
{
	int iTotalLines = 0;
	for (int i = 0; i < game_limits::max_text_dlg_lines; i++)
	{
		if (m_pGame->m_pMsgTextList[i] != nullptr)
			iTotalLines++;
	}
	return iTotalLines;
}

void DialogBox_Text::OnDraw(short msX, short msY, short msZ, char cLB)
{
	short sX = m_pGame->m_dialogBoxManager.Info(DialogBoxId::Text).sX;
	short sY = m_pGame->m_dialogBoxManager.Info(DialogBoxId::Text).sY;

	m_pGame->DrawNewDialogBox(InterfaceNdGame2, sX, sY, 0);

	int iTotalLines = GetTotalLines();

	if (iTotalLines > 17)
		m_pGame->DrawNewDialogBox(InterfaceNdGame2, sX, sY, 1);

	// Mouse wheel scrolling
	if (m_pGame->m_dialogBoxManager.iGetTopDialogBoxIndex() == DialogBoxId::Text && msZ != 0)
	{
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::Text).sView -= msZ / 60;

	}

	// Clamp scroll view
	if (m_pGame->m_dialogBoxManager.Info(DialogBoxId::Text).sView < 0)
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::Text).sView = 0;
	if (iTotalLines > 17 && m_pGame->m_dialogBoxManager.Info(DialogBoxId::Text).sView > iTotalLines - 17)
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::Text).sView = iTotalLines - 17;

	// Draw scroll bar
	int iPointerLoc = 0;
	if (iTotalLines > 17)
	{
		double d1 = (double)m_pGame->m_dialogBoxManager.Info(DialogBoxId::Text).sView;
		double d2 = (double)(iTotalLines - 17);
		double d3 = (274.0 * d1) / d2;
		iPointerLoc = (int)(d3 + 0.5);
		m_pGame->DrawNewDialogBox(InterfaceNdGame2, sX, sY, 1);
		m_pGame->DrawNewDialogBox(InterfaceNdGame2, sX + 242, sY + 35 + iPointerLoc, 7);
	}

	// Draw text lines
	short sView = m_pGame->m_dialogBoxManager.Info(DialogBoxId::Text).sView;
	for (int i = 0; i < 17; i++)
	{
		if (m_pGame->m_pMsgTextList[i + sView] != nullptr)
		{
			char* pMsg = m_pGame->m_pMsgTextList[i + sView]->m_pMsg;
			if (ConfigManager::Get().IsDialogTransparencyEnabled() == false)
			{
				switch (pMsg[0])
				{
				case '_':
					hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 50 + i * 13, sX + 236 - (sX + 24), 15, pMsg + 1, hb::shared::text::TextStyle::Color(GameColors::UIWhite), hb::shared::text::Align::TopCenter);
					break;
				case ';':
					hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 50 + i * 13, sX + 236 - (sX + 24), 15, pMsg + 1, hb::shared::text::TextStyle::Color(GameColors::UIMagicBlue), hb::shared::text::Align::TopCenter);
					break;
				default:
					hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 50 + i * 13, sX + 236 - (sX + 24), 15, pMsg, hb::shared::text::TextStyle::Color(GameColors::UILabel), hb::shared::text::Align::TopCenter);
					break;
				}
			}
			else
			{
				hb::shared::text::DrawTextAligned(GameFont::Default, sX + 24, sY + 50 + i * 13, sX + 236 - (sX + 24), 15, pMsg, hb::shared::text::TextStyle::Color(GameColors::UIBlack), hb::shared::text::Align::TopCenter);
			}
		}
	}

	// Handle scroll bar dragging
	if (cLB != 0 && iTotalLines > 17)
	{
		if (m_pGame->m_dialogBoxManager.iGetTopDialogBoxIndex() == DialogBoxId::Text)
		{
			if ((msX >= sX + 240) && (msX <= sX + 260) && (msY >= sY + 40) && (msY <= sY + 320))
			{
				double d1 = (double)(msY - (sY + 35));
				double d2 = (double)(iTotalLines - 17);
				double d3 = (d1 * d2) / 274.0;
				iPointerLoc = (int)d3;
				if (iPointerLoc > iTotalLines - 17)
					iPointerLoc = iTotalLines - 17;
				m_pGame->m_dialogBoxManager.Info(DialogBoxId::Text).sView = iPointerLoc;
			}
		}
	}
	else
	{
		m_pGame->m_dialogBoxManager.Info(DialogBoxId::Text).bIsScrollSelected = false;
	}

	// Close button hover highlight
	if ((msX > sX + ui_layout::right_btn_x) && (msX < sX + ui_layout::right_btn_x + ui_layout::btn_size_x) &&
		(msY > sY + ui_layout::btn_y) && (msY < sY + ui_layout::btn_y + ui_layout::btn_size_y))
	{
		m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 1);
	}
	else
	{
		m_pGame->DrawNewDialogBox(InterfaceNdButton, sX + ui_layout::right_btn_x, sY + ui_layout::btn_y, 0);
	}
}

bool DialogBox_Text::OnClick(short msX, short msY)
{
	short sX = m_pGame->m_dialogBoxManager.Info(DialogBoxId::Text).sX;
	short sY = m_pGame->m_dialogBoxManager.Info(DialogBoxId::Text).sY;

	// Close button
	if ((msX >= sX + ui_layout::right_btn_x) && (msX <= sX + ui_layout::right_btn_x + ui_layout::btn_size_x) &&
		(msY > sY + ui_layout::btn_y) && (msY < sY + ui_layout::btn_y + ui_layout::btn_size_y))
	{
		m_pGame->m_dialogBoxManager.DisableDialogBox(DialogBoxId::Text);
		m_pGame->PlayGameSound('E', 14, 5);
		return true;
	}

	return false;
}

PressResult DialogBox_Text::OnPress(short msX, short msY)
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


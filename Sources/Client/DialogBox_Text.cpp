#include "DialogBox_Text.h"
#include "ConfigManager.h"
#include "Game.h"
#include "IInput.h"
#include "GameFonts.h"
#include "TextLibExt.h"
// DEF_TEXTDLGMAXLINES is in GameConstants.h (via Game.h)

DialogBox_Text::DialogBox_Text(CGame* pGame)
	: IDialogBox(DialogBoxId::Text, pGame)
{
	SetDefaultRect(20 , 65 , 258, 339);
}

int DialogBox_Text::GetTotalLines() const
{
	int iTotalLines = 0;
	for (int i = 0; i < DEF_TEXTDLGMAXLINES; i++)
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

	m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME2, sX, sY, 0);

	int iTotalLines = GetTotalLines();

	if (iTotalLines > 17)
		m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME2, sX, sY, 1);

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
		m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME2, sX, sY, 1);
		m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME2, sX + 242, sY + 35 + iPointerLoc, 7);
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
					TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 50 + i * 13, sX + 236 - (sX + 24), 15, pMsg + 1, TextLib::TextStyle::Color(GameColors::UIWhite), TextLib::Align::TopCenter);
					break;
				case ';':
					TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 50 + i * 13, sX + 236 - (sX + 24), 15, pMsg + 1, TextLib::TextStyle::Color(GameColors::UIMagicBlue), TextLib::Align::TopCenter);
					break;
				default:
					TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 50 + i * 13, sX + 236 - (sX + 24), 15, pMsg, TextLib::TextStyle::Color(GameColors::UILabel), TextLib::Align::TopCenter);
					break;
				}
			}
			else
			{
				TextLib::DrawTextAligned(GameFont::Default, sX + 24, sY + 50 + i * 13, sX + 236 - (sX + 24), 15, pMsg, TextLib::TextStyle::Color(GameColors::UIBlack), TextLib::Align::TopCenter);
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
	if ((msX > sX + DEF_RBTNPOSX) && (msX < sX + DEF_RBTNPOSX + DEF_BTNSZX) &&
		(msY > sY + DEF_BTNPOSY) && (msY < sY + DEF_BTNPOSY + DEF_BTNSZY))
	{
		m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + DEF_RBTNPOSX, sY + DEF_BTNPOSY, 1);
	}
	else
	{
		m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_BUTTON, sX + DEF_RBTNPOSX, sY + DEF_BTNPOSY, 0);
	}
}

bool DialogBox_Text::OnClick(short msX, short msY)
{
	short sX = m_pGame->m_dialogBoxManager.Info(DialogBoxId::Text).sX;
	short sY = m_pGame->m_dialogBoxManager.Info(DialogBoxId::Text).sY;

	// Close button
	if ((msX >= sX + DEF_RBTNPOSX) && (msX <= sX + DEF_RBTNPOSX + DEF_BTNSZX) &&
		(msY > sY + DEF_BTNPOSY) && (msY < sY + DEF_BTNPOSY + DEF_BTNSZY))
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


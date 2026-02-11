#include "DialogBox_ChatHistory.h"
#include "ConfigManager.h"
#include "Game.h"
#include "IInput.h"
#include "GameFonts.h"
#include "TextLibExt.h"

#define DEF_CHAT_VISIBLE_LINES 8
#define DEF_CHAT_SCROLLBAR_HEIGHT 105

DialogBox_ChatHistory::DialogBox_ChatHistory(CGame* pGame)
	: IDialogBox(DialogBoxId::ChatHistory, pGame)
{
	SetDefaultRect(135 , 273  , 364, 162);
}

void DialogBox_ChatHistory::OnDraw(short msX, short msY, short msZ, char cLB)
{
	short sX = m_pGame->m_dialogBoxManager.Info(DialogBoxId::ChatHistory).sX;
	short sY = m_pGame->m_dialogBoxManager.Info(DialogBoxId::ChatHistory).sY;

	const bool dialogTrans = ConfigManager::Get().IsDialogTransparencyEnabled();
	m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME2, sX, sY, 4, false, dialogTrans);
	m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_TEXT, sX, sY, 22, false, dialogTrans);

	HandleScrollInput(sX, sY, msX, msY, msZ, cLB);
	DrawScrollBar(sX, sY);
	DrawChatMessages(sX, sY);
}

void DialogBox_ChatHistory::HandleScrollInput(short sX, short sY, short msX, short msY, short msZ, char cLB)
{
	auto& info = m_pGame->m_dialogBoxManager.Info(DialogBoxId::ChatHistory);

	// Mouse wheel scrolling
	if (m_pGame->m_dialogBoxManager.iGetTopDialogBoxIndex() == DialogBoxId::ChatHistory)
	{
		short sWheelDelta = hb::shared::input::GetMouseWheelDelta();
		if (sWheelDelta != 0)
		{
			info.sView += sWheelDelta / 30;
		}
	}

	// Scroll bar dragging
	if ((cLB != 0) && (m_pGame->m_dialogBoxManager.iGetTopDialogBoxIndex() == DialogBoxId::ChatHistory))
	{
		// Drag scrollbar track
		if ((msX >= sX + 336) && (msX <= sX + 361) && (msY >= sY + 28) && (msY <= sY + 140))
		{
			double d1 = (double)(msY - (sY + 28));
			double d2 = ((game_limits::max_chat_scroll_msgs - DEF_CHAT_VISIBLE_LINES) * d1) / (double)DEF_CHAT_SCROLLBAR_HEIGHT;
			info.sView = game_limits::max_chat_scroll_msgs - DEF_CHAT_VISIBLE_LINES - (int)d2;
		}

		// Scroll to top button
		if ((msX >= sX + 336) && (msX <= sX + 361) && (msY > sY + 18) && (msY < sY + 28))
			info.sView = game_limits::max_chat_scroll_msgs - DEF_CHAT_VISIBLE_LINES;

		// Scroll to bottom button
		if ((msX >= sX + 336) && (msX <= sX + 361) && (msY > sY + 140) && (msY < sY + 163))
			info.sView = 0;
	}
	else
	{
		info.bIsScrollSelected = false;
	}

	// Clamp scroll view (must be after all scroll modifications)
	if (info.sView < 0)
		info.sView = 0;
	if (info.sView > game_limits::max_chat_scroll_msgs - DEF_CHAT_VISIBLE_LINES)
		info.sView = game_limits::max_chat_scroll_msgs - DEF_CHAT_VISIBLE_LINES;
}

void DialogBox_ChatHistory::DrawScrollBar(short sX, short sY)
{
	double d1 = (double)m_pGame->m_dialogBoxManager.Info(DialogBoxId::ChatHistory).sView;
	double d2 = (double)DEF_CHAT_SCROLLBAR_HEIGHT;
	double d3 = (d1 * d2) / (game_limits::max_chat_scroll_msgs - DEF_CHAT_VISIBLE_LINES);
	int iPointerLoc = (int)d3;
	iPointerLoc = DEF_CHAT_SCROLLBAR_HEIGHT - iPointerLoc;
	m_pGame->DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_GAME2, sX + 346, sY + 33 + iPointerLoc, 7);
}

void DialogBox_ChatHistory::DrawChatMessages(short sX, short sY)
{
	short sView = m_pGame->m_dialogBoxManager.Info(DialogBoxId::ChatHistory).sView;

	for (int i = 0; i < DEF_CHAT_VISIBLE_LINES; i++)
	{
		int iIndex = i + sView;
		if (iIndex < 0 || iIndex >= game_limits::max_chat_scroll_msgs) continue;
		if (m_pGame->m_pChatScrollList[iIndex] != nullptr)
		{
			int iYPos = sY + 127 - i * 13;
			char* pMsg = m_pGame->m_pChatScrollList[iIndex]->m_pMsg;

			switch (m_pGame->m_pChatScrollList[iIndex]->m_dwTime)
			{
			case 0:  hb::shared::text::DrawText(GameFont::Default, sX + 25, iYPos, pMsg, hb::shared::text::TextStyle::WithShadow(GameColors::UINearWhite)); break; // Normal
			case 1:  hb::shared::text::DrawText(GameFont::Default, sX + 25, iYPos, pMsg, hb::shared::text::TextStyle::WithShadow(GameColors::UIGuildGreen)); break; // Green
			case 2:  hb::shared::text::DrawText(GameFont::Default, sX + 25, iYPos, pMsg, hb::shared::text::TextStyle::WithShadow(GameColors::UIWorldChat)); break; // Red
			case 3:  hb::shared::text::DrawText(GameFont::Default, sX + 25, iYPos, pMsg, hb::shared::text::TextStyle::WithShadow(GameColors::UIFactionChat)); break; // Blue
			case 4:  hb::shared::text::DrawText(GameFont::Default, sX + 25, iYPos, pMsg, hb::shared::text::TextStyle::WithShadow(GameColors::UIPartyChat)); break; // Yellow
			case 10: hb::shared::text::DrawText(GameFont::Default, sX + 25, iYPos, pMsg, hb::shared::text::TextStyle::WithShadow(GameColors::UIGameMasterChat)); break; // Light green
			case 20: hb::shared::text::DrawText(GameFont::Default, sX + 25, iYPos, pMsg, hb::shared::text::TextStyle::WithShadow(GameColors::UINormalChat)); break; // Gray
			}
		}
	}
}

bool DialogBox_ChatHistory::OnClick(short msX, short msY)
{
	// Chat history dialog has no click actions - scrolling is handled in OnDraw
	return false;
}

PressResult DialogBox_ChatHistory::OnPress(short msX, short msY)
{
	short sX = Info().sX;
	short sY = Info().sY;

	// Check if click is in scroll bar region (track + buttons)
	if ((msX >= sX + 336) && (msX <= sX + 361) && (msY >= sY + 18) && (msY <= sY + 163))
	{
		return PressResult::ScrollClaimed;
	}

	return PressResult::Normal;
}


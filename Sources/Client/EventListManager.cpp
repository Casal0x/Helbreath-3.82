#include "EventListManager.h"
#include "Game.h"
#include "GameFonts.h"
#include "TextLibExt.h"
#include "GameTimer.h"
#include "lan_eng.h"

EventListManager& EventListManager::Get()
{
	static EventListManager instance;
	return instance;
}

void EventListManager::SetGame(CGame* pGame)
{
	m_game = pGame;
}

void EventListManager::AddEvent(const char* pTxt, char cColor, bool bDupAllow)
{
	int i;
	if ((bDupAllow == false) && (strcmp(m_events[5].cTxt, pTxt) == 0)) return;
	if (cColor == 10)
	{
		for (i = 1; i < 6; i++)
		{
			std::snprintf(m_events2[i - 1].cTxt, sizeof(m_events2[i - 1].cTxt), "%s", m_events2[i].cTxt);
			m_events2[i - 1].cColor = m_events2[i].cColor;
			m_events2[i - 1].dwTime = m_events2[i].dwTime;
		}
		std::memset(m_events2[5].cTxt, 0, sizeof(m_events2[5].cTxt));
		std::snprintf(m_events2[5].cTxt, sizeof(m_events2[5].cTxt), "%s", pTxt);
		m_events2[5].cColor = cColor;
		m_events2[5].dwTime = GameClock::GetTimeMS();
	}
	else
	{
		for (i = 1; i < 6; i++)
		{
			std::snprintf(m_events[i - 1].cTxt, sizeof(m_events[i - 1].cTxt), "%s", m_events[i].cTxt);
			m_events[i - 1].cColor = m_events[i].cColor;
			m_events[i - 1].dwTime = m_events[i].dwTime;
		}
		std::memset(m_events[5].cTxt, 0, sizeof(m_events[5].cTxt));
		std::snprintf(m_events[5].cTxt, sizeof(m_events[5].cTxt), "%s", pTxt);
		m_events[5].cColor = cColor;
		m_events[5].dwTime = GameClock::GetTimeMS();
	}
}

void EventListManager::AddEventTop(const char* pTxt, char cColor)
{
	for (int i = 1; i < 6; i++)
	{
		std::snprintf(m_events[i - 1].cTxt, sizeof(m_events[i - 1].cTxt), "%s", m_events[i].cTxt);
		m_events[i - 1].cColor = m_events[i].cColor;
		m_events[i - 1].dwTime = m_events[i].dwTime;
	}
	std::memset(m_events[5].cTxt, 0, sizeof(m_events[5].cTxt));
	std::snprintf(m_events[5].cTxt, sizeof(m_events[5].cTxt), "%s", pTxt);
	m_events[5].cColor = cColor;
	m_events[5].dwTime = GameClock::GetTimeMS();
}

void EventListManager::ShowEvents(uint32_t dwTime)
{
	int i;
	int baseY = EVENTLIST2_BASE_Y();
	m_game->m_Renderer->BeginTextBatch();
	for (i = 0; i < 6; i++)
		if ((dwTime - m_events[i].dwTime) < 5000)
		{
			switch (m_events[i].cColor) {
			case 0:
				hb::shared::text::DrawText(GameFont::Default, 10, 10 + i * 15, m_events[i].cTxt, hb::shared::text::TextStyle::WithShadow(GameColors::UINearWhite));
				break;
			case 1:
				hb::shared::text::DrawText(GameFont::Default, 10, 10 + i * 15, m_events[i].cTxt, hb::shared::text::TextStyle::WithShadow(GameColors::ChatEventGreen));
				break;
			case 2:
				hb::shared::text::DrawText(GameFont::Default, 10, 10 + i * 15, m_events[i].cTxt, hb::shared::text::TextStyle::WithShadow(GameColors::UIWorldChat));
				break;
			case 3:
				hb::shared::text::DrawText(GameFont::Default, 10, 10 + i * 15, m_events[i].cTxt, hb::shared::text::TextStyle::WithShadow(GameColors::UIFactionChat));
				break;
			case 4:
				hb::shared::text::DrawText(GameFont::Default, 10, 10 + i * 15, m_events[i].cTxt, hb::shared::text::TextStyle::WithShadow(GameColors::UIPartyChat));
				break;
			case hb::shared::owner::Slime:
				hb::shared::text::DrawText(GameFont::Default, 10, 10 + i * 15, m_events[i].cTxt, hb::shared::text::TextStyle::WithShadow(GameColors::UIGameMasterChat));
				break;
			case hb::shared::owner::Howard:
				hb::shared::text::DrawText(GameFont::Default, 10, 10 + i * 15, m_events[i].cTxt, hb::shared::text::TextStyle::WithShadow(GameColors::UINormalChat));
				break;
			}
		}

	for (i = 0; i < 6; i++)
		if ((dwTime - m_events2[i].dwTime) < 5000)
		{
			switch (m_events2[i].cColor) {
			case 0:
				hb::shared::text::DrawText(GameFont::Default, 10, baseY + i * 15, m_events2[i].cTxt, hb::shared::text::TextStyle::WithShadow(GameColors::UINearWhite));
				break;
			case 1:
				hb::shared::text::DrawText(GameFont::Default, 10, baseY + i * 15, m_events2[i].cTxt, hb::shared::text::TextStyle::WithShadow(GameColors::ChatEventGreen));
				break;
			case 2:
				hb::shared::text::DrawText(GameFont::Default, 10, baseY + i * 15, m_events2[i].cTxt, hb::shared::text::TextStyle::WithShadow(GameColors::UIWorldChat));
				break;
			case 3:
				hb::shared::text::DrawText(GameFont::Default, 10, baseY + i * 15, m_events2[i].cTxt, hb::shared::text::TextStyle::WithShadow(GameColors::UIFactionChat));
				break;
			case 4:
				hb::shared::text::DrawText(GameFont::Default, 10, baseY + i * 15, m_events2[i].cTxt, hb::shared::text::TextStyle::WithShadow(GameColors::UIPartyChat));
				break;
			case hb::shared::owner::Slime:
				hb::shared::text::DrawText(GameFont::Default, 10, baseY + i * 15, m_events2[i].cTxt, hb::shared::text::TextStyle::WithShadow(GameColors::UIGameMasterChat));
				break;
			case hb::shared::owner::Howard:
				hb::shared::text::DrawText(GameFont::Default, 10, baseY + i * 15, m_events2[i].cTxt, hb::shared::text::TextStyle::WithShadow(GameColors::UINormalChat));
				break;
			}
		}
	if (m_game->m_bSkillUsingStatus == true)
	{
		hb::shared::text::DrawText(GameFont::Default, 440 - 29, 440 - 52, SHOW_EVENT_LIST1, hb::shared::text::TextStyle::WithShadow(GameColors::UINearWhite));
	}
	m_game->m_Renderer->EndTextBatch();
}

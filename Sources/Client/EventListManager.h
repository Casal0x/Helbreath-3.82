#pragma once

#include <cstdint>
#include <cstring>
#include <cstdio>

class CGame;

namespace hb::shared::render { class IRenderer; }

struct EventEntry
{
	uint32_t dwTime = 0;
	char cColor = 0;
	char cTxt[96]{};
};

class EventListManager
{
public:
	static EventListManager& Get();
	void SetGame(CGame* pGame);

	void AddEvent(const char* pTxt, char cColor = 0, bool bDupAllow = true);
	void AddEventTop(const char* pTxt, char cColor);
	void ShowEvents(uint32_t dwTime);

private:
	EventListManager() = default;
	~EventListManager() = default;

	CGame* m_game = nullptr;
	EventEntry m_events[6]{};
	EventEntry m_events2[6]{};
};

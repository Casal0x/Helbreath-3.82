// DelayEventManager.h: Manages timed delay events (magic expiration, meteors, etc.).
// Extracted from Game.cpp (Phase B5).

#pragma once

#include <cstdint>

class CGame;
class CDelayEvent;

class DelayEventManager
{
public:
	DelayEventManager() = default;
	~DelayEventManager() = default;

	void SetGame(CGame* pGame) { m_pGame = pGame; }

	void InitArrays();
	void CleanupArrays();

	// Core delay event management
	bool bRegisterDelayEvent(int iDelayType, int iEffectType, uint32_t dwLastTime, int iTargetH, char cTargetType, char cMapIndex, int dX, int dY, int iV1, int iV2, int iV3);
	bool bRemoveFromDelayEventList(int iH, char cType, int iEffectType);
	void DelayEventProcessor();
	void DelayEventProcess();

	// Public array for backward compatibility
	CDelayEvent* m_pDelayEventList[60000];

private:
	CGame* m_pGame = nullptr;
};

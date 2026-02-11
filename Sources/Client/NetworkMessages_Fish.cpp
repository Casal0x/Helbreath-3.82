// NetworkMessages_Fish.cpp: Fish network message handlers.
// Handlers now live in FishingManager; this file provides the
// NetworkMessageHandlers namespace wrappers for backward compatibility.

#include "Game.h"

namespace NetworkMessageHandlers {

void HandleFishChance(CGame* pGame, char* pData)
{
	pGame->m_fishingManager.HandleFishChance(pData);
}

void HandleEventFishMode(CGame* pGame, char* pData)
{
	pGame->m_fishingManager.HandleEventFishMode(pData);
}

void HandleFishCanceled(CGame* pGame, char* pData)
{
	pGame->m_fishingManager.HandleFishCanceled(pData);
}

void HandleFishSuccess(CGame* pGame, char* pData)
{
	pGame->m_fishingManager.HandleFishSuccess(pData);
}

void HandleFishFail(CGame* pGame, char* pData)
{
	pGame->m_fishingManager.HandleFishFail(pData);
}

} // namespace NetworkMessageHandlers

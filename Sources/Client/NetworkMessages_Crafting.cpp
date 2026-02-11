#include "Game.h"
#include "CraftingManager.h"

namespace NetworkMessageHandlers {

void HandleCraftingSuccess(CGame* pGame, char* pData)
{
	pGame->m_craftingManager.HandleCraftingSuccess(pData);
}

void HandleCraftingFail(CGame* pGame, char* pData)
{
	pGame->m_craftingManager.HandleCraftingFail(pData);
}

void HandleBuildItemSuccess(CGame* pGame, char* pData)
{
	pGame->m_craftingManager.HandleBuildItemSuccess(pData);
}

void HandleBuildItemFail(CGame* pGame, char* pData)
{
	pGame->m_craftingManager.HandleBuildItemFail(pData);
}

void HandlePortionSuccess(CGame* pGame, char* pData)
{
	pGame->m_craftingManager.HandlePortionSuccess(pData);
}

void HandlePortionFail(CGame* pGame, char* pData)
{
	pGame->m_craftingManager.HandlePortionFail(pData);
}

void HandleLowPortionSkill(CGame* pGame, char* pData)
{
	pGame->m_craftingManager.HandleLowPortionSkill(pData);
}

void HandleNoMatchingPortion(CGame* pGame, char* pData)
{
	pGame->m_craftingManager.HandleNoMatchingPortion(pData);
}

} // namespace NetworkMessageHandlers

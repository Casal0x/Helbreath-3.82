#include "Game.h"
#include "QuestManager.h"

namespace NetworkMessageHandlers {

void HandleQuestCounter(CGame* pGame, char* pData)
{
	pGame->m_questManager.HandleQuestCounter(pData);
}

void HandleQuestContents(CGame* pGame, char* pData)
{
	pGame->m_questManager.HandleQuestContents(pData);
}

void HandleQuestReward(CGame* pGame, char* pData)
{
	pGame->m_questManager.HandleQuestReward(pData);
}

void HandleQuestCompleted(CGame* pGame, char* pData)
{
	pGame->m_questManager.HandleQuestCompleted(pData);
}

void HandleQuestAborted(CGame* pGame, char* pData)
{
	pGame->m_questManager.HandleQuestAborted(pData);
}

} // namespace NetworkMessageHandlers

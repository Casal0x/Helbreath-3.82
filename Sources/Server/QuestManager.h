// QuestManager.h: Manages quest assignment, progress tracking, and completion.
// Extracted from CGame (Phase B3).

#pragma once

#include "CommonTypes.h"
#include "Quest.h"
#include "Game.h"  // For hb::server::config constants

class QuestManager
{
public:
	QuestManager();
	~QuestManager();

	void SetGame(CGame* pGame) { m_pGame = pGame; }

	// Lifecycle
	void InitArrays();
	void CleanupArrays();

	// Quest handlers (moved from CGame)
	void NpcTalkHandler(int iClientH, int iWho);
	void QuestAcceptedHandler(int iClientH);
	void CancelQuestHandler(int iClientH);
	void _SendQuestContents(int iClientH);
	void _CheckQuestEnvironment(int iClientH);
	bool _bCheckIsQuestCompleted(int iClientH);
	void _ClearQuestStatus(int iClientH);

	// Config array (public for database loading)
	CQuest* m_pQuestConfigList[hb::server::config::MaxQuestType]{};

private:
	CGame* m_pGame = nullptr;

	// Private quest search helpers
	int _iTalkToNpcResult_Cityhall(int iClientH, int* pQuestType, int* pMode, int* pRewardType, int* pRewardAmount, int* pContribution, char* pTargetName, int* pTargetType, int* pTargetCount, int* pX, int* pY, int* pRange);
	int _iTalkToNpcResult_Guard(int iClientH, int* pQuestType, int* pMode, int* pRewardType, int* pRewardAmount, int* pContribution, char* pTargetName, int* pTargetType, int* pTargetCount, int* pX, int* pY, int* pRange);
	int __iSearchForQuest(int iClientH, int iWho, int* pQuestType, int* pMode, int* pRewardType, int* pRewardAmount, int* pContribution, char* pTargetName, int* pTargetType, int* pTargetCount, int* pX, int* pY, int* pRange);
};

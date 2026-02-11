// QuestManager.h: Handles client-side quest network messages.
// Extracted from NetworkMessages_Quest.cpp (Phase B3).

#pragma once

class CGame;

class QuestManager
{
public:
	QuestManager() = default;
	~QuestManager() = default;

	void SetGame(CGame* pGame) { m_pGame = pGame; }

	// Network message handlers
	void HandleQuestCounter(char* pData);
	void HandleQuestContents(char* pData);
	void HandleQuestReward(char* pData);
	void HandleQuestCompleted(char* pData);
	void HandleQuestAborted(char* pData);

private:
	CGame* m_pGame = nullptr;
};

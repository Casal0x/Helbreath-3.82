// CraftingManager.h: Handles client-side crafting/portion network messages.
// Extracted from NetworkMessages_Crafting.cpp (Phase B2).

#pragma once

class CGame;

class CraftingManager
{
public:
	CraftingManager() = default;
	~CraftingManager() = default;

	void SetGame(CGame* pGame) { m_pGame = pGame; }

	// Network message handlers
	void HandleCraftingSuccess(char* pData);
	void HandleCraftingFail(char* pData);
	void HandleBuildItemSuccess(char* pData);
	void HandleBuildItemFail(char* pData);
	void HandlePortionSuccess(char* pData);
	void HandlePortionFail(char* pData);
	void HandleLowPortionSkill(char* pData);
	void HandleNoMatchingPortion(char* pData);

private:
	CGame* m_pGame = nullptr;
};

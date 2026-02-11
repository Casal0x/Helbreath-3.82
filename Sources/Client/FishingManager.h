// FishingManager.h: Handles client-side fishing network messages.
// Extracted from NetworkMessages_Fish.cpp (Phase B1).

#pragma once

class CGame;

class FishingManager
{
public:
	FishingManager() = default;
	~FishingManager() = default;

	void SetGame(CGame* pGame) { m_pGame = pGame; }

	// Network message handlers (moved from NetworkMessageHandlers namespace)
	void HandleFishChance(char* pData);
	void HandleEventFishMode(char* pData);
	void HandleFishCanceled(char* pData);
	void HandleFishSuccess(char* pData);
	void HandleFishFail(char* pData);

private:
	CGame* m_pGame = nullptr;
};

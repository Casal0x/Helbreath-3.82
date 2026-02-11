// FishingManager.h: Manages fish spawning, catch processing, and fishing interactions.
// Extracted from CGame (Phase B1).

#pragma once

#include "CommonTypes.h"
#include "Fish.h"
#include "Game.h"  // For hb::server::config constants

class FishingManager
{
public:
	FishingManager();
	~FishingManager();

	void SetGame(CGame* pGame) { m_pGame = pGame; }

	// Lifecycle
	void InitArrays();
	void CleanupArrays();

	// Core fishing methods (moved from CGame)
	void FishGenerator();
	void FishProcessor();
	void ReqGetFishThisTimeHandler(int iClientH);
	int iCheckFish(int iClientH, char cMapIndex, short dX, short dY);
	int iCreateFish(char cMapIndex, short sX, short sY, short sType, CItem* pItem, int iDifficulty, uint32_t dwLastTime);
	bool bDeleteFish(int iHandle, int iDelMode);

	// Called by CGame when a client disconnects or stops using skills
	void ReleaseFishEngagement(int iClientH);

	// Timer
	uint32_t m_dwFishTime = 0;

private:
	CGame* m_pGame = nullptr;
	CFish* m_pFish[hb::server::config::MaxFishs]{};
};

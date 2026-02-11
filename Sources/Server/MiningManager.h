// MiningManager.h: Manages mineral spawning, mining actions, and mineral lifecycle.
// Extracted from CGame (Phase B1).

#pragma once

#include "CommonTypes.h"
#include "Mineral.h"
#include "Game.h"  // For hb::server::config constants

class MiningManager
{
public:
	MiningManager();
	~MiningManager();

	void SetGame(CGame* pGame) { m_pGame = pGame; }

	// Lifecycle
	void InitArrays();
	void CleanupArrays();

	// Core mining methods (moved from CGame)
	void MineralGenerator();
	int iCreateMineral(char cMapIndex, int tX, int tY, char cLevel);
	void _CheckMiningAction(int iClientH, int dX, int dY);
	bool bDeleteMineral(int iIndex);

private:
	CGame* m_pGame = nullptr;
	CMineral* m_pMineral[hb::server::config::MaxMinerals]{};
};

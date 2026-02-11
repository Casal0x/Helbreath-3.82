#pragma once

#include <cstdint>

class CGame;

class LootManager
{
public:
	LootManager() = default;
	~LootManager() = default;

	void SetGame(CGame* pGame) { m_pGame = pGame; }

	// Kill rewards
	void PK_KillRewardHandler(short sAttackerH, short sVictumH);
	void EnemyKillRewardHandler(int iAttackerH, int iClientH);
	void GetRewardMoneyHandler(int iClientH);

	// Kill penalties
	void ApplyPKpenalty(short sAttackerH, short sVictumH);
	void ApplyCombatKilledPenalty(int iClientH, int cPenaltyLevel, bool bIsSAattacked);
	void _PenaltyItemDrop(int iClientH, int iTotal, bool bIsSAattacked = false);

private:
	CGame* m_pGame = nullptr;
};

#pragma once
#include <cstdint>

class CGame;

class StatusEffectManager
{
public:
	StatusEffectManager() = default;
	~StatusEffectManager() = default;
	void SetGame(CGame* pGame) { m_pGame = pGame; }

	// Status effect flags
	void SetHeroFlag(short sOwnerH, char cOwnerType, bool bStatus);
	void SetBerserkFlag(short sOwnerH, char cOwnerType, bool bStatus);
	void SetHasteFlag(short sOwnerH, char cOwnerType, bool bStatus);
	void SetPoisonFlag(short sOwnerH, char cOwnerType, bool bStatus);
	void SetDefenseShieldFlag(short sOwnerH, char cOwnerType, bool bStatus);
	void SetMagicProtectionFlag(short sOwnerH, char cOwnerType, bool bStatus);
	void SetProtectionFromArrowFlag(short sOwnerH, char cOwnerType, bool bStatus);
	void SetIllusionMovementFlag(short sOwnerH, char cOwnerType, bool bStatus);
	void SetIllusionFlag(short sOwnerH, char cOwnerType, bool bStatus);
	void SetIceFlag(short sOwnerH, char cOwnerType, bool bStatus);
	void SetInvisibilityFlag(short sOwnerH, char cOwnerType, bool bStatus);
	void SetInhibitionCastingFlag(short sOwnerH, char cOwnerType, bool bStatus);
	void SetAngelFlag(short sOwnerH, char cOwnerType, int iStatus, int iTemp);

	// Farming exploit detection
	void _CheckFarmingAction(short sAttackerH, short sTargetH, bool bType);

private:
	CGame* m_pGame = nullptr;
};

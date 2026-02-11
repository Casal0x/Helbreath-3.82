#pragma once

#include <cstdint>
#include "EntityRelationship.h"

class CGame;

class CombatManager
{
public:
	CombatManager() = default;
	~CombatManager() = default;

	void SetGame(CGame* pGame) { m_pGame = pGame; }

	// Core attack calculation
	uint32_t iCalculateAttackEffect(short sTargetH, char cTargetType, short sAttackerH, char cAttackerType, int tdX, int tdY, int iAttackMode, bool bNearAttack = false, bool bIsDash = false, bool bArrowUse = false);
	bool bCalculateEnduranceDecrement(short sTargetH, short sAttackerH, char cTargetType, int iArmorType);

	// Damage application
	void Effect_Damage_Spot(short sAttackerH, char cAttackerType, short sTargetH, char cTargetType, short sV1, short sV2, short sV3, bool bExp, int iAttr = 0);
	void Effect_Damage_Spot_DamageMove(short sAttackerH, char cAttackerType, short sTargetH, char cTargetType, short sAtkX, short sAtkY, short sV1, short sV2, short sV3, bool bExp, int iAttr);
	void Effect_HpUp_Spot(short sAttackerH, char cAttackerType, short sTargetH, char cTargetType, short sV1, short sV2, short sV3);
	void Effect_SpUp_Spot(short sAttackerH, char cAttackerType, short sTargetH, char cTargetType, short sV1, short sV2, short sV3);
	void Effect_SpDown_Spot(short sAttackerH, char cAttackerType, short sTargetH, char cTargetType, short sV1, short sV2, short sV3);

	// Resistance checks
	bool bCheckResistingMagicSuccess(char cAttackerDir, short sTargetH, char cTargetType, int iHitRatio);
	bool bCheckResistingIceSuccess(char cAttackerDir, short sTargetH, char cTargetType, int iHitRatio);
	bool bCheckResistingPoisonSuccess(short sOwnerH, char cOwnerType);

	// Kill handler
	void ClientKilledHandler(int iClientH, int iAttackerH, char cAttackerType, short sDamage);

	// Combat status
	void PoisonEffect(int iClientH, int iV1);
	void CheckFireBluring(char cMapIndex, int sX, int sY);
	void ArmorLifeDecrement(int iAttackerH, int iTargetH, char cOwnerType, int iValue);

	// Attack type helpers
	void _CheckAttackType(int iClientH, short * spType);
	int  _iGetWeaponSkillType(int iClientH);
	int  iGetComboAttackBonus(int iSkill, int iComboCount);

	// Hostility / criminal
	bool bAnalyzeCriminalAction(int iClientH, short dX, short dY, bool bIsCheck = false);
	bool _bGetIsPlayerHostile(int iClientH, int sOwnerH);
	int  iGetPlayerRelationship(int iClientH, int iOpponentH);
	EntityRelationship GetPlayerRelationship(int iOwnerH, int iViewerH);

	// Target management
	void RemoveFromTarget(short sTargetH, char cTargetType, int iCode = 0);
	int  iGetDangerValue(int iNpcH, short dX, short dY);

	// Combat validation
	bool bCheckClientAttackFrequency(int iClientH, uint32_t dwClientTime);

	// Logging
	bool _bPKLog(int iAction, int iAttackerH, int iVictumH, char * pNPC);

private:
	CGame* m_pGame = nullptr;
};

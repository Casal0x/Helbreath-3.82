#pragma once
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <array>
#include "PlayerController.h"
#include "NetConstants.h"
#include "AppearanceData.h"
#include "PlayerStatusData.h"
#include "ActionID.h"

#define DEF_PLAYERNAME_LENGTH      12
#define DEF_GUILDNAME_LENGTH       22
#define DEF_PLAYER_MAXMAGICTYPE    100
#define DEF_PLAYER_MAXSKILLTYPE    60

//=============================================================================
// Player Animation Definitions
// All player types (1-6) share these timings.
// Values from original m_stFrame[1-6] initialization in MapData.cpp
//=============================================================================
struct AnimDef
{
	int16_t sMaxFrame;
	int16_t sFrameTime;  // Base ms per frame (before status modifiers)
	bool    bLoop;
};

namespace PlayerAnim {
	static constexpr AnimDef Stop       = { 14, 60,  false };
	static constexpr AnimDef Move       = {  7, 74,  false };
	static constexpr AnimDef Run        = {  7, 39,  false };
	static constexpr AnimDef Attack     = {  7, 78,  false };
	static constexpr AnimDef AttackMove = { 12, 78,  false };
	static constexpr AnimDef Magic      = { 15, 88,  false };
	static constexpr AnimDef GetItem    = {  3, 150, false };
	static constexpr AnimDef Damage     = {  7, 70,  false }; // 3+4
	static constexpr AnimDef DamageMove = {  3, 50,  false };
	static constexpr AnimDef Dying      = { 12, 80,  false };

	inline const AnimDef& FromAction(int8_t action)
	{
		switch (action) {
		case DEF_OBJECTSTOP:       return Stop;
		case DEF_OBJECTMOVE:       return Move;
		case DEF_OBJECTRUN:        return Run;
		case DEF_OBJECTATTACK:     return Attack;
		case DEF_OBJECTATTACKMOVE: return AttackMove;
		case DEF_OBJECTMAGIC:      return Magic;
		case DEF_OBJECTGETITEM:    return GetItem;
		case DEF_OBJECTDAMAGE:     return Damage;
		case DEF_OBJECTDAMAGEMOVE: return DamageMove;
		case DEF_OBJECTDYING:      return Dying;
		default:                   return Stop;
		}
	}
}

class CPlayer
{
public:
    CPlayer();
    ~CPlayer();
    void Reset();

    // Movement Controller
    CPlayerController m_Controller;

    // IDENTITY & ACCOUNT
    char m_cPlayerName[DEF_PLAYERNAME_LENGTH];
    short m_sPlayerObjectID;
    short m_sPlayerType;
    char m_cAccountName[DEF_ACCOUNT_NAME];
    char m_cAccountPassword[DEF_ACCOUNT_PASS];
    char m_cGuildName[DEF_GUILDNAME_LENGTH];
    int m_iGuildRank;
    int m_iTotalGuildsMan;

    // POSITION & MOVEMENT
    short m_sPlayerX, m_sPlayerY;
    int8_t m_iPlayerDir;
    short m_sDamageMove, m_sDamageMoveAmount;

    // RESOURCES
    int m_iHP, m_iMP, m_iSP, m_iHungerStatus;

    // BASE STATS
    int m_iStr, m_iVit, m_iDex, m_iInt, m_iMag, m_iCharisma;
    int m_iAngelicStr, m_iAngelicInt, m_iAngelicDex, m_iAngelicMag;

    // PROGRESSION
    int m_iLevel;
    uint32_t m_iExp;
    int m_iLU_Point;
    int16_t m_wLU_Str, m_wLU_Vit, m_wLU_Dex, m_wLU_Int, m_wLU_Mag, m_wLU_Char;
    int8_t m_iStatModStr, m_iStatModVit, m_iStatModDex, m_iStatModInt, m_iStatModMag, m_iStatModChr;

    // COMBAT
    int m_iAC, m_iTHAC0;
    PlayerStatus m_playerStatus;
    int m_iPKCount, m_iEnemyKillCount, m_iRewardGold, m_iContribution;
    int m_iSuperAttackLeft, m_iSpecialAbilityType, m_iSpecialAbilityTimeLeftSec;

    // APPEARANCE
    PlayerAppearance m_playerAppearance;

    // Illusion Effect appearance
    PlayerStatus m_illusionStatus;
    PlayerAppearance m_illusionAppearance;
    int8_t m_iGender, m_iSkinCol, m_iHairStyle, m_iHairCol, m_iUnderCol;

    // SKILLS & MAGIC
    std::array<int8_t, DEF_PLAYER_MAXMAGICTYPE> m_iMagicMastery{};
    std::array<uint8_t, DEF_PLAYER_MAXSKILLTYPE> m_iSkillMastery{};

    // STATUS FLAGS
    bool m_bIsPoisoned, m_bIsConfusion, m_bParalyze;
    bool m_bIsCombatMode, m_bIsSafeAttackMode, m_bForceAttack;
    bool m_bSuperAttackMode, m_bIsSpecialAbilityEnabled;
    bool m_bHunter, m_bAresden, m_bCitizen;

    // ADMIN / GM
    bool m_bIsGMMode = false;

    // CRUSADE/WAR
    int m_iCrusadeDuty, m_iWarContribution, m_iConstructionPoint;
    int m_iConstructLocX, m_iConstructLocY;
};

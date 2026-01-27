#pragma once
#include <windows.h>
#include <cstdint>
#include <cstring>
#include <array>
#include "PlayerController.h"

#define DEF_PLAYERNAME_LENGTH      12
#define DEF_GUILDNAME_LENGTH       22
#define DEF_ACCOUNTNAME_LENGTH     12
#define DEF_ACCOUNTPASS_LENGTH     12
#define DEF_PLAYER_MAXMAGICTYPE    100
#define DEF_PLAYER_MAXSKILLTYPE    60

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
    char m_cAccountName[DEF_ACCOUNTNAME_LENGTH];
    char m_cAccountPassword[DEF_ACCOUNTPASS_LENGTH];
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
    uint16_t m_wLU_Str, m_wLU_Vit, m_wLU_Dex, m_wLU_Int, m_wLU_Mag, m_wLU_Char;
    int8_t m_iStatModStr, m_iStatModVit, m_iStatModDex, m_iStatModInt, m_iStatModMag, m_iStatModChr;

    // COMBAT
    int m_iAC, m_iTHAC0, m_iPlayerStatus;
    int m_iPKCount, m_iEnemyKillCount, m_iRewardGold, m_iContribution;
    int m_iSuperAttackLeft, m_iSpecialAbilityType, m_iSpecialAbilityTimeLeftSec;

    // APPEARANCE
    short m_sPlayerAppr1, m_sPlayerAppr2, m_sPlayerAppr3, m_sPlayerAppr4;
    int m_iPlayerApprColor;
    short m_sAppr1_IE, m_sAppr2_IE, m_sAppr3_IE, m_sAppr4_IE;
    int m_iApprColor_IE, m_iStatus_IE;
    int8_t m_iGender, m_iSkinCol, m_iHairStyle, m_iHairCol, m_iUnderCol;

    // SKILLS & MAGIC
    std::array<int8_t, DEF_PLAYER_MAXMAGICTYPE> m_iMagicMastery{};
    std::array<uint8_t, DEF_PLAYER_MAXSKILLTYPE> m_iSkillMastery{};

    // STATUS FLAGS
    bool m_bIsPoisoned, m_bIsConfusion, m_bParalyze;
    bool m_bIsCombatMode, m_bIsSafeAttackMode, m_bForceAttack;
    bool m_bSuperAttackMode, m_bIsSpecialAbilityEnabled;
    bool m_bHunter, m_bAresden, m_bCitizen;

    // CRUSADE/WAR
    int m_iCrusadeDuty, m_iWarContribution, m_iConstructionPoint;
    int m_iConstructLocX, m_iConstructLocY;
};

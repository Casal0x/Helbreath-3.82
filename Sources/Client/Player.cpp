#include "Player.h"

CPlayer::CPlayer()
{
    Reset();
}

CPlayer::~CPlayer()
{
}

void CPlayer::Reset()
{
    // IDENTITY & ACCOUNT
    std::memset(m_cPlayerName, 0, sizeof(m_cPlayerName));
    m_sPlayerObjectID = 0;
    m_sPlayerType = 0;
    std::memset(m_cAccountName, 0, sizeof(m_cAccountName));
    std::memset(m_cAccountPassword, 0, sizeof(m_cAccountPassword));
    std::memset(m_cGuildName, 0, sizeof(m_cGuildName));
    m_iGuildRank = 0;
    m_iTotalGuildsMan = 0;

    // POSITION & MOVEMENT
    m_sPlayerX = 0;
    m_sPlayerY = 0;
    m_iPlayerDir = 0;
    m_sDamageMove = 0;
    m_sDamageMoveAmount = 0;

    // RESOURCES
    m_iHP = 0;
    m_iMP = 0;
    m_iSP = 0;
    m_iHungerStatus = 0;

    // BASE STATS
    m_iStr = 0;
    m_iVit = 0;
    m_iDex = 0;
    m_iInt = 0;
    m_iMag = 0;
    m_iCharisma = 0;
    m_iAngelicStr = 0;
    m_iAngelicInt = 0;
    m_iAngelicDex = 0;
    m_iAngelicMag = 0;

    // PROGRESSION
    m_iLevel = 0;
    m_iExp = 0;
    m_iLU_Point = 0;
    m_wLU_Str = 0;
    m_wLU_Vit = 0;
    m_wLU_Dex = 0;
    m_wLU_Int = 0;
    m_wLU_Mag = 0;
    m_wLU_Char = 0;
    m_iStatModStr = 0;
    m_iStatModVit = 0;
    m_iStatModDex = 0;
    m_iStatModInt = 0;
    m_iStatModMag = 0;
    m_iStatModChr = 0;

    // COMBAT
    m_iAC = 0;
    m_iTHAC0 = 0;
    m_playerStatus.Clear();
    m_iPKCount = 0;
    m_iEnemyKillCount = 0;
    m_iRewardGold = 0;
    m_iContribution = 0;
    m_iSuperAttackLeft = 0;
    m_iSpecialAbilityType = 0;
    m_iSpecialAbilityTimeLeftSec = 0;

    // APPEARANCE
    m_playerAppearance.Clear();
    m_illusionStatus.Clear();
    m_illusionAppearance.Clear();
    m_iGender = 0;
    m_iSkinCol = 0;
    m_iHairStyle = 0;
    m_iHairCol = 0;
    m_iUnderCol = 0;

    // SKILLS & MAGIC
    m_iMagicMastery.fill(0);
    m_iSkillMastery.fill(0);

    // STATUS FLAGS
    m_bIsPoisoned = false;
    m_bIsConfusion = false;
    m_bParalyze = false;
    m_bIsCombatMode = false;
    m_bIsSafeAttackMode = false;
    m_bForceAttack = false;
    m_bSuperAttackMode = false;
    m_bIsSpecialAbilityEnabled = false;
    m_bHunter = false;
    m_bAresden = false;
    m_bCitizen = false;

    // CRUSADE/WAR
    m_iCrusadeDuty = 0;
    m_iWarContribution = 0;
    m_iConstructionPoint = 0;
    m_iConstructLocX = 0;
    m_iConstructLocY = 0;
}

#pragma once
#include "StatusFlags.h"

// EntityStatus: Base status fields shared by both NPCs and Players.
// NPCs use attack delay, angel percent, and the basic effect flags.
struct EntityStatus
{
	// Multi-bit fields
	int iAttackDelay;          // bits 0-3
	int iAngelPercent;         // bits 8-11

	// Boolean flags (shared NPC+Player)
	bool bInvisibility;        // bit 4
	bool bBerserk;             // bit 5
	bool bFrozen;              // bit 6
	bool bPoisoned;            // bit 7

	void Unpack(int iStatus)
	{
		iAttackDelay  = iStatus & hb::status::AttackDelayMask;
		iAngelPercent = (iStatus & hb::status::AngelPercentMask) >> 8;

		bInvisibility = (iStatus & hb::status::Invisibility) != 0;
		bBerserk      = (iStatus & hb::status::Berserk) != 0;
		bFrozen       = (iStatus & hb::status::Frozen) != 0;
		bPoisoned     = (iStatus & hb::status::Poisoned) != 0;
	}

	void Clear()
	{
		iAttackDelay  = 0;
		iAngelPercent = 0;
		bInvisibility = false;
		bBerserk      = false;
		bFrozen       = false;
		bPoisoned     = false;
	}
};

// PlayerStatus: Extends EntityStatus with player-only fields.
// CTile and EntityRenderState store PlayerStatus (the superset) since any
// tile can hold either a player or NPC. Functions that only need NPC-level
// data can accept const EntityStatus&.
struct PlayerStatus : EntityStatus
{
	// Angel type flags (player-only)
	bool bAngelSTR;            // bit 12
	bool bAngelDEX;            // bit 13
	bool bAngelINT;            // bit 14
	bool bAngelMAG;            // bit 15

	// Player effect flags
	bool bSlateExp;            // bit 16
	bool bHero;                // bit 17
	bool bHaste;               // bit 18
	bool bGMMode;              // bit 19
	bool bInhibitionCasting;   // bit 20
	bool bIllusionMovement;    // bit 21
	bool bSlateInvincible;     // bit 22
	bool bSlateMana;           // bit 23
	bool bIllusion;            // bit 24
	bool bDefenseShield;       // bit 25
	bool bMagicProtection;     // bit 26
	bool bProtectionFromArrow; // bit 27

	// Faction/identity flags
	bool bHunter;              // bit 28
	bool bAresden;             // bit 29
	bool bCitizen;             // bit 30
	bool bPK;                  // bit 31

	bool HasAngelType() const
	{
		return bAngelSTR || bAngelDEX || bAngelINT || bAngelMAG;
	}

	void Unpack(int iStatus)
	{
		EntityStatus::Unpack(iStatus);

		bAngelSTR             = (iStatus & hb::status::AngelSTR) != 0;
		bAngelDEX             = (iStatus & hb::status::AngelDEX) != 0;
		bAngelINT             = (iStatus & hb::status::AngelINT) != 0;
		bAngelMAG             = (iStatus & hb::status::AngelMAG) != 0;
		bSlateExp             = (iStatus & hb::status::SlateExp) != 0;
		bHero                 = (iStatus & hb::status::Hero) != 0;
		bHaste                = (iStatus & hb::status::Haste) != 0;
		bGMMode               = (iStatus & hb::status::GMMode) != 0;
		bInhibitionCasting    = (iStatus & hb::status::InhibitionCasting) != 0;
		bIllusionMovement     = (iStatus & hb::status::IllusionMovement) != 0;
		bSlateInvincible      = (iStatus & hb::status::SlateInvincible) != 0;
		bSlateMana            = (iStatus & hb::status::SlateMana) != 0;
		bIllusion             = (iStatus & hb::status::Illusion) != 0;
		bDefenseShield        = (iStatus & hb::status::DefenseShield) != 0;
		bMagicProtection      = (iStatus & hb::status::MagicProtection) != 0;
		bProtectionFromArrow  = (iStatus & hb::status::ProtectionFromArrow) != 0;
		bHunter               = (iStatus & hb::status::Hunter) != 0;
		bAresden              = (iStatus & hb::status::Aresden) != 0;
		bCitizen              = (iStatus & hb::status::Citizen) != 0;
		bPK                   = (iStatus & hb::status::PK) != 0;
	}

	void Clear()
	{
		EntityStatus::Clear();
		bAngelSTR = false;
		bAngelDEX = false;
		bAngelINT = false;
		bAngelMAG = false;
		bSlateExp = false;
		bHero = false;
		bHaste = false;
		bGMMode = false;
		bInhibitionCasting = false;
		bIllusionMovement = false;
		bSlateInvincible = false;
		bSlateMana = false;
		bIllusion = false;
		bDefenseShield = false;
		bMagicProtection = false;
		bProtectionFromArrow = false;
		bHunter = false;
		bAresden = false;
		bCitizen = false;
		bPK = false;
	}
};

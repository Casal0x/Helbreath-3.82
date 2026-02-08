#pragma once

#include "Packet/PacketCommon.h"
#include "EntityRelationship.h"
#include <cstdint>
#include <cstring>

// Compile-time guarantee that bool is 1 byte (true on all major compilers).
// This ensures bool fields in packed structs have the same wire size as uint8_t.
static_assert(sizeof(bool) == 1, "bool must be 1 byte for packed struct compatibility");

// EntityStatus: Base status fields shared by both NPCs and Players.
// Replaces the old packed int32_t iStatus bitmask.
HB_PACK_BEGIN
struct HB_PACKED EntityStatus
{
	// Multi-value fields (0-15 range)
	uint8_t iAttackDelay;
	uint8_t iAngelPercent;

	// Boolean flags (shared NPC+Player)
	bool bInvisibility;
	bool bBerserk;
	bool bFrozen;
	bool bPoisoned;

	// Combat effect flags (apply to both NPCs and players)
	bool bInhibitionCasting;
	bool bIllusion;
	bool bHero;
	bool bDefenseShield;
	bool bMagicProtection;
	bool bProtectionFromArrow;

	// Per-viewer NPC relationship (set by server before sending, not persisted)
	EntityRelationship iRelationship;

	void Clear() { std::memset(this, 0, sizeof(*this)); }
};

// PlayerStatus: Full status for players. Contains all EntityStatus fields plus player-only fields.
// Uses composition (not inheritance) to remain standard_layout for packet embedding.
// CTile and EntityRenderState store PlayerStatus (the superset).
struct HB_PACKED PlayerStatus
{
	// === EntityStatus fields (must match EntityStatus layout) ===
	uint8_t iAttackDelay;
	uint8_t iAngelPercent;
	bool bInvisibility;
	bool bBerserk;
	bool bFrozen;
	bool bPoisoned;
	bool bInhibitionCasting;
	bool bIllusion;
	bool bHero;
	bool bDefenseShield;
	bool bMagicProtection;
	bool bProtectionFromArrow;
	EntityRelationship iRelationship;

	// === Player-only fields ===
	// Angel type flags
	bool bAngelSTR;
	bool bAngelDEX;
	bool bAngelINT;
	bool bAngelMAG;

	// Player effect flags
	bool bSlateExp;
	bool bHaste;
	bool bGMMode;
	bool bIllusionMovement;
	bool bSlateInvincible;
	bool bSlateMana;

	// Faction/identity flags
	bool bHunter;
	bool bAresden;
	bool bCitizen;
	bool bPK;

	// AFK state (server-controlled)
	bool bAfk;

	bool HasAngelType() const
	{
		return bAngelSTR || bAngelDEX || bAngelINT || bAngelMAG;
	}

	void Clear() { std::memset(this, 0, sizeof(*this)); }

	// Assign from EntityStatus (NPC -> tile storage). Clears player-only fields.
	void SetFromEntityStatus(const EntityStatus& npc)
	{
		Clear();
		iAttackDelay = npc.iAttackDelay;
		iAngelPercent = npc.iAngelPercent;
		bInvisibility = npc.bInvisibility;
		bBerserk = npc.bBerserk;
		bFrozen = npc.bFrozen;
		bPoisoned = npc.bPoisoned;
		bInhibitionCasting = npc.bInhibitionCasting;
		bIllusion = npc.bIllusion;
		bHero = npc.bHero;
		bDefenseShield = npc.bDefenseShield;
		bMagicProtection = npc.bMagicProtection;
		bProtectionFromArrow = npc.bProtectionFromArrow;
		iRelationship = npc.iRelationship;
	}
};
HB_PACK_END

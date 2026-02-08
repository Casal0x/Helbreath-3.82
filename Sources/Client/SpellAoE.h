// SpellAoE.h: Spell area-of-effect tile calculator for targeting overlay
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include <cstdint>

struct SpellAoETile
{
	int x, y;
};

// Parameters extracted from CMagic for AoE calculation
struct SpellAoEParams
{
	int magicType;      // DEF_MAGICTYPE_*
	int aoeRadiusX;     // Server m_sValue2 — area spell X radius
	int aoeRadiusY;     // Server m_sValue3 — area spell Y radius
	int dynamicPattern; // Server m_sValue11 — 1=wall, 2=field
	int dynamicRadius;  // Server m_sValue12 — wall length / field radius
};

namespace SpellAoE
{
	// Calculates all affected tiles for a spell cast from (casterX,casterY)
	// targeting (targetX,targetY). Returns number of tiles written to outTiles.
	int CalculateTiles(const SpellAoEParams& params,
		int casterX, int casterY,
		int targetX, int targetY,
		SpellAoETile* outTiles, int outMax);
}

#pragma once

#include <cstdint>


namespace hb::shared::calc {

// SharedCalculations.h - Shared Formulas for Client and Server
//
// This file contains stat and resource calculations that must be identical
// on both client and server to ensure consistency.

// ============================================================================
// Max Resource Calculations
// ============================================================================

// Calculate maximum HP
// Formula: Vit * 3 + Level * 2 + (Str + AngelicStr) / 2
inline int CalculateMaxHP(int vit, int level, int str, int angelic_str)
{
	return vit * 3 + level * 2 + (str + angelic_str) / 2;
}

// Calculate maximum MP
// Formula: (Mag + AngelicMag) * 2 + Level * 2 + (Int + AngelicInt) / 2
inline int CalculateMaxMP(int mag, int angelic_mag, int level, int iInt, int angelic_int)
{
	return (mag + angelic_mag) * 2 + level * 2 + (iInt + angelic_int) / 2;
}

// Calculate maximum SP (Stamina)
// Formula: (Str + AngelicStr) * 2 + Level * 2
inline int CalculateMaxSP(int str, int angelic_str, int level)
{
	return (str + angelic_str) * 2 + level * 2;
}

// ============================================================================
// Carry Weight Calculations
// ============================================================================

// Calculate maximum carry weight
// Formula: (Str + AngelicStr) * 5 + Level * 5
inline int CalculateMaxLoad(int str, int angelic_str, int level)
{
	return (str + angelic_str) * 5 + level * 5;
}

// ============================================================================
// Max Stat Calculations
// ============================================================================

// Calculate maximum stat value based on server configuration
// Formula: BaseStatValue + CreationStatBonus + (LevelupStatGain * MaxLevel) + AngelicBonus(16)
inline int CalculateMaxStatValue(int base_stat_value, int creation_stat_bonus, int levelup_stat_gain, int max_level)
{
	return base_stat_value + creation_stat_bonus + (levelup_stat_gain * max_level) + 16;
}

// ============================================================================
// Experience Calculations
// ============================================================================

// Calculate experience required for a given level
// Formula: Recursive - GetLevelExp(Level-1) + Level * (50 + (Level * (Level/17) * (Level/17)))
// Note: This is a recursive function - for performance, server pre-caches this in m_level_exp_table[]
inline uint32_t CalculateLevelExp(int level)
{
	if (level <= 0) return 0;
	return CalculateLevelExp(level - 1) + level * (50 + (level * (level / 17) * (level / 17)));
}

// ============================================================================
// Level-Up Point Pool Calculations
// ============================================================================

// Calculate level-up stat points pool (server-side storage value)
// Formula: Level * 3 - (TotalStats - 70)
// Where TotalStats = Str + Vit + Dex + Int + Mag + Charisma
inline int CalculateLevelUpPool(int level, int total_stats)
{
	return level * 3 - (total_stats - 70);
}

// Calculate level-up stat points for display (client-side)
// Formula: Level * 3 - (TotalStats - 70) - 3
// The -3 accounts for base starting points in display
inline int CalculateLevelUpPointsDisplay(int level, int total_stats)
{
	return CalculateLevelUpPool(level, total_stats) - 3;
}

} // namespace hb::shared::calc

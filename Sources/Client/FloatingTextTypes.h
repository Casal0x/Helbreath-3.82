#pragma once

#include <cstdint>
#include "CommonTypes.h"

// ---------------------------------------------------------------
// Floating text category enums
// ---------------------------------------------------------------

enum class ChatTextType : uint8_t {
	PlayerChat
};

enum class DamageTextType : uint8_t {
	Small,      // <12 pts  - small sprite font
	Medium,     // 12-39 pts, Immune, Failed - medium sprite font
	Large,      // 40+ pts, Critical! - large sprite font
};

enum class NotifyTextType : uint8_t {
	SkillChange,    // "+2% Mining" - yellow, delayed 650ms
	MagicCastName,  // "Fire Ball!" - red sprite font
	LevelUp,        // "Level up!" - large sprite font
	EnemyKill,      // "Enemy Kill!" - large sprite font
};

// ---------------------------------------------------------------
// Animation parameters for each floating text type
// ---------------------------------------------------------------

struct AnimParams {
	uint32_t dwLifetimeMs;    // Total display duration
	uint32_t dwShowDelayMs;   // Delay before visible (0 = instant)
	int iStartOffsetY;        // Starting Y offset above entity foot (pixels)
	int iRisePixels;          // Total upward rise distance
	int iRiseDurationMs;      // Time to reach final position
	int iFontOffset;          // Offset from SprFont3_0 (0=large, 1=medium, 2=small)
	Color color;              // Text color
	bool bUseSpriteFont;      // true = sprite font, false = renderer text
};

// ---------------------------------------------------------------
// Parameter tables (constexpr)
// ---------------------------------------------------------------

namespace FloatingTextParams {

inline constexpr AnimParams Chat[] = {
	// PlayerChat: white renderer text, 4s, slow rise
	{ 4000, 0, 55, 1, 200, 0, GameColors::UIWhite, false },
};

inline constexpr AnimParams Damage[] = {
	// Small:  yellow sprite font (small), 200ms, fast rise
	{ 500, 0, 55, 20, 200, 2, GameColors::UIDmgYellow, true },
	// Medium: yellow sprite font (medium), 200ms, fast rise
	{ 500, 0, 55, 20, 200, 1, GameColors::UIDmgYellow, true },
	// Large:  yellow sprite font (large), 200ms, fast rise
	{ 500, 0, 55, 20, 200, 0, GameColors::UIDmgYellow, true },
};

inline constexpr AnimParams Notify[] = {
	// SkillChange: yellow renderer text, 4s, delayed 650ms
	{ 4000, 0, 55, 10, 80, 0, GameColors::UIDmgYellow, false },
	// MagicCastName: red sprite font (large), 2s
	{ 2000, 0, 55, 15, 200, 0, GameColors::UIDmgRed, true },
	// LevelUp: yellow sprite font (large), 2s, fast rise
	{ 2000, 0, 55, 10, 80, 0, GameColors::UIDmgYellow, true },
	// EnemyKill: yellow sprite font (large), 2s, fast rise
	{ 2000, 0, 55, 10, 80, 0, GameColors::UIDmgYellow, true },
};

} // namespace FloatingTextParams

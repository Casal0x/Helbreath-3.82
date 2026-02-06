// CommonTypes.h: Cross-platform type definitions
//
// This header replaces Windows-specific typedefs (DWORD, WORD, BYTE) with
// standard C++ types (uint32_t, uint16_t, uint8_t) for cross-platform compatibility.
//
// Windows API types (HWND, HANDLE, HRESULT) are preserved for platform-specific code.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include <cstdint>
#include <cstring>
#include <chrono>

// ============================================================================
// Standard C++ Type Replacements
// ============================================================================
//
// The following Windows typedefs have been replaced throughout the codebase:
//   DWORD → uint32_t (32-bit unsigned integer)
//   WORD  → uint16_t (16-bit unsigned integer)
//   BYTE  → uint8_t  (8-bit unsigned integer)
//
// Hungarian notation is preserved (e.g., dwTime remains dwTime, but type is uint32_t)
//
// ============================================================================

// Windows API types (platform-specific)
// These are used with DirectX and Windows system calls and must remain Windows-specific
#ifdef _WIN32
    #ifndef _WINSOCKAPI_
        #define _WINSOCKAPI_   // Prevent winsock.h from being included by windows.h
    #endif
    #include <windows.h>
    // HWND, HANDLE, HRESULT, HINSTANCE, RECT are provided by windows.h
#endif

#include "GameGeometry.h"

// ============================================================================
// Memory Operations
// ============================================================================

// Windows ZeroMemory() should be replaced with std::memset() throughout the codebase
// Old: ZeroMemory(&struct, sizeof(struct));
// New: std::memset(&struct, 0, sizeof(struct));

// ============================================================================
// Timing System
// ============================================================================

// GameClock: Cross-platform timing system
// Replaces Windows timeGetTime() with std::chrono for cross-platform compatibility
//
// Usage:
//   GameClock::Initialize();  // Call once at startup
//   uint32_t time = GameClock::GetTimeMS();  // Get current time in milliseconds
//
// Note: Time wraps around after ~49.7 days (2^32 milliseconds), same as timeGetTime().
//       Use delta time calculations for robust timing that handles wraparound.
//
class GameClock {
private:
    static std::chrono::steady_clock::time_point s_startTime;
    static bool s_initialized;

public:
    // Initialize the game clock - call once at application startup
    static void Initialize() {
        if (!s_initialized) {
            s_startTime = std::chrono::steady_clock::now();
            s_initialized = true;
        }
    }

    // Get milliseconds since initialization
    // Returns: uint32_t milliseconds (0 to 4,294,967,295, wraps after ~49.7 days)
    //
    // This replaces Windows timeGetTime() with equivalent behavior:
    //   Old: DWORD dwTime = timeGetTime();
    //   New: uint32_t dwTime = GameClock::GetTimeMS();
    //
    static uint32_t GetTimeMS() {
        if (!s_initialized) Initialize();

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - s_startTime);
        return static_cast<uint32_t>(elapsed.count());
    }
};

// ============================================================================
// Color System
// ============================================================================

struct GameColor
{
	uint8_t r, g, b, a;
	constexpr GameColor() : r(0), g(0), b(0), a(255) {}
	constexpr GameColor(uint8_t r_, uint8_t g_, uint8_t b_, uint8_t a_ = 255) : r(r_), g(g_), b(b_), a(a_) {}
};


namespace GameColors
{
	// Base gray (subtracted for tinting)
	inline constexpr GameColor Base{ 96, 96, 96 };

	// Item/Equipment palette (indices 0-15)
	inline constexpr GameColor Items[16] = {
		{ 96, 96, 96 },   // 0  Base gray
		{ 40, 40, 96 },   // 1  IndigoBlue
		{ 72, 72, 56 },   // 2  CustomWeapon
		{ 128, 104, 24 }, // 3  Gold
		{ 120, 16, 0 },   // 4  Crimson
		{ 8, 56, 8 },     // 5  Green
		{ 40, 40, 40 },   // 6  Gray
		{ 40, 72, 80 },   // 7  Aqua
		{ 120, 48, 88 },  // 8  Pink
		{ 88, 56, 88 },   // 9  Violet
		{ 0, 32, 56 },    // 10 Blue
		{ 104, 88, 64 },  // 11 Tan
		{ 88, 88, 48 },   // 12 Khaki
		{ 80, 80, 8 },    // 13 Yellow
		{ 72, 8, 8 },     // 14 Red
		{ 48, 48, 48 },   // 15 Black
	};

	// Weapon palette (indices 0-15, only 1-9 are set)
	inline constexpr GameColor Weapons[16] = {
		{ 0, 0, 0 },      // 0  (unused)
		{ 64, 64, 80 },   // 1  LightBlue
		{ 64, 64, 80 },   // 2  LightBlue
		{ 64, 64, 80 },   // 3  LightBlue
		{ 64, 96, 64 },   // 4  Green
		{ 128, 88, 8 },   // 5  Critical
		{ 40, 48, 104 },  // 6  HeavyBlue
		{ 144, 144, 144 },// 7  White
		{ 120, 96, 120 }, // 8  Violet
		{ 72, 8, 8 },     // 9  HeavyRed
		{ 0, 0, 0 },      // 10 (unused)
		{ 0, 0, 0 },      // 11 (unused)
		{ 0, 0, 0 },      // 12 (unused)
		{ 0, 0, 0 },      // 13 (unused)
		{ 0, 0, 0 },      // 14 (unused)
		{ 0, 0, 0 },      // 15 (unused)
	};

	// Pre-computed math colors
	// original: m_wR[13]*2, m_wG[13]*2, m_wB[13]*2
	inline constexpr GameColor Yellow2x{ 160, 160, 16 };
	// original: m_wR[13]*4, m_wG[13]*4, m_wB[13]*4
	inline constexpr GameColor Yellow4x{ 255, 255, 32 };
	// original: m_wR[14]*4, m_wG[14]*4, m_wB[14]*4
	inline constexpr GameColor Red4x{ 255, 32, 32 };
	// original: m_wR[5]*11, m_wG[5]*11, m_wB[5]*11
	inline constexpr GameColor PoisonText{ 88, 255, 88 };
	// original: m_wR[5]*8, m_wG[5]*8, m_wB[5]*8
	inline constexpr GameColor PoisonLabel{ 64, 255, 64 };
	// Status effect additive colors (for AdditiveColored blend)
	// Berserk: creates a bright reddish/golden glow when added to destination
	inline constexpr GameColor BerserkGlow{ 255, 240, 240 };

	// ====================================================================
	// UI Text Colors
	// ====================================================================

	// ====================================================================
	// HUD / Name Plate Colors
	// ====================================================================

	// TextLib::TextStyle::WithShadow (Game.cpp, Screen_OnGame.cpp)
	inline constexpr GameColor InfoGrayLight{ 180, 180, 180 };  // Lighter info text

	// ====================================================================
	// Additional UI Colors
	// ====================================================================

	inline constexpr GameColor UIMenuHighlight{ 250, 250, 0 };  // Teleport menu highlight (DialogBox_CityHallMenu, DialogBox_GuildHallMenu)
	inline constexpr GameColor UINoticeRed{ 100, 10, 10 };      // Notice message text (DialogBox_Noticement)
	inline constexpr GameColor UITooltip{ 250, 250, 220 };      // Tooltip text (DialogBox_HudPanel)
	inline constexpr GameColor UIDisabledMed{ 120, 120, 120 };      // Grayed out text (DialogBox_Manufacture)
	inline constexpr GameColor UISelectPurple{ 51, 0, 51 };     // Character select labels (Screen_SelectCharacter)
	inline constexpr GameColor UIFormLabel{ 100, 100, 200 };    // Form field labels (Screen_CreateAccount)
	inline constexpr GameColor UIProfileYellow{ 255, 255, 100 };// Profile overlay text (Screen_OnGame)
	inline constexpr GameColor UITopMsgYellow{ 255, 255, 0 };   // Top message text (Game.cpp)
	inline constexpr GameColor UIDmgYellow{ 255, 255, 20 };     // Damage text yellow (Game.cpp)
	inline constexpr GameColor UIDmgRed{ 255, 80, 80 };         // Damage text red (Game.cpp)
	inline constexpr GameColor UIBuildRed{ 180, 30, 30 };       // Build/craft warning (Game.cpp)
	inline constexpr GameColor ChatEventGreen{ 130, 255, 130 }; // Event history green (Game.cpp)
	inline constexpr GameColor UISlatesPink{ 220, 140, 160 };   // Slates effect text (DialogBox_Slates)
	inline constexpr GameColor UISlatesCyan{ 90, 220, 200 };    // Slates effect text (DialogBox_Slates)

	// Bitmap font button colors
	// TextLib::TextStyle::WithHighlight (various dialog boxes)
	inline constexpr GameColor BmpBtnNormal{ 0, 0, 7 };         // Normal bitmap button (DialogBox_NpcTalk, DialogBox_Exchange, DialogBox_Manufacture, DialogBox_Slates)
	inline constexpr GameColor BmpBtnHover{ 15, 15, 15 };       // Hover bitmap button (DialogBox_Exchange)
	inline constexpr GameColor BmpBtnActive{ 10, 10, 10 };      // Active bitmap button (Game.cpp, DialogBox_Manufacture)
	inline constexpr GameColor BmpBtnBlue{ 16, 16, 30 };        // Blue bitmap button (DialogBox_Manufacture)
	inline constexpr GameColor BmpBtnRed{ 20, 6, 6 };           // Red bitmap button (DialogBox_Manufacture)
	inline constexpr GameColor BmpBtnFishRed{ 10, 0, 0 };       // Fishing button (DialogBox_Fishing)

	// Minimap night colors (Game.cpp CMisc::ColorTransfer)
	inline constexpr GameColor NightBlueMid{ 50, 50, 100 };     // Night sky mid
	inline constexpr GameColor NightBlueDark{ 30, 30, 100 };    // Night sky dark
	inline constexpr GameColor NightBlueDeep{ 0, 0, 30 };       // Night deep
	inline constexpr GameColor NightBlueBright{ 50, 50, 200 };  // Night sky bright


	// Completed
	inline constexpr GameColor UIMagicBlue{ 4,0,50 };
	inline constexpr GameColor UIMagicPurple{ 60, 10, 60 };
	inline constexpr GameColor UIGuildGreen{ 130, 200, 130 };
	inline constexpr GameColor UIWorldChat{ 255, 130, 130 };
	inline constexpr GameColor UIFactionChat{ 130, 130, 255 };
	inline constexpr GameColor UIPartyChat{ 230, 230, 130 };
	inline constexpr GameColor UINormalChat{ 150, 150, 170 };
	inline constexpr GameColor UIGameMasterChat{ 180, 255, 180 };
	inline constexpr GameColor UILabel{ 25, 25, 25 };
	inline constexpr GameColor UIDisabled{ 65, 65, 65 };
	inline constexpr GameColor MonsterStatusEffect{ 240, 240, 70 };
	inline constexpr GameColor UIItemName_Special{ 0, 255, 50 };
	inline constexpr GameColor NeutralNamePlate{ 50, 50, 255 };
	inline constexpr GameColor EnemyNamePlate{ 255, 0, 0 };
	inline constexpr GameColor FriendlyNamePlate{ 30, 255, 30 };
	inline constexpr GameColor UIModifiedStat{ 0, 0, 192 };

	inline constexpr GameColor InputValid{ 100, 200, 100 };
	inline constexpr GameColor InputInvalid{ 200, 100, 100 };
	inline constexpr GameColor InputNormal{ 200, 200, 200 };

	inline constexpr GameColor UIBlack{ 0, 0, 0 };
	inline constexpr GameColor UIWhite{ 255, 255, 255 };
	inline constexpr GameColor UINearWhite{ 232, 232, 232 };
	inline constexpr GameColor UIDescription{ 150, 150, 150 };  // Item tooltips, stats, descriptions
	inline constexpr GameColor UIGreen{ 0, 255, 0 };
	inline constexpr GameColor UIDarkGreen{ 0, 55, 0 };
	inline constexpr GameColor UIRed{ 255, 0, 0 };
	inline constexpr GameColor UIDarkRed{ 58, 0, 0 };
	inline constexpr GameColor UIWarningRed{ 195, 25, 25 };
	inline constexpr GameColor UIOrange{ 220, 130, 45 };
	inline constexpr GameColor UIYellow{ 200, 200, 25 };
	inline constexpr GameColor UIPaleYellow{ 200, 200, 120 };
}

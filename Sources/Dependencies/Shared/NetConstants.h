#pragma once

// NetConstants.h - Shared Network Constants
//
// Shared between client and server to ensure buffer sizes stay in sync.

#define DEF_MSGBUFFERSIZE	60000
#define DEF_ITEMNAME		42

// Name and account field sizes (buffer = wire size + null terminator)
#define DEF_CHARNAME        11   // 10 chars + null (wire size is 10)
#define DEF_NPCNAME         21   // 20 chars + null (wire size is 20)
#define DEF_ACCOUNT_NAME    11   // 10 chars + null (wire size is 10)
#define DEF_ACCOUNT_PASS    11   // 10 chars + null (wire size is 10)
#define DEF_ACCOUNT_EMAIL   51   // 50 chars + null

// View area - tiles visible to the client (Olympia style)
// Uses the LARGER resolution (800x600) values so both resolutions work
// Width: 800/32 = 25 tiles, centered (12 left + 12 right)
// Height: (600-53)/32 = 17 tiles
#define DEF_VIEWTILES_X				25
#define DEF_VIEWTILES_Y				17
#define DEF_VIEWRANGE_X				(DEF_VIEWTILES_X / 2)	// 12
#define DEF_VIEWRANGE_Y				(DEF_VIEWTILES_Y / 2)	// 8
// Mastery array sizes (must match client and server)
#define DEF_MAXMAGICTYPE    100
#define DEF_MAXSKILLTYPE    60
#define DEF_MAXNPCCONFIGS   200

// Default max player level (server may override via GameConfigs.db)
#define DEF_PLAYERMAXLEVEL  180

// Init map data area (what server sends on map init/move)
#define DEF_INITDATA_TILES_X    DEF_VIEWTILES_X            // 25
#define DEF_INITDATA_TILES_Y    (DEF_VIEWTILES_Y + 2)      // 19

// View center (player position relative to init data origin)
#define DEF_VIEWCENTER_X        DEF_VIEWRANGE_X             // 12
#define DEF_VIEWCENTER_Y        (DEF_VIEWRANGE_Y + 1)       // 9

// Client buffer offsets (position of init data in client's 60x55 array)
#define DEF_MAPDATA_BUFFER_X    7
#define DEF_MAPDATA_BUFFER_Y    8

// Pivot offsets (player world pos to/from pivot)
#define DEF_PLAYER_PIVOT_OFFSET_X  (DEF_MAPDATA_BUFFER_X + DEF_VIEWCENTER_X)  // 19
#define DEF_PLAYER_PIVOT_OFFSET_Y  (DEF_MAPDATA_BUFFER_Y + DEF_VIEWCENTER_Y)  // 17

// Move table array size (longest direction: row + column + sentinel)
#define DEF_MOVELOC_MAX_ENTRIES (DEF_INITDATA_TILES_X + DEF_INITDATA_TILES_Y)  // 44

// Extra tiles beyond screen edge for entity broadcast (prevents pop-in/out)
#define DEF_VIEWRANGE_BUFFER 3

// Message buffer layout offsets
namespace hb {
namespace net {
	constexpr int MessageOffsetId   = 0;
	constexpr int MessageOffsetType = 4;
} // namespace net
} // namespace hb

// Game limits (authoritative values shared between client and server)
namespace hb {
namespace limits {
	constexpr int MaxItems              = 50;
	constexpr int MaxBankItems          = 1000;
	constexpr int MaxGuildsmen          = 128;
	constexpr int MaxBuildItems         = 300;
	constexpr int MaxPartyMembers       = 9;
	constexpr int MaxCrusadeStructures  = 300;
} // namespace limits
} // namespace hb

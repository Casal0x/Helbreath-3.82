#pragma once

// NetConstants.h - Shared Network Constants
//
// Shared between client and server to ensure buffer sizes stay in sync.

#define DEF_MSGBUFFERSIZE	60000
#define DEF_ITEMNAME		42

// Account field sizes (buffer = wire size + null terminator)
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

// Default max player level (server may override via GameConfigs.db)
#define DEF_PLAYERMAXLEVEL  180

// Extra tiles beyond screen edge for entity broadcast (prevents pop-in/out)
#define DEF_VIEWRANGE_BUFFER 3

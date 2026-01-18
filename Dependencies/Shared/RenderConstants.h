// RenderConstants.h: Shared rendering constants between client and renderer
//
// This header contains constants needed by the renderer that were originally
// defined in GlobalDef.h. Moving them here allows the renderer library to
// be independent of client-specific definitions.
//
//////////////////////////////////////////////////////////////////////

#pragma once

// Logical rendering resolution (internal coordinate system)
// The renderer always works in this coordinate space, regardless of window size
#define RENDER_LOGICAL_WIDTH    640
#define RENDER_LOGICAL_HEIGHT   480

// For backward compatibility with existing code that uses LOGICAL_WIDTH/HEIGHT
#ifndef LOGICAL_WIDTH
#define LOGICAL_WIDTH   RENDER_LOGICAL_WIDTH
#endif

#ifndef LOGICAL_HEIGHT
#define LOGICAL_HEIGHT  RENDER_LOGICAL_HEIGHT
#endif

// Max coordinates (for bounds checking)
#ifndef LOGICAL_MAX_X
#define LOGICAL_MAX_X   (LOGICAL_WIDTH - 1)
#endif

#ifndef LOGICAL_MAX_Y
#define LOGICAL_MAX_Y   (LOGICAL_HEIGHT - 1)
#endif

// Pixel format constants
#define PIXELFORMAT_RGB565  1   // 5:6:5 format (R=5, G=6, B=5)
#define PIXELFORMAT_RGB555  2   // 5:5:5 format (R=5, G=5, B=5)
#define PIXELFORMAT_BGR565  3   // 5:6:5 BGR format

// Transparency table sizes
#define TRANS_TABLE_SIZE    64
#define ADD_TABLE_SIZE      510

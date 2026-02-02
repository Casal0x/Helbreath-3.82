// RenderConstants.h: Shared rendering constants between client and renderer
//
// This header contains constants needed by the renderer that were originally
// defined in GlobalDef.h. Moving them here allows the renderer library to
// be independent of client-specific definitions.
//
// Resolution-dependent values are now provided by ResolutionConfig singleton.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "ResolutionConfig.h"

// Logical rendering resolution - now dynamic based on settings
// Use these inline functions instead of the old macros
inline int RENDER_LOGICAL_WIDTH()  { return ResolutionConfig::Get().LogicalWidth(); }
inline int RENDER_LOGICAL_HEIGHT() { return ResolutionConfig::Get().LogicalHeight(); }

// PDBGS (Pre-Draw Background Surface) size
// This surface holds tiles for smooth scrolling and needs to be larger than
// the visible area by 32 pixels in each direction (one tile width/height)
//
// NOTE: GlobalDef.h defines PDBGS_WIDTH/HEIGHT for client code.
// If GlobalDef.h is NOT included (i.e., in renderer engine code), we define them here.
// We use a define guard to prevent redefinition.
#ifndef GLOBALDEF_H_RESOLUTION_FUNCTIONS
inline int PDBGS_WIDTH()  { return ResolutionConfig::Get().PdbgsWidth(); }
inline int PDBGS_HEIGHT() { return ResolutionConfig::Get().PdbgsHeight(); }

// These are also defined in GlobalDef.h for client code
inline int LOGICAL_WIDTH()  { return ResolutionConfig::Get().LogicalWidth(); }
inline int LOGICAL_HEIGHT() { return ResolutionConfig::Get().LogicalHeight(); }
inline int LOGICAL_MAX_X()  { return ResolutionConfig::Get().LogicalMaxX(); }
inline int LOGICAL_MAX_Y()  { return ResolutionConfig::Get().LogicalMaxY(); }
#endif

// Pixel format constants
#define PIXELFORMAT_RGB565  1   // 5:6:5 format (R=5, G=6, B=5)
#define PIXELFORMAT_RGB555  2   // 5:5:5 format (R=5, G=5, B=5)
#define PIXELFORMAT_BGR565  3   // 5:6:5 BGR format

// Transparency table sizes
#define TRANS_TABLE_SIZE    64
#define ADD_TABLE_SIZE      510

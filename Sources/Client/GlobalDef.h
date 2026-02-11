#pragma once

/****************************************************************
*		 This client was writen by Diuuude & Snoopy81.			*
*					Based on Cleroth work.						*
*																*
*			V3.51 compatibility by Cleroth						*
*			V3.51 dialogs by Diuuude							*
*			Effects, mobs, Apocalypse, Heldenian				*
*			& finalizing by Snoopy81							*
*			V3.82 Crafting & Angels by Snoopy81					*
*****************************************************************/

/****************************************************************
*	Find here all compilation options							*
*	I removed useless ones and tryed to add some explanations	*
*	( Snoopy81 06/2005 )										*
*****************************************************************/

/*** Put here global data for your server ***/
#include "Version.h"

constexpr const char* NAME_WORLDNAME1 = "WS1";
constexpr const char* DEF_SERVER_IP = "199.187.160.239"; //"127.0.0.1";
constexpr const int DEF_SERVER_PORT = 2500;
constexpr const int DEF_GSERVER_PORT = 9907;

// Resolution-dependent values are now provided by hb::shared::render::ResolutionConfig singleton
// Include ResolutionConfig.h and use hb::shared::render::ResolutionConfig::Get().MethodName()
//
// For backward compatibility, these inline functions provide the same interface
// as the old macros but now return dynamic values based on settings.json

// Define guard to prevent RenderConstants.h from redefining these functions
#define GLOBALDEF_H_RESOLUTION_FUNCTIONS

#include "ResolutionConfig.h"

inline int LOGICAL_WIDTH()      { return hb::shared::render::ResolutionConfig::Get().LogicalWidth(); }
inline int LOGICAL_HEIGHT()     { return hb::shared::render::ResolutionConfig::Get().LogicalHeight(); }
inline int BASE_SCREEN_WIDTH()  { return hb::shared::render::ResolutionConfig::Get().LogicalWidth(); }
inline int BASE_SCREEN_HEIGHT() { return hb::shared::render::ResolutionConfig::Get().LogicalHeight(); }
inline int LOGICAL_MAX_X()      { return hb::shared::render::ResolutionConfig::Get().LogicalMaxX(); }
inline int LOGICAL_MAX_Y()      { return hb::shared::render::ResolutionConfig::Get().LogicalMaxY(); }
inline int VIEW_TILE_WIDTH()    { return hb::shared::render::ResolutionConfig::Get().ViewTileWidth(); }
inline int VIEW_TILE_HEIGHT()   { return hb::shared::render::ResolutionConfig::Get().ViewTileHeight(); }
inline int VIEW_CENTER_TILE_X() { return hb::shared::render::ResolutionConfig::Get().ViewCenterTileX(); }
inline int VIEW_CENTER_TILE_Y() { return hb::shared::render::ResolutionConfig::Get().ViewCenterTileY(); }
inline int ICON_PANEL_WIDTH()   { return hb::shared::render::ResolutionConfig::Get().IconPanelWidth(); }
inline int ICON_PANEL_HEIGHT()  { return hb::shared::render::ResolutionConfig::Get().IconPanelHeight(); }
inline int ICON_PANEL_OFFSET_X(){ return hb::shared::render::ResolutionConfig::Get().IconPanelOffsetX(); }

inline int CHAT_INPUT_X()       { return hb::shared::render::ResolutionConfig::Get().ChatInputX(); }
inline int CHAT_INPUT_Y()       { return hb::shared::render::ResolutionConfig::Get().ChatInputY(); }
inline int EVENTLIST2_BASE_Y()  { return hb::shared::render::ResolutionConfig::Get().EventList2BaseY(); }
inline int LEVELUP_TEXT_X()     { return hb::shared::render::ResolutionConfig::Get().LevelUpTextX(); }
inline int LEVELUP_TEXT_Y()     { return hb::shared::render::ResolutionConfig::Get().LevelUpTextY(); }


/*** Some more compilation options ***/
#define DEF_COMMA_GOLD			// Allows to show comma nbe as original HB (ie: 1,200,000)

#define DEF_WINDOWED_MODE		// Shows HB in a windowed mode (for debug purpose only, sprite will bug....)

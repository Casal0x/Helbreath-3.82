// MapData.h: interface for the CMapData class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include <cstdio>
#include <stdlib.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "CommonTypes.h"
#include "Tile.h"
#include "ActionID.h"
#include "Game.h"
#include "TileSpr.h"
#include "ActionID_Client.h"
#include "ObjectIDRange.h"

#define MAPDATASIZEX	60 // 30
#define MAPDATASIZEY	55 // 25

class CMapData
{
public:
	CMapData(class CGame * pGame);
	virtual ~CMapData();
	void Init();
	void OpenMapDataFile(char * cFn);
	void GetOwnerStatusByObjectID(uint16_t wObjectID, char * pOwnerType, char * pDir, PlayerAppearance * pAppearance, PlayerStatus * pStatus, char * pName);
	void ClearDeadChatMsg(short sX, short sY);
	void ClearChatMsg(short sX, short sY);
	void ShiftMapData(char cDir);
	void _bDecodeMapInfo(char * pHeader);
	bool __fastcall bSetChatMsgOwner(uint16_t wObjectID, short sX, short sY, int iIndex);
	bool __fastcall bSetDeadOwner(uint16_t wObjectID, short sX, short sY, short sType, char cDir, const PlayerAppearance& appearance, const PlayerStatus& status, char * pName, short npcConfigId = -1);
	bool __fastcall bSetOwner(uint16_t wObjectID, int sX, int sY, int sType, int cDir, const PlayerAppearance& appearance, const PlayerStatus& status, char * pName, short sAction, short sV1, short sV2, short sV3, int iPreLoc = 0, int iFrame = 0, short npcConfigId = -1);
	bool __fastcall bGetOwner(short sX, short sY, char * pName, short * pOwnerType, PlayerStatus * pOwnerStatus, uint16_t * pObjectID);
	bool bSetDynamicObject(short sX, short sY, uint16_t wID, short sType, bool bIsEvent);
	bool bIsTeleportLoc(short sX, short sY);
	bool bGetIsLocateable(short sX, short sY);
	bool bSetItem(short sX, short sY, short sIDnum, char cItemColor, uint32_t dwItemAttr, bool bDropEffect = true);
	int  iObjectFrameCounter(char * cPlayerName, short sViewPointX, short sViewPointY);

	class CTile m_pData[MAPDATASIZEX][MAPDATASIZEY];
	class CTile m_pTmpData[MAPDATASIZEX][MAPDATASIZEY];
	class CTileSpr m_tile[752][752];
	class CGame * m_pGame;

	struct {
		short m_sMaxFrame;
		short m_sFrameTime;
	} m_stFrame[DEF_TOTALCHARACTERS][DEF_TOTALACTION];
	uint32_t m_dwFrameTime;
	uint32_t m_dwDOframeTime;
	uint32_t m_dwFrameCheckTime;
	int m_iObjectIDcacheLocX[hb::objectid::NpcMax];
	int m_iObjectIDcacheLocY[hb::objectid::NpcMax];
	uint32_t m_dwFrameAdjustTime;
	short m_sMapSizeX, m_sMapSizeY;
	short m_sRectX, m_sRectY;
	short m_sPivotX, m_sPivotY;
};

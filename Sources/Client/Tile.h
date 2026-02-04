// Tile.h: interface for the CTile class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include <windows.h>
#include <mmsystem.h>
#include "EntityMotion.h"
#include "AppearanceData.h"

class CTile
{
public:
	inline void Clear()
	{
		m_wObjectID     = 0;
		m_wDeadObjectID = 0;

		m_sOwnerType = 0;
		ZeroMemory(m_cOwnerName, sizeof(m_cOwnerName));

		m_sDeadOwnerType = 0;
		ZeroMemory(m_cDeadOwnerName, sizeof(m_cDeadOwnerName));

		m_cDeadOwnerFrame = -1;
		m_dwDeadOwnerTime = 0;

		m_cOwnerAction = 0;
		m_cDir         = 0;
		m_cOwnerFrame  = 0;

		m_sItemID = 0;
		m_dwItemAttr = 0;
		m_cItemColor       = 0;

		m_sDynamicObjectType  = 0;
		m_cDynamicObjectFrame = 0;

		m_iChatMsg     = 0;
		m_iDeadChatMsg = 0;

		m_iStatus      = 0;
		m_iDeadStatus  = 0;

		m_sV1 = 0;
		m_sV2 = 0;
		m_sV3 = 0;

		m_iEffectType  = 0;
		m_iEffectFrame = 0;
		m_iEffectTotalFrame = 0;
		m_dwEffectTime = 0;

		m_dwOwnerTime        = 0;

		m_appearance.Clear();
		m_deadAppearance.Clear();

		m_motion.Reset();
	}

	inline CTile()
	{
		m_sOwnerType = 0;
		ZeroMemory(m_cOwnerName, sizeof(m_cOwnerName));
		m_sDeadOwnerType = 0;
		ZeroMemory(m_cDeadOwnerName, sizeof(m_cDeadOwnerName));
		m_cDeadOwnerFrame     = -1;

		m_sDynamicObjectType  = 0;
		m_cDynamicObjectFrame = 0;

		m_iChatMsg       = 0;
		m_iDeadChatMsg   = 0;

		m_wObjectID = 0;

		m_iEffectType  = 0;
		m_iEffectFrame = 0;
		m_iEffectTotalFrame = 0;
		m_dwEffectTime = 0;
	}

	inline virtual ~CTile()
	{
	}	
	DWORD m_dwOwnerTime;
	DWORD m_dwEffectTime;
	DWORD m_dwDeadOwnerTime;
	DWORD m_dwDynamicObjectTime;
	
	int   m_iChatMsg;
	int   m_cItemColor; // v1.4
	int   m_iEffectType;
	int   m_iEffectFrame, m_iEffectTotalFrame;
	int   m_iDeadChatMsg;

	WORD  m_wDeadObjectID;
	WORD  m_wObjectID;

	short m_sOwnerType;							// +B2C
	int m_iStatus;								// +B38

	short m_sDeadOwnerType;						// +B3C
	
	int m_iDeadStatus;
	short m_sV1;
	short m_sV2;					
	short m_sV3;								// +B50
	short m_sDynamicObjectType;

	short m_sItemID;
	DWORD m_dwItemAttr;

	char  m_cDeadOwnerFrame;
	char  m_cOwnerAction;						// +B59
	char  m_cOwnerFrame;						// +B5A
	char  m_cDir;
	char  m_cDeadDir;
	
	char  m_cDynamicObjectFrame;
	char  m_cDynamicObjectData1, m_cDynamicObjectData2, m_cDynamicObjectData3, m_cDynamicObjectData4;
	char  m_cOwnerName[12];
	char  m_cDeadOwnerName[12];

	// Unpacked appearance data (extracted from m_sAppr1-4 at reception)
	PlayerAppearance m_appearance;
	PlayerAppearance m_deadAppearance;

	// Smooth movement interpolation
	EntityMotion m_motion;
};

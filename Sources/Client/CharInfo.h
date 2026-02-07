// CharInfo.h: interface for the CCharInfo class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include <windows.h>
#include "Appearance.h"

class CCharInfo
{
public:
	inline CCharInfo()
	{
		ZeroMemory(m_cName, sizeof(m_cName));
		ZeroMemory(m_cMapName, sizeof(m_cMapName));
		m_iYear   = 0;
		m_iMonth  = 0;
		m_iDay    = 0;
		m_iHour   = 0;
		m_iMinute = 0;
	}

	inline virtual ~CCharInfo()
	{
	}

	char m_cName[12], m_cMapName[12];
	short m_sSkinCol, m_sSex;
	PlayerAppearance m_appearance;
	short	m_sStr, m_sVit, m_sDex, m_sInt, m_sMag, m_sChr;
	int	m_sLevel;
//	short m_sLevel;
	DWORD   m_iExp;
	int   m_iYear, m_iMonth, m_iDay, m_iHour, m_iMinute;
};

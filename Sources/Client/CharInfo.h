// CharInfo.h: interface for the CCharInfo class.
//
//////////////////////////////////////////////////////////////////////

#pragma once
#include <string>

#include <windows.h>
#include "Appearance.h"

class CCharInfo
{
public:
	inline CCharInfo() = default;

	inline virtual ~CCharInfo()
	{
	}

	std::string m_cName;
	std::string m_cMapName;
	short m_sSkinCol = 0, m_sSex = 0;
	hb::shared::entity::PlayerAppearance m_appearance{};
	short	m_sStr = 0, m_sVit = 0, m_sDex = 0, m_sInt = 0, m_sMag = 0, m_sChr = 0;
	int	m_sLevel = 0;
	DWORD   m_iExp = 0;
	int   m_iYear = 0, m_iMonth = 0, m_iDay = 0, m_iHour = 0, m_iMinute = 0;
};

// GuildsMan.h: interface for the CGuildsMan class.
//
//////////////////////////////////////////////////////////////////////

#pragma once


#include "CommonTypes.h"

class CGuildsMan
{
public:

	inline CGuildsMan()
	{
		std::memset(m_cName, 0, sizeof(m_cName));
	}

	inline virtual ~CGuildsMan()
	{
	}

	char m_cName[hb::shared::limits::CharNameLen];
	int  m_iRank;
};

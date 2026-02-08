// GuildsMan.h: interface for the CGuildsMan class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

// MODERNIZED: Prevent old winsock.h from loading (must be before windows.h)

#include <windows.h>
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

	char m_cName[DEF_CHARNAME];
	int  m_iRank;
};

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
		std::memset(m_name, 0, sizeof(m_name));
	}

	inline virtual ~CGuildsMan()
	{
	}

	char m_name[hb::shared::limits::CharNameLen];
	int  m_rank;
};

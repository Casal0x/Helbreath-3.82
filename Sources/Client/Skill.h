// Skill.h: interface for the CSkill class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include <cstring>

class CSkill
{
public:
	inline CSkill()
	{
		std::memset(m_cName, 0, sizeof(m_cName));

		m_iLevel = 0;
		m_bIsUseable = false;
		m_cUseMethod = 0;
	}

	inline virtual ~CSkill()
	{
	}

	char m_cName[42];

	int  m_iLevel;
	bool m_bIsUseable;
	char m_cUseMethod;
};

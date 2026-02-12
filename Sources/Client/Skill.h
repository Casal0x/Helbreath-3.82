// Skill.h: interface for the CSkill class.
//
//////////////////////////////////////////////////////////////////////

#pragma once
#include <string>

#include <cstring>

class CSkill
{
public:
	inline CSkill()
	{

		m_iLevel = 0;
		m_bIsUseable = false;
		m_cUseMethod = 0;
	}

	inline virtual ~CSkill()
	{
	}

	std::string m_cName;

	int  m_iLevel;
	bool m_bIsUseable;
	char m_cUseMethod;
};

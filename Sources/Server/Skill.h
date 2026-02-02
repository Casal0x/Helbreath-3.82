// Skill.h: interface for the CSkill class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

// MODERNIZED: Prevent old winsock.h from loading (must be before windows.h)

#include <windows.h>
#include "CommonTypes.h"



#define DEF_SKILLEFFECTTYPE_GET			    1		// ��´�. �����⳪ ������ 
#define DEF_SKILLEFFECTTYPE_PRETEND			2		// ...�� ü �ϴ� 
#define DEF_SKILLEFFECTTYPE_TAMING			3		//  ����̱� �迭	


class CSkill
{
public:
	inline CSkill()
	{
		std::memset(m_cName, 0, sizeof(m_cName));
		m_bIsUseable = false;
		m_cUseMethod = 0;
	}

	inline virtual ~CSkill()
	{
	}

	char m_cName[42];

	short m_sType;
	short m_sValue1, m_sValue2, m_sValue3, m_sValue4, m_sValue5, m_sValue6;

	// Client display fields (sent via PacketSkillConfig)
	bool  m_bIsUseable;    // Whether skill can be actively used
	char  m_cUseMethod;    // Use method (0=passive, 1=click, 2=target)
};

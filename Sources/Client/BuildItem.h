// BuildItem.h: interface for the build_item class.
//
//////////////////////////////////////////////////////////////////////

#pragma once
#include <string>

#include "CommonTypes.h"
#include "NetConstants.h"

class build_item
{
public:
	inline build_item()
	{
		int i;

		m_build_enabled = false;
		m_skill_limit   = 0;
		m_max_skill     = 0;

		m_sprite_handle     = 0;
		m_sprite_frame = 0;

		for (i = 0; i < 7; i++) {
			m_element_count[i] = 0;
			m_element_flag[i]  = 0;
		}
	}

	inline virtual ~build_item()
	{

	}

	bool m_build_enabled;
	std::string m_name;
	int	 m_skill_limit;
	int  m_max_skill;
	int  m_sprite_handle, m_sprite_frame;
	std::string m_element_name_1;
	std::string m_element_name_2;
	std::string m_element_name_3;
	std::string m_element_name_4;
	std::string m_element_name_5;
	std::string m_element_name_6;
	uint32_t m_element_count[7];
	bool  m_element_flag[7];

};

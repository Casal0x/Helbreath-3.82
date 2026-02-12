// Magic.h: interface for the CMagic class.
//
//////////////////////////////////////////////////////////////////////

#pragma once
#include <string>

#include <cstring>
#include "MagicTypes.h"

class CMagic
{
public:
	inline CMagic()
	{
	}

	inline virtual ~CMagic()
	{
	}

	std::string m_cName;
	int  m_sValue1, m_sValue2, m_sValue3;
	// CLEROTH
	int m_sValue4;
	bool m_bIsVisible;
	int m_sType = 0;          // DEF_MAGICTYPE_*
	int m_sAoERadiusX = 0;    // AoE X radius in tiles (server m_sValue2)
	int m_sAoERadiusY = 0;    // AoE Y radius in tiles (server m_sValue3)
	int m_sDynamicPattern = 0; // Wall=1, Field=2 (server m_sValue11)
	int m_sDynamicRadius = 0;  // Wall length / field radius (server m_sValue12)
};

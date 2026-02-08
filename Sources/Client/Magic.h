// Magic.h: interface for the CMagic class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include <cstring>

#define DEF_MAGICTYPE_DAMAGE_SPOT		1
#define DEF_MAGICTYPE_HPUP_SPOT			2
#define DEF_MAGICTYPE_DAMAGE_AREA		3
#define DEF_MAGICTYPE_SPDOWN_SPOT		4
#define DEF_MAGICTYPE_SPDOWN_AREA		5
#define DEF_MAGICTYPE_SPUP_SPOT			6
#define DEF_MAGICTYPE_SPUP_AREA			7
#define DEF_MAGICTYPE_TELEPORT			8
#define DEF_MAGICTYPE_SUMMON			9 
#define DEF_MAGICTYPE_CREATE			10
#define DEF_MAGICTYPE_PROTECT			11
#define DEF_MAGICTYPE_HOLDOBJECT		12
#define DEF_MAGICTYPE_INVISIBILITY		13
#define DEF_MAGICTYPE_CREATE_DYNAMIC	14
#define DEF_MAGICTYPE_POSSESSION		15 
#define DEF_MAGICTYPE_CONFUSE			16 
#define DEF_MAGICTYPE_POISON			17 
#define DEF_MAGICTYPE_BERSERK			18
#define DEF_MAGICTYPE_DAMAGE_LINEAR		19
#define DEF_MAGICTYPE_POLYMORPH			20
#define DEF_MAGICTYPE_DAMAGE_AREA_NOSPOT	21
#define DEF_MAGICTYPE_TREMOR				22
#define DEF_MAGICTYPE_ICE					23
#define DEF_MAGICTYPE_DAMAGE_AREA_NOSPOT_SPDOWN	25
#define DEF_MAGICTYPE_ICE_LINEAR			26
#define DEF_MAGICTYPE_DAMAGE_AREA_ARMOR_BREAK	28
#define DEF_MAGICTYPE_CANCELLATION			29
#define DEF_MAGICTYPE_DAMAGE_LINEAR_SPDOWN	30
#define DEF_MAGICTYPE_INHIBITION			31
#define DEF_MAGICTYPE_RESURRECTION			32
#define DEF_MAGICTYPE_SCAN					33
#define DEF_MAGICTYPE_HASTE				45

class CMagic
{
public:
	inline CMagic()
	{
		std::memset(m_cName, 0, sizeof(m_cName));
	}

	inline virtual ~CMagic()
	{
	}

	char m_cName[31];
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

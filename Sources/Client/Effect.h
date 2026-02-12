// Effect.h: interface for the CEffect class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include <windows.h>
#include "CommonTypes.h"
#include "EffectType.h"

class CEffect
{
public:
	inline CEffect() = default;

	inline virtual ~CEffect()
	{

	}

	EffectType m_sType = EffectType::INVALID;
	char  m_cFrame = -1, m_cMaxFrame = 0;
	char  m_cDir = 0;
	uint32_t m_dwTime = 0, m_dwFrameTime = 0;
	int   m_sX = 0, m_sY = 0, m_dX = 0, m_dY = 0;
	int   m_mX = 0, m_mY = 0, m_mX2 = 0, m_mY2 = 0, m_mX3 = 0, m_mY3 = 0;
	int   m_iErr = 0;
	int   m_rX = 0, m_rY = 0;
	int   m_iV1 = 0;
};

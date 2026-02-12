// GameMonitor.h: interface for the CGameMonitor class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include <cstring>
#include "ChatMsg.h"

namespace hb::client::config { constexpr int MaxBadWord = 500; }

class CGameMonitor
{
public:
	bool bCheckBadWord(const char * pWord);
	int iReadBadWordFileList(const char * pFn);
	CGameMonitor();
	~CGameMonitor();

	class CMsg * m_pWordList[hb::client::config::MaxBadWord];

};

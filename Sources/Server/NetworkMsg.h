// Msg.h: interface for the CMsg class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

// MODERNIZED: Prevent old winsock.h from loading (must be before windows.h)
#include <windows.h>
#include "CommonTypes.h"

#define DEF_MSGFROM_CLIENT		1
#define DEF_MSGFROM_LOGSERVER	2
#define DEF_MSGFROM_BOT			4

class CMsg  								 
{
public:
	void Get(char * pFrom, char * pData, size_t* pSize, int * pIndex, char * pKey);
	bool bPut(char cFrom, char * pData, size_t dwSize, int iIndex, char cKey);
	CMsg();
	virtual ~CMsg();

	char   m_cFrom;

	char * m_pData;
	size_t  m_dwSize;

	int    m_iIndex;
	char   m_cKey;   // v1.4
};

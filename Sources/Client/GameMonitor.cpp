// GameMonitor.cpp: implementation of the CGameMonitor class.
//
//////////////////////////////////////////////////////////////////////

#include "GameMonitor.h"
#include "CommonTypes.h"
#include <fstream>
#include <string>
#include <cstring>
using namespace hb::client::config;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGameMonitor::CGameMonitor()
{
	for (int i = 0; i < MaxBadWord; i++)
		m_pWordList[i] = 0;
}

CGameMonitor::~CGameMonitor()
{
	for (int i = 0; i < MaxBadWord; i++)
		if (m_pWordList[i] != 0) delete m_pWordList[i];
}

int CGameMonitor::iReadBadWordFileList(const char* pFn)
{
	// BUG-19: Clear existing entries before reload to prevent memory leak
	for (int i = 0; i < MaxBadWord; i++)
	{
		if (m_pWordList[i] != 0)
		{
			delete m_pWordList[i];
			m_pWordList[i] = 0;
		}
	}

	// BUG-23/BUG-03: Use single std::ifstream instead of dual CreateFile/fopen
	std::ifstream file(pFn);
	if (!file.is_open()) return 0;

	std::string contents((std::istreambuf_iterator<char>(file)),
		std::istreambuf_iterator<char>());
	file.close();

	int iIndex = 0;
	char* pContents = contents.data();
	char seps[] = "/,\t\n";
	char* token = strtok(pContents, seps);
	while (token != 0)
	{
		m_pWordList[iIndex] = new class CMsg(0, token, 0);
		iIndex++;
		if (iIndex >= MaxBadWord) break;
		token = strtok(NULL, seps);
	}
	return iIndex;
}

bool CGameMonitor::bCheckBadWord(const char* pWord)
{
#ifndef _DEBUG
	// BUG-04: Use strncpy to prevent buffer overflow
	char cBuffer[500];
	std::memset(cBuffer, 0, sizeof(cBuffer));
	strncpy(cBuffer, pWord, sizeof(cBuffer) - 1);

	// BUG-10: Add i < MaxBadWord to prevent out-of-bounds access
	int i = 0;
	while ((i < MaxBadWord) && (m_pWordList[i] != 0) && (strlen(m_pWordList[i]->m_pMsg) != 0))
	{
		if (memcmp(cBuffer, m_pWordList[i]->m_pMsg, strlen(m_pWordList[i]->m_pMsg)) == 0)
		{
			return true;
		}
		i++;
	}
	return false;
#else
	return false;
#endif
}

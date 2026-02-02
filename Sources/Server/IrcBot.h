#pragma once
#include "CommonTypes.h"

class CIrcBot
{
public:
	CIrcBot(void);
	~CIrcBot(void);
	void Startup(char* cServerAddr, short sServerPort, HWND hWnd);
	class ASIOSocket*m_pIrcSocket;
	void Shutdown(void);
	void SendTestMessage(void);
};

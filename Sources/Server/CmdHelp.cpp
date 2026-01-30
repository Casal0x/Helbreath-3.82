#define _WINSOCKAPI_
#include <windows.h>
#include "CmdHelp.h"
#include "winmain.h"
#include <cstdio>

void CmdHelp::Execute(CGame* pGame, const char* pArgs)
{
	PutLogList((char*)"Hello world");
}

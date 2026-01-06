#include "CommonTypes.h"
#include "LoginServer.h"
#ifdef _WIN32
#include <direct.h>
#endif
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
using namespace std;

#include "AccountSqliteStore.h"

extern char	G_cData50000[50000];
//extern void PutLogList(char* cMsg);
extern char G_cTxt[512];
extern class CGame* G_pGame;

#define WORLDNAMELS   "WS1"
#define WORLDNAMELS2   "WS2"

LoginServer::LoginServer()
{

}

LoginServer::~LoginServer()
{

}

bool IsValidName(char* pStr)
{
	int i, iLen;
	iLen = strlen(pStr);
	for (i = 0; i < iLen; i++)
	{
		//if (pStr[i] < 0)	return false;
		if ((pStr[i] == ',') || (pStr[i] == '=') || (pStr[i] == ' ') || (pStr[i] == '\n') ||
			(pStr[i] == '\t') || (pStr[i] == '.') || (pStr[i] == '\\') || (pStr[i] == '/') ||
			(pStr[i] == ':') || (pStr[i] == '*') || (pStr[i] == '?') || (pStr[i] == '<') ||
			(pStr[i] == '>') || (pStr[i] == '|') || (pStr[i] == '"') || (pStr[i] == '`') ||
			(pStr[i] == ';') || (pStr[i] == '=') || (pStr[i] == '@') || (pStr[i] == '[') ||
			(pStr[i] == ']') || (pStr[i] == '^') || (pStr[i] == '_') || (pStr[i] == '\'')) return false;
		//if ((pStr[i] < '0') || (pStr[i] > 'z')) return false;
	}
	return true;
}

void FormatTimestamp(const SYSTEMTIME& sysTime, char* outBuffer, size_t outBufferSize)
{
	std::snprintf(outBuffer, outBufferSize, "%04d-%02d-%02d %02d:%02d:%02d",
		sysTime.wYear, sysTime.wMonth, sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond);
}

void LoginServer::Activated()
{
	
}

void LoginServer::RequestLogin(int h, char* pData)
{
	if (G_pGame->_lclients[h] == 0)
		return;

	char cName[11] = {};
	char cPassword[11] = {};
	char world_name[32] = {};

	auto cp = (char*)(pData + DEF_INDEX2_MSGTYPE + 2);
	Pop(cp, cName, 10);
	Pop(cp, cPassword, 10);
	Pop(cp, world_name, 30);

	if (string(world_name) != WORLDNAMELS)
		return;

	if (!IsValidName(cName) || !IsValidName(cPassword) || !IsValidName(world_name))
		return;

	std::snprintf(G_cTxt, sizeof(G_cTxt), "(!) Account Request Login: %s", cName);
	PutLogList(G_cTxt);

	std::vector<string> chars;
	auto status = AccountLogIn(cName, cPassword, chars);
	switch (status)
	{
	case LogIn::Ok:
	{
		char pData[512] = {};
		char* cp2 = pData;
		Push(cp2, (int)chars.size());
		GetCharList(cName, cp2, chars);
		SendLoginMsg(DEF_LOGRESMSGTYPE_CONFIRM, DEF_LOGRESMSGTYPE_CONFIRM, pData, cp2 - pData, h);
		//PutLogList("Ok");
		break;
	}

	case LogIn::NoAcc:
		SendLoginMsg(DEF_LOGRESMSGTYPE_NOTEXISTINGACCOUNT, DEF_LOGRESMSGTYPE_NOTEXISTINGACCOUNT, 0, 0, h);
		PutLogList("No Acc");
		break;

	case LogIn::NoPass:
		SendLoginMsg(DEF_LOGRESMSGTYPE_PASSWORDMISMATCH, DEF_LOGRESMSGTYPE_PASSWORDMISMATCH, 0, 0, h);
		PutLogList("No Pass");
		break;
	}
}

void LoginServer::GetCharList(string acc, char*& cp2, std::vector<string> chars)
{
	for (size_t i = 0; i < chars.size(); i++)
	{
		char seps[] = "= \t\r\n";
		char cFileName[112] = {};
		char cDir[112] = {};
#ifdef _WIN32
		_mkdir("Characters");
#endif
		std::memset(cFileName, 0, sizeof(cFileName));
		strcat(cFileName, "Characters");
		strcat(cFileName, "\\");
		strcat(cFileName, "\\");
		std::snprintf(cDir, sizeof(cDir), "AscII%d", chars[i][0]);
		strcat(cFileName, cDir);
		strcat(cFileName, "\\");
		strcat(cFileName, "\\");
		strcat(cFileName, (char*)chars[i].c_str());
		strcat(cFileName, ".txt");

		DWORD lpNumberOfBytesRead;
		HANDLE  hFile = CreateFile(cFileName, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			CloseHandle(hFile);
			return;
		}
		auto dwFileSize = GetFileSize(hFile, 0);
		auto cp = new char[dwFileSize + 1];
		std::memset(cp, 0, dwFileSize + 1);
		ReadFile(hFile, cp, dwFileSize, &lpNumberOfBytesRead, 0);
		CloseHandle(hFile);
		int cReadModeA = 0;
		char* token = strtok(cp, seps);

		short sAppr1 = 0, sAppr2 = 0, sAppr3 = 0, sAppr4 = 0;
		uint16_t cSex = 0, cSkin = 0;
		uint16_t iLevel = 0;
		uint32_t iExp = 0;
		uint32_t iApprColor = 0; // Antes ten�as u64, pero el cliente espera uint32_t (4 bytes)
		char cMapName[11] = {}; // Siempre tama�o 11 para forzar terminador

		while (token != 0)
		{
			if (cReadModeA != 0)
			{
				switch (cReadModeA)
				{
				case 1: sAppr1 = atoi(token); cReadModeA = 0; break;
				case 2: sAppr2 = atoi(token); cReadModeA = 0; break;
				case 3: sAppr3 = atoi(token); cReadModeA = 0; break;
				case 4: sAppr4 = atoi(token); cReadModeA = 0; break;
				case 5: cSex = atoi(token); cReadModeA = 0; break;
				case 6: cSkin = atoi(token); cReadModeA = 0; break;
				case 7: iLevel = atoi(token); cReadModeA = 0; break;
				case 14: iExp = atoi(token); cReadModeA = 0; break;
				case 15: iApprColor = (uint32_t)strtoul(token, nullptr, 10); cReadModeA = 0; break;
				case 17: std::memset(cMapName, 0, sizeof(cMapName)); strncpy(cMapName, token, 10); cMapName[10] = 0; cReadModeA = 0; break;
				}
			}
			else {
				if (memcmp(token, "appr1", 5) == 0) cReadModeA = 1;
				if (memcmp(token, "appr2", 5) == 0) cReadModeA = 2;
				if (memcmp(token, "appr3", 5) == 0) cReadModeA = 3;
				if (memcmp(token, "appr4", 5) == 0) cReadModeA = 4;
				if (memcmp(token, "sex-status", 10) == 0) cReadModeA = 5;
				if (memcmp(token, "skin-status", 11) == 0) cReadModeA = 6;
				if (memcmp(token, "character-LEVEL", 15) == 0) cReadModeA = 7;
				if (memcmp(token, "character-EXP", 13) == 0) cReadModeA = 14;
				if (memcmp(token, "appr-color-new", 14) == 0) cReadModeA = 15;
				if (memcmp(token, "character-loc-map", 17) == 0) cReadModeA = 17;
			}
			token = strtok(NULL, seps);
		}

		delete[] cp;

		// Nombre
		char cName[11] = {};
		std::memset(cName, 0, sizeof(cName));
		strncpy(cName, chars[i].c_str(), 10); // Solo 10 bytes
		cName[10] = 0;

		Push(cp2, cName, 10);          // <- SOLO 10 bytes, sin null extra
		Push(cp2, sAppr1);             // short
		Push(cp2, sAppr2);
		Push(cp2, sAppr3);
		Push(cp2, sAppr4);
		Push(cp2, cSex);               // uint16_t
		Push(cp2, cSkin);              // uint16_t
		Push(cp2, iLevel);             // uint16_t
		Push(cp2, iExp);               // uint32_t
		Push(cp2, iApprColor);         // uint32_t
		Push(cp2, cMapName, 10);       // SOLO 10 BYTES del mapname, igual que nombre
	}
}


LogIn LoginServer::AccountLogIn(string acc, string pass, std::vector<string>& chars)
{
	char file_name[255], cDir[63], cTxt[50];
	char* cp, * token, cReadModeA, cReadModeB;
	char seps[] = "= \t\r\n";

	cReadModeA = cReadModeB = 0;

	std::memset(cTxt, 0, sizeof(cTxt));
	std::snprintf(cTxt, sizeof(cTxt), "account-character");

	if (acc.size() == 0)
		return LogIn::NoAcc;

	if (pass.size() == 0)
		return LogIn::NoPass;

	std::memset(file_name, 0, sizeof(file_name));
	std::memset(cDir, 0, sizeof(cDir));
	strcat(file_name, "Accounts");
	strcat(file_name, "\\");
	strcat(file_name, "\\");
	std::snprintf(cDir, sizeof(cDir), "AscII%d", acc[0]);
	strcat(file_name, cDir);
	strcat(file_name, "\\");
	strcat(file_name, "\\");
	strcat(file_name, (char*)acc.c_str());
	strcat(file_name, ".txt");


	HANDLE file_handle = CreateFile(file_name, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
	uint32_t file_sz = 0;
	if (file_handle == INVALID_HANDLE_VALUE)
	{
		CloseHandle(file_handle);
		std::snprintf(G_cTxt, sizeof(G_cTxt), "(!) Account Does not Exist [invalid handle] (%s)", file_name);
		PutLogList(G_cTxt);
		return LogIn::NoAcc;
	}

	file_sz = GetFileSize(file_handle, 0);
	CloseHandle(file_handle);

	if (file_sz == 0)
	{
		std::snprintf(G_cTxt, sizeof(G_cTxt), "(!) Login File sz = 0 (%s)", file_name);
		PutLogList(G_cTxt);
		return LogIn::NoAcc;
	}

	auto file = fopen(file_name, "rt");
	if (file == 0)
	{
		std::snprintf(G_cTxt, sizeof(G_cTxt), "(!) Account Does not Exist (%s)", file_name);
		PutLogList(G_cTxt);
		return LogIn::NoAcc;
	}

	struct Acc
	{
		string name;
		string pass;
	} account;

	cp = new char[file_sz + 1];
	std::memset(cp, 0, file_sz + 1);
	fread(cp, file_sz, 1, file);
	token = strtok(cp, seps);
	while (token != 0)
	{
		if (cReadModeA != 0)
		{
			switch (cReadModeA)
			{
			case 1:
				if (strlen(token) <= 0)
				{

					delete[] cp;

					fclose(file);
					return LogIn::NoAcc;
				}

				if (acc != token)
				{
					PutLogList("(!) Wrong Acc name");

					delete[] cp;

					fclose(file);
					return LogIn::NoAcc;
				}
				cReadModeA = 0;
				cReadModeA = 0;
				break;
			case 2:
				if (strlen(token) <= 0)
				{
					delete[] cp;
					return LogIn::NoAcc;
				}
				if (pass != token)
				{
					PutLogList("(!) Wrong pass");

					delete[] cp;

					fclose(file);
					return LogIn::NoPass;

				}
				cReadModeA = 0;
				break;

			case 3:
			{
				string tok = token;
				chars.push_back(tok);
				if (chars.size() > 4)
				{
					PutLogList("(!) Charlist exceeds 4 in acc cfg!");

					delete[] cp;

					fclose(file);
					return LogIn::NoAcc;
				}
				cReadModeA = 0;
				break;
			}
			}
		}
		else {
			if (memcmp(token, "account-name", 12) == 0)
				cReadModeA = 1;

			if (memcmp(token, "account-password", 16) == 0)
				cReadModeA = 2;

			if (memcmp(token, cTxt, strlen(cTxt)) == 0)
				cReadModeA = 3;
		}
		token = strtok(NULL, seps);
	}

	delete[] cp;

	fclose(file);

	PutLogList("Account Login!");
	return LogIn::Ok;
}

void LoginServer::ResponseCharacter(int h, char* pData)
{

	char cName[11] = {};
	char cAcc[11] = {};
	char cPassword[11] = {};
	char world_name[32] = {};

	char gender, skin, hairstyle, haircolor, under, str, vit, dex, intl, mag, chr;

	auto cp = (char*)(pData + DEF_INDEX2_MSGTYPE + 2);
	Pop(cp, cName, 10);
	Pop(cp, cAcc, 10);
	Pop(cp, cPassword, 10);
	Pop(cp, world_name, 30);
	Pop(cp, gender);
	Pop(cp, skin);
	Pop(cp, hairstyle);
	Pop(cp, haircolor);
	Pop(cp, under);
	Pop(cp, str);
	Pop(cp, vit);
	Pop(cp, dex);
	Pop(cp, intl);
	Pop(cp, mag);
	Pop(cp, chr);

	if (string(world_name) != WORLDNAMELS)
		return;

	std::snprintf(G_cTxt, sizeof(G_cTxt), "(!) Request create new Character: %s", cName);
	PutLogList(G_cTxt);

	std::vector<string> chars;
	auto status = AccountLogIn(cAcc, cPassword, chars);
	if (status != LogIn::Ok)
		return;

	if (chars.size() >= 4)
		return;

	if (!IsValidName(cAcc) || !IsValidName(cPassword) || !IsValidName(cName))
		return;

	char seps[] = "= \t\r\n";
	char cFileName[112] = {};
	char cDir[112] = {};

	std::memset(cFileName, 0, sizeof(cFileName));
	strcat(cFileName, "Characters");
	strcat(cFileName, "\\");
	strcat(cFileName, "\\");
	std::snprintf(cDir, sizeof(cDir), "AscII%d", cName[0]);
	strcat(cFileName, cDir);
	strcat(cFileName, "\\");
	strcat(cFileName, "\\");
	strcat(cFileName, cName);
	strcat(cFileName, ".txt");
	//std::snprintf(G_cTxt, sizeof(G_cTxt), "(!) Getting character file %s", cFileName);
	//PutLogList(G_cTxt);

	HANDLE  hFile = CreateFile(cFileName, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
	auto dwFileSize = GetFileSize(hFile, 0);

	CloseHandle(hFile);

	if (hFile != INVALID_HANDLE_VALUE)
	{
		SendLoginMsg(DEF_LOGRESMSGTYPE_NEWCHARACTERFAILED, DEF_LOGRESMSGTYPE_NEWCHARACTERFAILED, 0, 0, h);
		return;
	}

#ifdef _WIN32
	_mkdir("Characters");
	_mkdir(string(string("Characters\\") + cDir).c_str());
#endif
	auto file = fopen(cFileName, "wt");
	if (!file)
	{
		SendLoginMsg(DEF_LOGRESMSGTYPE_NEWCHARACTERFAILED, DEF_LOGRESMSGTYPE_NEWCHARACTERFAILED, 0, 0, h);
		return;
	}

	SYSTEMTIME SysTime;
	GetLocalTime(&SysTime);

	char cBuffer[112] = {};
	char cFile[2048] = {};

	std::memset(cFile, 0, sizeof(cFile));

	strcat(cFile, "[FILE-DATE]");
	strcat(cFile, "\n\n");
	std::memset(cBuffer, 0, sizeof(cBuffer));
	std::snprintf(cBuffer, sizeof(cBuffer), "file-saved-date: %d %d %d %d %d", SysTime.wYear, SysTime.wMonth, SysTime.wDay, SysTime.wHour, SysTime.wMinute);
	strcat(cFile, cBuffer);
	strcat(cFile, "\n\n");

	strcat(cFile, "[NAME-ACCOUNT]");
	strcat(cFile, "\n\n");

	std::memset(cBuffer, 0, sizeof(cBuffer));
	std::snprintf(cBuffer, sizeof(cBuffer), "character-name = %s\n", cName);
	strcat(cFile, cBuffer);

	std::memset(cBuffer, 0, sizeof(cBuffer));
	std::snprintf(cBuffer, sizeof(cBuffer), "account-name = %s\n", cAcc);
	strcat(cFile, cBuffer);

	strcat(cFile, "\n\n");
	strcat(cFile, "[STATUS]\n\n");

	std::memset(cBuffer, 0, sizeof(cBuffer));
	std::snprintf(cBuffer, sizeof(cBuffer), "character-location = NONE\n");
	strcat(cFile, cBuffer);

	std::memset(cBuffer, 0, sizeof(cBuffer));
	std::snprintf(cBuffer, sizeof(cBuffer), "character-guild-name = NONE\n");
	strcat(cFile, cBuffer);

	std::memset(cBuffer, 0, sizeof(cBuffer));
	std::snprintf(cBuffer, sizeof(cBuffer), "character-guild-rank = -1\n");
	strcat(cFile, cBuffer);

	std::memset(cBuffer, 0, sizeof(cBuffer));
	std::snprintf(cBuffer, sizeof(cBuffer), "character-loc-map = default\n");
	strcat(cFile, cBuffer);
	std::memset(cBuffer, 0, sizeof(cBuffer));
	std::snprintf(cBuffer, sizeof(cBuffer), "character-loc-x = -1\n");
	strcat(cFile, cBuffer);
	std::memset(cBuffer, 0, sizeof(cBuffer));
	std::snprintf(cBuffer, sizeof(cBuffer), "character-loc-y = -1\n");
	strcat(cFile, cBuffer);

	int hp = vit * 3 + 1 * 2 + str / 2;
	std::memset(cBuffer, 0, sizeof(cBuffer));
	std::snprintf(cBuffer, sizeof(cBuffer), "character-HP = %d\n", hp);
	strcat(cFile, cBuffer);

	int mp = (mag) * 2 + 1 * 2 + (intl) / 2;
	std::memset(cBuffer, 0, sizeof(cBuffer));
	std::snprintf(cBuffer, sizeof(cBuffer), "character-MP = %d\n", mp);
	strcat(cFile, cBuffer);

	int sp = 1 * 2 + str * 2;
	std::memset(cBuffer, 0, sizeof(cBuffer));
	std::snprintf(cBuffer, sizeof(cBuffer), "character-SP = %d\n", sp);
	strcat(cFile, cBuffer);


	std::memset(cBuffer, 0, sizeof(cBuffer));
	std::snprintf(cBuffer, sizeof(cBuffer), "character-DefenseRatio = %d\n", 14);
	strcat(cFile, cBuffer);

	std::memset(cBuffer, 0, sizeof(cBuffer));
	std::snprintf(cBuffer, sizeof(cBuffer), "character-LEVEL = 1\n");
	strcat(cFile, cBuffer);

	std::memset(cBuffer, 0, sizeof(cBuffer));
	std::snprintf(cBuffer, sizeof(cBuffer), "character-STR = %d\n", str);
	strcat(cFile, cBuffer);

	std::memset(cBuffer, 0, sizeof(cBuffer));
	std::snprintf(cBuffer, sizeof(cBuffer), "character-INT = %d\n", intl);
	strcat(cFile, cBuffer);

	std::memset(cBuffer, 0, sizeof(cBuffer));
	std::snprintf(cBuffer, sizeof(cBuffer), "character-VIT = %d\n", vit);
	strcat(cFile, cBuffer);

	std::memset(cBuffer, 0, sizeof(cBuffer));
	std::snprintf(cBuffer, sizeof(cBuffer), "character-DEX = %d\n", dex);
	strcat(cFile, cBuffer);

	std::memset(cBuffer, 0, sizeof(cBuffer));
	std::snprintf(cBuffer, sizeof(cBuffer), "character-MAG = %d\n", mag);
	strcat(cFile, cBuffer);

	std::memset(cBuffer, 0, sizeof(cBuffer));
	std::snprintf(cBuffer, sizeof(cBuffer), "character-CHARISMA = %d\n", chr);
	strcat(cFile, cBuffer);

	std::memset(cBuffer, 0, sizeof(cBuffer));
	std::snprintf(cBuffer, sizeof(cBuffer), "character-LUCK = %d\n", 10);
	strcat(cFile, cBuffer);

	std::memset(cBuffer, 0, sizeof(cBuffer));
	std::snprintf(cBuffer, sizeof(cBuffer), "character-EXP = %d\n", 0);
	strcat(cFile, cBuffer);

	std::memset(cBuffer, 0, sizeof(cBuffer));
	std::snprintf(cBuffer, sizeof(cBuffer), "character-Lu_Pool = %d\n", 0);
	strcat(cFile, cBuffer);

	std::memset(cBuffer, 0, sizeof(cBuffer));
	std::snprintf(cBuffer, sizeof(cBuffer), "character-EK-Count = %d\n", 0);
	strcat(cFile, cBuffer);

	std::memset(cBuffer, 0, sizeof(cBuffer));
	std::snprintf(cBuffer, sizeof(cBuffer), "character-PK-Count = %d\n", 0);
	strcat(cFile, cBuffer);

	std::memset(cBuffer, 0, sizeof(cBuffer));
	std::snprintf(cBuffer, sizeof(cBuffer), "sex-status = %d\n", gender);
	strcat(cFile, cBuffer);

	std::memset(cBuffer, 0, sizeof(cBuffer));
	std::snprintf(cBuffer, sizeof(cBuffer), "skin-status = %d\n", skin);
	strcat(cFile, cBuffer);

	std::memset(cBuffer, 0, sizeof(cBuffer));
	std::snprintf(cBuffer, sizeof(cBuffer), "hairstyle-status = %d\n", hairstyle);
	strcat(cFile, cBuffer);

	std::memset(cBuffer, 0, sizeof(cBuffer));
	std::snprintf(cBuffer, sizeof(cBuffer), "haircolor-status = %d\n", haircolor);
	strcat(cFile, cBuffer);

	std::memset(cBuffer, 0, sizeof(cBuffer));
	std::snprintf(cBuffer, sizeof(cBuffer), "underwear-status = %d\n", under);
	strcat(cFile, cBuffer);

	std::memset(cBuffer, 0, sizeof(cBuffer));
	std::snprintf(cBuffer, sizeof(cBuffer), "appr1 = %d\n", 1187);
	strcat(cFile, cBuffer);

	std::memset(cBuffer, 0, sizeof(cBuffer));
	std::snprintf(cBuffer, sizeof(cBuffer), "appr2 = %d\n", 0);
	strcat(cFile, cBuffer);

	std::memset(cBuffer, 0, sizeof(cBuffer));
	std::snprintf(cBuffer, sizeof(cBuffer), "appr3 = %d\n", 0);
	strcat(cFile, cBuffer);

	std::memset(cBuffer, 0, sizeof(cBuffer));
	std::snprintf(cBuffer, sizeof(cBuffer), "appr4 = %d\n", 0);
	strcat(cFile, cBuffer);

	strcat(cFile, "\n\n[ITEMLIST]\n\n");

	strcat(pData, "character-item = Dagger               1 0 0 0 0 0 0 0 0 300 0\n");
	strcat(pData, "character-item = RecallScroll         1 0 0 0 0 0 0 0 0 300 0\n");
	strcat(pData, "character-item = BigRedPotion         1 0 0 0 0 0 0 0 0 300 0\n");
	strcat(pData, "character-item = BigGreenPotion	     1 0 0 0 0 0 0 0 0 300 0\n");
	strcat(pData, "character-item = BigBluePotion        1 0 0 0 0 0 0 0 0 300 0\n");
	strcat(pData, "character-item = Map			         1 0 0 0 0 0 0 0 0 300 0\n");

	int iRand = (rand() % 16);
	char cTxt[120];
	switch (iRand) {
	case 1:
	case 2:
	case 3:
	case 4:
	case 7:
	case 8:
	case 15:
		strcat(pData, "character-item = WoodShield           1 0 0 0 0 0 0 0 0 300 0\n");
		break;

	default:
		if (gender == 1) {
			std::snprintf(cTxt, sizeof(cTxt), "character-item = KneeTrousers(M)     1 0 0 0 0 %d 0 0 0 300 0\n", iRand);
		}
		else {
			std::snprintf(cTxt, sizeof(cTxt), "character-item = Chemise(W)	         1 0 0 0 0 %d 0 0 0 300 0\n", iRand);
		}
		strcat(pData, cTxt);
		strcat(pData, "\n");
		break;
	}

	strcat(pData, "[MAGIC-SKILL-MASTERY]\n\n");
	strcat(pData, "magic-mastery     = 0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000");
	strcat(pData, "\n\n");

	strcat(pData, "skill-mastery     = 0 0 0 3 20 24 0 24 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ");
	//	for (i = 0; i < 60; i++) {
			//std::memset(cTxt, 0, sizeof(cTxt));
			//std::snprintf(cTxt, sizeof(cTxt), "0 0 0 3 20 24 0 24 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ");
			//strcat(pData, cTxt);
	//	}
	strcat(pData, "\n");

	strcat(pData, "skill-SSN     = ");
	for (int i = 0; i < 60; i++) {
		strcat(pData, "0 ");
	}
	strcat(pData, "\n\n");

	strcat(pData, "[ITEM-EQUIP-STATUS]\n\n");
	strcat(pData, "item-equip-status = 00000110000000000000000000000000000000000000000000");
	strcat(pData, "\n\n");

	strcat(pData, "[EOF]");
	strcat(pData, "\n\n\n\n");

	fwrite(cFile, 1, strlen(cFile), file);
	fclose(file);

	SaveAccountInfo(0, cAcc, nullptr, cName, 1, h);

	{
		sqlite3* db = nullptr;
		std::string dbPath;
		if (EnsureAccountDatabase(cAcc, &db, dbPath)) {
			AccountDbCharacterData data = {};
			std::snprintf(data.accountName, sizeof(data.accountName), "%s", cAcc);
			std::snprintf(data.characterName, sizeof(data.characterName), "%s", cName);
			FormatTimestamp(SysTime, data.createdAt, sizeof(data.createdAt));
			data.appr1 = 1187;
			data.appr2 = 0;
			data.appr3 = 0;
			data.appr4 = 0;
			data.apprColor = 0;
			data.level = 1;
			data.exp = 0;
			std::snprintf(data.mapName, sizeof(data.mapName), "default");
			data.mapX = -1;
			data.mapY = -1;
			data.hp = hp;
			data.mp = mp;
			data.sp = sp;
			data.str = str;
			data.vit = vit;
			data.dex = dex;
			data.intl = intl;
			data.mag = mag;
			data.chr = chr;
			data.gender = gender;
			data.skin = skin;
			data.hairStyle = hairstyle;
			data.hairColor = haircolor;
			data.underwear = under;
			InsertCharacterRecord(db, data);
			CloseAccountDatabase(db);
		}
	}

	char cData[512] = {};
	char* cp2 = cData;
	Push(cp2, cName, 10);
	chars.push_back(cName);
	Push(cp2, (int)chars.size());
	GetCharList(cAcc, cp2, chars);
	SendLoginMsg(DEF_LOGRESMSGTYPE_NEWCHARACTERCREATED, DEF_LOGRESMSGTYPE_NEWCHARACTERCREATED, cData, cp2 - cData, h);
}

#include <sstream>
void LoginServer::DeleteCharacter(int h, char* pData)
{
	//PutLogList("DeleteCharacter()");

	char cName[11] = {};
	char cAcc[11] = {};
	char cPassword[11] = {};
	char world_name[32] = {};

	auto cp = (char*)(pData + DEF_INDEX2_MSGTYPE + 2);
	Pop(cp, cName, 10);
	Pop(cp, cAcc, 10);
	Pop(cp, cPassword, 10);
	Pop(cp, world_name, 30);

	std::snprintf(G_cTxt, sizeof(G_cTxt), "(!) Request delete Character: %s", cName);
	PutLogList(G_cTxt);

	std::vector<string> chars;
	auto status = AccountLogIn(cAcc, cPassword, chars);
	if (status != LogIn::Ok)
		return;

	if (chars.size() == 0)
		return;

	char cDir[112] = {};
	char cTxt[112] = {};
	char cFileName[112] = {};
	strcat(cFileName, "Characters");
	strcat(cFileName, "\\");
	strcat(cFileName, "\\");
	std::snprintf(cTxt, sizeof(cTxt), "AscII%d", *cName);
	strcat(cFileName, cTxt);
	strcpy(cDir, cFileName);
	strcat(cFileName, "\\");
	strcat(cFileName, "\\");
	strcat(cFileName, cName);
	strcat(cFileName, ".txt");

	DeleteFile(cFileName);


	std::memset(cFileName, 0, sizeof(cFileName));
	std::memset(cDir, 0, sizeof(cDir));
	strcat(cFileName, "Accounts");
	strcat(cFileName, "\\");
	std::snprintf(cDir, sizeof(cDir), "AscII%d", *cAcc);
	strcat(cFileName, cDir);
	strcat(cFileName, "\\");
	strcat(cFileName, cAcc);
	strcat(cFileName, ".txt");

	ifstream in(cFileName, ios::in);
	if (!in.is_open())
	{
		PutLogList("in is_opem = false");
		return;
	}
	string wordToReplace("account-character = ");
	wordToReplace.append(cName);

	string wordToReplace2("account-character = ");
	wordToReplace2.append(cName);
	string wordToReplaceWith("");

	stringstream ss;
	bool done = false;
	string line;
	size_t len = wordToReplace.length();
	while (getline(in, line))
	{
		string repl = wordToReplace;
		size_t pos = line.find(repl);
		len = repl.length();
		if (pos == string::npos)
		{
			repl = wordToReplace2;
			pos = line.find(repl);
			len = repl.length();
		}

		if (pos != string::npos)
		{
			line.replace(pos, len, wordToReplaceWith);
			done = true;
		}

		ss << line << '\n';
	}
	in.close();

	ofstream out(cFileName);
	out << ss.str();


	if (done)
	{
		char cData[512] = {};
		char* cp2 = cData;
		for (auto it = chars.begin(); it != chars.end();)
		{
			if (cName == *it)
			{
				it = chars.erase(it);
				continue;
			}
			else ++it;
		}
		Push(cp2, (int)chars.size());
		GetCharList(cAcc, cp2, chars);
		SendLoginMsg(DEF_LOGRESMSGTYPE_CHARACTERDELETED, DEF_LOGRESMSGTYPE_CHARACTERDELETED, cData, cp2 - cData, h);
	}
	/*

		if (SaveAccountInfo(0, cAcc, nullptr, cName, 3, h) )
		{

		}*/
}

void LoginServer::ChangePassword(int h, char* pData)
{
	char cAcc[11] = {};
	char cPassword[11] = {};
	char cNewPw[11] = {};
	char cNewPwConf[11] = {};

	auto cp = (char*)(pData + DEF_INDEX2_MSGTYPE + 2);
	Pop(cp, cAcc, 10);
	Pop(cp, cPassword, 10);
	Pop(cp, cNewPw, 10);
	Pop(cp, cNewPwConf, 10);

	std::snprintf(G_cTxt, sizeof(G_cTxt), "(!) Request change password: %s", cAcc);
	PutLogList(G_cTxt);

	std::vector<string> chars;
	auto status = AccountLogIn(cAcc, cPassword, chars);
	if (status != LogIn::Ok)
		return;

	if (string(cNewPw) != cNewPwConf)
	{
		SendLoginMsg(DEF_LOGRESMSGTYPE_PASSWORDCHANGEFAIL, DEF_LOGRESMSGTYPE_PASSWORDCHANGEFAIL, 0, 0, h);
		return;
	}


	char cBuffer[1024] = {};
	char cBuffer2[1024] = {};
	char cTmp[1024] = {};
	char cTxt[1024] = {};
	char cTxt2[1024] = {};
	int iTest = -1;
	std::snprintf(cTmp, sizeof(cTmp), "Accounts\\AscII%d\\%s.txt", cAcc[0], cAcc);
	HANDLE  hFile = CreateFile(cTmp, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		SendLoginMsg(DEF_LOGRESMSGTYPE_PASSWORDCHANGEFAIL, DEF_LOGRESMSGTYPE_PASSWORDCHANGEFAIL, 0, 0, h);
		CloseHandle(hFile);
		return;
	}
	auto iSize = GetFileSize(hFile, 0);
	CloseHandle(hFile);

	char cFileName[512] = {};
	char cDir[112] = {};
	std::memset(cFileName, 0, sizeof(cFileName));
	std::memset(cDir, 0, sizeof(cDir));
	strcat(cFileName, "Accounts");
	strcat(cFileName, "\\");
	std::snprintf(cDir, sizeof(cDir), "AscII%d", *cAcc);
	strcat(cFileName, cDir);
	strcat(cFileName, "\\");
	strcat(cFileName, cAcc);
	strcat(cFileName, ".txt");

	ifstream in(cFileName, ios::in);
	if (!in.is_open())
	{
		PutLogList("in is_opem = false");
		return;
	}
	string wordToReplace("account-password = ");
	wordToReplace.append(cPassword);

	string wordToReplaceWith("account-password = ");
	wordToReplaceWith.append(cNewPw);

	stringstream ss;
	bool done = false;
	string line;
	size_t len = wordToReplace.length();
	while (getline(in, line))
	{
		string repl = wordToReplace;
		size_t pos = line.find(repl);
		len = repl.length();

		if (pos != string::npos)
		{
			line.replace(pos, len, wordToReplaceWith);
			done = true;
		}

		ss << line << '\n';
	}
	in.close();

	ofstream out(cFileName);
	out << ss.str();
	if (done)
	{
		SendLoginMsg(DEF_LOGRESMSGTYPE_PASSWORDCHANGESUCCESS, DEF_LOGRESMSGTYPE_PASSWORDCHANGESUCCESS, 0, 0, h);
	}
	/*

		fpos_t pos;
		fgetpos(pFile, &pos);
		iSize = fread(cBuffer, 1, iSize, pFile);

		std::snprintf(cTxt, sizeof(cTxt), "account-password = %s", cPassword);

		for (int i = 0; i < iSize; i++)
		{
			if (memcmp((char*)cBuffer + i, cTxt, strlen(cTxt)) == 0)
			{
				iTest = i;
				break;
			}
		}

		if (iTest != -1)
		{
			memcpy_secure(cBuffer2, cBuffer, iTest);
			std::snprintf(cTxt2, sizeof(cTxt2), "account-password = %s", cNewPw);
			strcat(cBuffer2, cTxt2);
			memcpy_secure(cBuffer2 + iTest + strlen(cTxt2), cBuffer + iTest + strlen(cTxt), iSize - strlen(cTxt) - iTest);
			fsetpos(pFile, &pos);
			fwrite(cBuffer2, 1, iSize - strlen(cTxt) + strlen(cTxt2), pFile);
			fclose(pFile);
		}
		else
		{
			SendLoginMsg(LOGRESMSGTYPE_PASSWORDCHANGEFAIL, LOGRESMSGTYPE_PASSWORDCHANGEFAIL, 0, 0, h);
			fclose(pFile);
			return;
		}
	*/



}

void LoginServer::CreateNewAccount(int h, char* pData)
{
	char* cp;
	FILE* pFile;
	HANDLE hFile;
	uint32_t  dwFileSize;
	SYSTEMTIME SysTime;
	char cFile[20000], cBuffer[1024], cFn[1024];
	char cName[12] = {};
	char cPassword[12] = {};
	char cEmailAddr[52] = {};
	char cQuiz[47] = {};
	char cAnswer[27] = {};

	if (G_pGame->_lclients[h] == 0)
		return;

	GetLocalTime(&SysTime);

	cp = (char*)(pData + DEF_INDEX2_MSGTYPE + 2);
	Pop(cp, cName, 10);
	Pop(cp, cPassword, 10);
	Pop(cp, cEmailAddr, 50);
	Pop(cp, cQuiz, 45);
	Pop(cp, cAnswer, 25);

	if ((strlen(cName) == 0) || (strlen(cPassword) == 0) ||
		(strlen(cEmailAddr) == 0) || (strlen(cQuiz) == 0) ||
		(strlen(cAnswer) == 0))
		return;

	std::snprintf(G_cTxt, sizeof(G_cTxt), "(!) Request create new Account: %s", cName);
	PutLogList(G_cTxt);

	if (!IsValidName(cName) || !IsValidName(cPassword))
		return;

	std::memset(cFn, 0, sizeof(cFn));
	std::snprintf(cFn, sizeof(cFn), "Accounts\\AscII%d\\%s.txt", cName[0], cName);
	hFile = CreateFile(cFn, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
	dwFileSize = GetFileSize(hFile, 0);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		CloseHandle(hFile);
	}
	else
	{
		CloseHandle(hFile);
		SendLoginMsg(DEF_LOGRESMSGTYPE_NEWACCOUNTFAILED, DEF_LOGRESMSGTYPE_NEWACCOUNTFAILED, 0, 0, h);
		return;
	}

	//mkdir("DataBase");
#ifdef _WIN32
	_mkdir("Accounts");
#endif
	char Aux = 0;
	Aux = cName[0];
	std::memset(cFn, 0, sizeof(cFn));
	std::snprintf(cFn, sizeof(cFn), "Accounts\\AscII%d", Aux);
#ifdef _WIN32
	_mkdir(cFn);
#endif

	std::memset(cFn, 0, sizeof(cFn));
	std::snprintf(cFn, sizeof(cFn), "Accounts\\AscII%d\\%s.txt", Aux, cName);
	pFile = fopen(cFn, "wt");
	if (pFile == 0)
	{
		SendLoginMsg(DEF_LOGRESMSGTYPE_NEWACCOUNTFAILED, DEF_LOGRESMSGTYPE_NEWACCOUNTFAILED, 0, 0, h);
		return;
	}

	SendLoginMsg(DEF_LOGRESMSGTYPE_NEWACCOUNTCREATED, DEF_LOGRESMSGTYPE_NEWACCOUNTCREATED, 0, 0, h);

	std::memset(cFile, 0, sizeof(cFile));
	strcat(cFile, "Account-generated: ");
	std::memset(cBuffer, 0, sizeof(cBuffer));
	std::snprintf(cBuffer, sizeof(cBuffer), "Time(%d:%d/%d/%d/%d) IP(%s)", SysTime.wYear, SysTime.wMonth, SysTime.wDay, SysTime.wHour, SysTime.wMinute, G_pGame->_lclients[h]->_ip);
	strcat(cFile, cBuffer);
	strcat(cFile, "\n");

	strcat(cFile, "[Account Info]");
	strcat(cFile, "\n");

	strcat(cFile, "account-name = ");
	std::memset(cBuffer, 0, sizeof(cBuffer));
	memcpy(cBuffer, cName, 10);
	strcat(cFile, cBuffer);
	strcat(cFile, "\n");

	strcat(cFile, "account-password = ");
	std::memset(cBuffer, 0, sizeof(cBuffer));
	memcpy(cBuffer, cPassword, 10);
	strcat(cFile, cBuffer);
	strcat(cFile, "\n");

	strcat(cFile, "account-Email = ");
	std::memset(cBuffer, 0, sizeof(cBuffer));
	std::snprintf(cBuffer, sizeof(cBuffer), "%s", cEmailAddr);
	strcat(cFile, cBuffer);
	strcat(cFile, "\n");

	strcat(cFile, "account-Quiz = ");
	std::memset(cBuffer, 0, sizeof(cBuffer));
	std::snprintf(cBuffer, sizeof(cBuffer), "%s", cQuiz);
	strcat(cFile, cBuffer);
	strcat(cFile, "\n");

	strcat(cFile, "account-Answer = ");
	std::memset(cBuffer, 0, sizeof(cBuffer));
	std::snprintf(cBuffer, sizeof(cBuffer), "%s", cAnswer);
	strcat(cFile, cBuffer);
	strcat(cFile, "\n");

	strcat(cFile, "account-change-password = ");
	std::memset(cBuffer, 0, sizeof(cBuffer));
	std::snprintf(cBuffer, sizeof(cBuffer), "%d %d %d", SysTime.wYear, SysTime.wMonth, SysTime.wDay);
	strcat(cFile, cBuffer);
	strcat(cFile, "\n");
	strcat(cFile, "\n");

	strcat(cFile, "[CHARACTERS]");
	strcat(cFile, "\n");

	fwrite(cFile, 1, strlen(cFile), pFile);
	fclose(pFile);

	{
		sqlite3* db = nullptr;
		std::string dbPath;
		if (EnsureAccountDatabase(cName, &db, dbPath)) {
			AccountDbAccountData data = {};
			std::snprintf(data.name, sizeof(data.name), "%s", cName);
			std::snprintf(data.password, sizeof(data.password), "%s", cPassword);
			std::snprintf(data.email, sizeof(data.email), "%s", cEmailAddr);
			std::snprintf(data.quiz, sizeof(data.quiz), "%s", cQuiz);
			std::snprintf(data.answer, sizeof(data.answer), "%s", cAnswer);
			FormatTimestamp(SysTime, data.createdAt, sizeof(data.createdAt));
			FormatTimestamp(SysTime, data.passwordChangedAt, sizeof(data.passwordChangedAt));
			std::snprintf(data.lastIp, sizeof(data.lastIp), "%s", G_pGame->_lclients[h]->_ip);
			InsertAccountRecord(db, data);
			CloseAccountDatabase(db);
		}
	}
}


bool LoginServer::SaveAccountInfo(int iAccount, char* cAccountName, char* cTemp, char* cCharName, char cMode, int h)
{
	char* g_txt = &G_cTxt[0];

	char cFileName[255], cDir[63], cTxt[50], cTxt2[2000], cData[2000];
	int iLine, i;
	int    iSize;
	short iMinus;
	int    iCharPos = -1;
	int    iTest = -1;
	bool bDeleteLine;
	HANDLE hFile;
	fpos_t pos;
	uint32_t  dwSize = 0;
	uint32_t dwFileSize;
	FILE* pFile;

	memset(cData, 0, 2000);
	memset(cTxt2, 0, 2000);
	dwFileSize = 0;
	iLine = 0;
	bDeleteLine = false;
	iMinus = 0;
	std::memset(cTxt, 0, sizeof(cTxt));
	std::memset(cTxt2, 0, sizeof(cTxt2));
	std::memset(cData, 0, sizeof(cData));

	std::memset(cFileName, 0, sizeof(cFileName));
	std::memset(cDir, 0, sizeof(cDir));
	strcat(cFileName, "Accounts");
	strcat(cFileName, "\\");
	strcat(cFileName, "\\");
	std::snprintf(cDir, sizeof(cDir), "AscII%d", *cAccountName);
	strcat(cFileName, cDir);
	strcat(cFileName, "\\");
	strcat(cFileName, "\\");
	strcat(cFileName, cAccountName);
	strcat(cFileName, ".txt");

	hFile = CreateFile(cFileName, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
	iSize = GetFileSize(hFile, 0);
	if (hFile != INVALID_HANDLE_VALUE) CloseHandle(hFile);

	//char cTxt3[112] = {};

	switch (cMode) {
	case 1: //save new character
		pFile = fopen(cFileName, "at");
		if (pFile == 0) {
			std::snprintf(g_txt, sizeof(g_txt), "(X) Account none exist: Name(%s)", cAccountName);
			PutLogList(g_txt);
			return false;
		}
		std::snprintf(cTxt, sizeof(cTxt), "\naccount-character = %s", cCharName);
		fwrite(cTxt, 1, strlen(cTxt), pFile);
		fclose(pFile);
		break;

	case 2: //password change
		std::snprintf(g_txt, sizeof(g_txt), "(X) PasswordChange(%s)", cTemp);
		PutLogList(g_txt);
		std::snprintf(cTxt, sizeof(cTxt), "account-password = %s", cTemp);
		pFile = fopen(cFileName, "rt");
		if (pFile == 0) {
			SendLoginMsg(DEF_LOGRESMSGTYPE_NOTEXISTINGACCOUNT, DEF_LOGRESMSGTYPE_NOTEXISTINGACCOUNT, 0, 0, h);
			return false;
		}
		fgetpos(pFile, &pos);
		iSize = fread(cData, 1, iSize, pFile);
		fread(cData, dwFileSize, 1, pFile);
		for (i = 0; i < iSize; i++)
		{
			if (memcmp((char*)cData + i, "[CHARACTERS]", 12) == 0)
			{
				iCharPos = i;
			}
			if (memcmp((char*)cData + i, "account-password = ", 19) == 0)
			{
				iTest = i;
			}
		}
		if (iTest == -1)
		{
			SendLoginMsg(DEF_LOGRESMSGTYPE_NOTEXISTINGACCOUNT, DEF_LOGRESMSGTYPE_NOTEXISTINGACCOUNT, 0, 0, h);
		}
		else
		{
			memcpy(cTxt2, cData, iTest + 19);
			memcpy(cTxt2 + iTest + 19, cTxt, strlen(cTxt));
			memcpy(cTxt2 + iTest + 19 + strlen(cTxt), cData + iTest + 19 + strlen(cTemp), iSize - 19 - iTest - strlen(cTemp));
			SaveInfo(cFileName, cTxt2, 1);
		}
		fclose(pFile);
		break;

	case 3: //delete character
		std::snprintf(g_txt, sizeof(g_txt), "(X) Character Delete(%s)", cCharName);
		PutLogList(g_txt);
		std::snprintf(cTxt, sizeof(cTxt), "account-character = %s", cCharName);
		//std::snprintf(cTxt3, sizeof(cTxt3), "account-character = %s", cCharName);
		pFile = fopen(cFileName, "rt");
		if (pFile == 0) {
			SendLoginMsg(DEF_LOGRESMSGTYPE_NOTEXISTINGACCOUNT, DEF_LOGRESMSGTYPE_NOTEXISTINGACCOUNT, 0, 0, h);
			return false;
		}
		fgetpos(pFile, &pos);
		iSize = fread(cData, 1, iSize, pFile);
		fread(cData, dwFileSize, 1, pFile);
		for (int i = 0; i < iSize; i++)
		{
			if (memcmp((char*)cData + i, "[CHARACTERS]", 12) == 0)
			{
				iCharPos = i;
			}
			if (memcmp((char*)cData + i, cTxt, strlen(cTxt)) == 0)
			{
				iTest = i;
			}
			/*if (memcmp((char*)cData + i, cTxt3, strlen(cTxt3)) == 0)
			{
				iTest = i;
			}*/
		}
		if (iTest == -1)
		{
			SendLoginMsg(DEF_LOGRESMSGTYPE_NOTEXISTINGCHARACTER, DEF_LOGRESMSGTYPE_NOTEXISTINGCHARACTER, 0, 0, h);
		}
		else
		{
			memcpy(cTxt2, cData, iTest - 1);
			memcpy(cTxt2 + iTest - 1, cData + iTest + strlen(cTxt), iSize - strlen(cTxt) - iTest);
			SaveInfo(cFileName, cTxt2, 1);
		}
		fclose(pFile);
		break;
	}
	return false;
}
void LoginServer::SaveInfo(char cFileName[255], char* pData, uint32_t dwStartSize)
{
	FILE* pFile;

	pFile = fopen(cFileName, "wt");
	if (pFile != 0)
	{
		if (sizeof(pData) > 0) fwrite(pData, dwStartSize, strlen(pData), pFile);
		fclose(pFile);
	}
}

void LoginServer::SendLoginMsg(uint32_t msg_id, uint16_t msg_type, char* data, int sz, int h)
{

	int iRet;
	uint32_t* dwp;
	char* cp;
	uint16_t* wp;
	int index = h;

	if (!G_pGame->_lclients[h])
		return;

	std::memset(G_cData50000, 0, sizeof(G_cData50000));

	dwp = (uint32_t*)(G_cData50000 + DEF_INDEX4_MSGID);
	*dwp = msg_id;
	wp = (uint16_t*)(G_cData50000 + DEF_INDEX2_MSGTYPE);
	*wp = msg_type;

	cp = (char*)(G_cData50000 + DEF_INDEX2_MSGTYPE + 2);

	memcpy((char*)cp, data, sz);

	//if is registered
	if (true)
	{
		iRet = G_pGame->_lclients[index]->_sock->iSendMsg(G_cData50000, sz + 6);

		switch (iRet)
		{
		case DEF_XSOCKEVENT_QUENEFULL:
		case DEF_XSOCKEVENT_SOCKETERROR:
		case DEF_XSOCKEVENT_CRITICALERROR:
		case DEF_XSOCKEVENT_SOCKETCLOSED:
			std::snprintf(G_cTxt, sizeof(G_cTxt), "(!) Login Connection Lost on Send (%d)", index);
			PutLogList(G_cTxt);
			delete G_pGame->_lclients[index];
			G_pGame->_lclients[index] = 0;
			return;
		}
	}
}

void LoginServer::RequestEnterGame(int h, char* pData)
{
	//	PutLogList("RequestEnterGame()");

	char cName[11] = {};
	char cMapName[11] = {};
	char cAcc[11] = {};
	char cPass[11] = {};
	int lvl;
	char ws_name[31] = {};
	char cmd_line[121] = {};

	auto cp = (char*)(pData + DEF_INDEX2_MSGTYPE + 2);
	Pop(cp, cName, 10);
	Pop(cp, cMapName, 10);
	Pop(cp, cAcc, 10);
	Pop(cp, cPass, 10);
	Pop(cp, lvl);
	Pop(cp, ws_name, 10);
	Pop(cp, cmd_line, 120);

	char cData[112] = {};
	char* cp2 = cData;
	if (string(ws_name) != WORLDNAMELS)
	{
		//SendEventToWLS(ENTERGAMERESTYPE_REJECT, ENTERGAMERESTYPE_REJECT, 0, 0, h);
		return;
	}

	std::snprintf(G_cTxt, sizeof(G_cTxt), "(!) Request enter Game: %s", cName);
	PutLogList(G_cTxt);

	std::vector<string> chars;
	auto status = AccountLogIn(cAcc, cPass, chars);
	if (status != LogIn::Ok)
		return;

	for (int i = 0; i < DEF_MAXCLIENTS; i++)
	{
		if (!G_pGame->m_pClientList[i])
			continue;

		if (string(G_pGame->m_pClientList[i]->m_cAccountName) == cAcc)
		{
			G_pGame->DeleteClient(i, true, true);
			break;
		}
	}

	cp2 = (char*)cData; //outgoing messag - to Client

	memcpy(cp2, (char*)G_pGame->m_cGameServerAddr, 16);
	cp2 += 16;

	auto wp = (uint16_t*)cp2;
	*wp = (uint16_t)G_pGame->m_iLogServerPort;
	cp2 += 2;

	char sv_name[21] = {};
	std::snprintf(sv_name, sizeof(sv_name), "%s", WORLDNAMELS2);
	memcpy(cp2, sv_name, 20);
	cp2 += 20;

	PutLogList("Send DEF_ENTERGAMERESTYPE_CONFIRM");
	SendLoginMsg(DEF_ENTERGAMERESTYPE_CONFIRM, DEF_ENTERGAMERESTYPE_CONFIRM, cData, 38, h);
}


void LoginServer::LocalSavePlayerData(int h)
{
	char* pData, * cp, cFn[256], cDir[256], cTxt[256], cCharDir[256];
	int    iSize;
	FILE* pFile;
	SYSTEMTIME SysTime;


	if (G_pGame->m_pClientList[h] == 0) return;

	pData = new char[30000];
	std::memset(pData, 0, 30000);

	cp = (char*)(pData);
	iSize = G_pGame->_iComposePlayerDataFileContents(h, cp);

	GetLocalTime(&SysTime);
	std::memset(cCharDir, 0, sizeof(cDir));
	std::snprintf(cCharDir, sizeof(cCharDir), "Characters");
	std::memset(cDir, 0, sizeof(cDir));
	std::memset(cFn, 0, sizeof(cFn));
	strcat(cFn, cCharDir);
	strcat(cFn, "\\");
	strcat(cFn, "\\");
	std::snprintf(cTxt, sizeof(cTxt), "AscII%d", (unsigned char)G_pGame->m_pClientList[h]->m_cCharName[0]);
	strcat(cFn, cTxt);
	strcpy(cDir, cFn);
	strcat(cFn, "\\");
	strcat(cFn, "\\");
	strcat(cFn, G_pGame->m_pClientList[h]->m_cCharName);
	strcat(cFn, ".txt");


#ifdef _WIN32
	_mkdir(cCharDir);
	_mkdir(cDir);
#endif


	if (iSize == 0) {
		//	PutLogList("(!) Character data body empty: Cannot create & save temporal player data file.");
		delete[]pData;
		return;
	}

	pFile = fopen(cFn, "wt");
	if (pFile != 0) {
		fwrite(cp, iSize, 1, pFile);
		fclose(pFile);
	}

	{
		sqlite3* db = nullptr;
		std::string dbPath;
		if (EnsureAccountDatabase(G_pGame->m_pClientList[h]->m_cAccountName, &db, dbPath)) {
			SaveCharacterSnapshot(db, G_pGame->m_pClientList[h]);
			CloseAccountDatabase(db);
		}
	}

	delete[]pData;
}

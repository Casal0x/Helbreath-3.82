#pragma once

#include <cstdint>

class CGame;
class CDynamicObject;

class DynamicObjectManager
{
public:
	DynamicObjectManager() = default;
	~DynamicObjectManager() = default;

	void SetGame(CGame* pGame) { m_pGame = pGame; }

	void InitArrays();
	void CleanupArrays();

	int  iAddDynamicObjectList(short sOwner, char cOwnerType, short sType, char cMapIndex, short sX, short sY, uint32_t dwLastTime, int iV1 = 0);
	void CheckDynamicObjectList();
	void DynamicObjectEffectProcessor();

	CDynamicObject* m_pDynamicObjectList[60000];

private:
	CGame* m_pGame = nullptr;
};

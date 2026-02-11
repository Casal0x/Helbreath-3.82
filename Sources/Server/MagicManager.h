#pragma once

#include <cstdint>

class CGame;

class MagicManager
{
public:
	MagicManager() = default;
	~MagicManager() = default;

	void SetGame(CGame* pGame) { m_pGame = pGame; }

	// Magic config
	bool bSendClientMagicConfigs(int iClientH);
	void ReloadMagicConfigs();

	// Magic casting
	void PlayerMagicHandler(int iClientH, int dX, int dY, short sType, bool bItemEffect = false, int iV1 = 0, uint16_t targetObjectID = 0);
	int iClientMotion_Magic_Handler(int iClientH, short sX, short sY, char cDir);

	// Magic study
	void RequestStudyMagicHandler(int iClientH, const char* pName, bool bIsPurchase = true);
	int _iGetMagicNumber(char* pMagicName, int* pReqInt, int* pCost);
	void GetMagicAbilityHandler(int iClientH);

	// Magic checks
	bool bCheckMagicInt(int iClientH);
	bool bCheckClientMagicFrequency(int iClientH, uint32_t dwClientTime);
	int iGetWhetherMagicBonusEffect(short sType, char cWheatherStatus);

private:
	CGame* m_pGame = nullptr;
};

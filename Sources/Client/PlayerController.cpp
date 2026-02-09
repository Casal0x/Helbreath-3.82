#include "Game.h"
#include "PlayerController.h"
#include "MapData.h"
#include "Misc.h"
#include "DirectionHelpers.h"

CPlayerController::CPlayerController()
{
	Reset();
}

void CPlayerController::Reset()
{
	m_sDestX = 0;
	m_sDestY = 0;
	m_cCommand = 0;
	m_bCommandAvailable = true;
	m_dwCommandTime = 0;
	m_cCommandCount = 0;
	m_iPrevMoveX = 0;
	m_iPrevMoveY = 0;
	m_bIsPrevMoveBlocked = false;
	m_cPlayerTurn = 0;
	m_cPendingStopDir = 0;
	m_dwAttackEndTime = 0;
}

char CPlayerController::GetNextMoveDir(short sX, short sY, short dstX, short dstY,
                                        CMapData* pMapData, bool bMoveCheck, bool bMIM)
{
	char cDir, cTmpDir;
	int aX, aY, dX, dY;
	int i;

	if ((sX == dstX) && (sY == dstY)) return 0;

	dX = sX;
	dY = sY;

	if (bMIM == false)
		cDir = CMisc::cGetNextMoveDir(dX, dY, dstX, dstY);
	else
		cDir = CMisc::cGetNextMoveDir(dstX, dstY, dX, dY);

	if (m_cPlayerTurn == 0)
	{
		for (i = cDir; i <= cDir + 2; i++)
		{
			cTmpDir = i;
			if (cTmpDir > 8) cTmpDir -= 8;
			aX = hb::direction::OffsetX[cTmpDir];
			aY = hb::direction::OffsetY[cTmpDir];
			if (((dX + aX) == m_iPrevMoveX) && ((dY + aY) == m_iPrevMoveY) && (m_bIsPrevMoveBlocked == true) && (bMoveCheck == true))
			{
				m_bIsPrevMoveBlocked = false;
			}
			else if (pMapData->bGetIsLocateable(dX + aX, dY + aY) == true)
			{
				if (pMapData->bIsTeleportLoc(dX + aX, dY + aY) == true)
				{
					return cTmpDir;
				}
				else return cTmpDir;
			}
		}
	}

	if (m_cPlayerTurn == 1)
	{
		for (i = cDir; i >= cDir - 2; i--)
		{
			cTmpDir = i;
			if (cTmpDir < 1) cTmpDir += 8;
			aX = hb::direction::OffsetX[cTmpDir];
			aY = hb::direction::OffsetY[cTmpDir];
			if (((dX + aX) == m_iPrevMoveX) && ((dY + aY) == m_iPrevMoveY) && (m_bIsPrevMoveBlocked == true) && (bMoveCheck == true))
			{
				m_bIsPrevMoveBlocked = false;
			}
			else if (pMapData->bGetIsLocateable(dX + aX, dY + aY) == true)
			{
				if (pMapData->bIsTeleportLoc(dX + aX, dY + aY) == true)
				{
					return cTmpDir;
				}
				else return cTmpDir;
			}
		}
	}

	return 0;
}

void CPlayerController::CalculatePlayerTurn(short playerX, short playerY, CMapData* pMapData)
{
	char cDir;
	short sX, sY, sCnt1, sCnt2;

	sX = playerX;
	sY = playerY;
	sCnt1 = 0;
	m_cPlayerTurn = 0;

	while (1)
	{
		cDir = GetNextMoveDir(sX, sY, m_sDestX, m_sDestY, pMapData);
		if (cDir == 0) break;
		hb::direction::ApplyOffset(cDir, sX, sY);
		sCnt1++;
		if (sCnt1 > 30) break;
	}

	sX = playerX;
	sY = playerY;
	sCnt2 = 0;
	m_cPlayerTurn = 1;

	while (1)
	{
		cDir = GetNextMoveDir(sX, sY, m_sDestX, m_sDestY, pMapData);
		if (cDir == 0) break;
		hb::direction::ApplyOffset(cDir, sX, sY);
		sCnt2++;
		if (sCnt2 > 30) break;
	}

	if (sCnt1 > sCnt2)
		m_cPlayerTurn = 0;
	else
		m_cPlayerTurn = 1;
}

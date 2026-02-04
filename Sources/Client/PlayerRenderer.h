#pragma once

#include "SpriteTypes.h"
#include <cstdint>

class CGame;

class CPlayerRenderer
{
public:
	explicit CPlayerRenderer(CGame& game) : m_game(game) {}

	SpriteLib::BoundRect DrawStop(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime);
	SpriteLib::BoundRect DrawMove(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime);
	SpriteLib::BoundRect DrawRun(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime);
	SpriteLib::BoundRect DrawAttack(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime);
	SpriteLib::BoundRect DrawAttackMove(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime);
	SpriteLib::BoundRect DrawMagic(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime);
	SpriteLib::BoundRect DrawGetItem(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime);
	SpriteLib::BoundRect DrawDamage(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime);
	SpriteLib::BoundRect DrawDamageMove(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime);
	SpriteLib::BoundRect DrawDying(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime);
	SpriteLib::BoundRect DrawDead(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime);

private:
	CGame& m_game;
};

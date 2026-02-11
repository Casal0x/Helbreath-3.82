#pragma once

#include "SpriteTypes.h"
#include <cstdint>

class CGame;

class CNpcRenderer
{
public:
	explicit CNpcRenderer(CGame& game) : m_game(game) {}

	hb::shared::sprite::BoundRect DrawStop(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime);
	hb::shared::sprite::BoundRect DrawMove(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime);
	hb::shared::sprite::BoundRect DrawRun(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime);
	hb::shared::sprite::BoundRect DrawAttack(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime);
	hb::shared::sprite::BoundRect DrawAttackMove(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime);
	hb::shared::sprite::BoundRect DrawMagic(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime);
	hb::shared::sprite::BoundRect DrawGetItem(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime);
	hb::shared::sprite::BoundRect DrawDamage(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime);
	hb::shared::sprite::BoundRect DrawDamageMove(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime);
	hb::shared::sprite::BoundRect DrawDying(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime);
	hb::shared::sprite::BoundRect DrawDead(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime);

private:
	CGame& m_game;
};

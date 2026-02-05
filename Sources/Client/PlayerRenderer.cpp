#include "PlayerRenderer.h"
#include "Game.h"
#include "EquipmentIndices.h"
#include "RenderHelpers.h"
#include "CommonTypes.h"

SpriteLib::BoundRect CPlayerRenderer::DrawStop(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime)
{
	SpriteLib::BoundRect invalidRect = {0, -1, 0, 0};
	auto& state = m_game.m_entityState;
	bool bInv = false;
	bool bAdminInvis = false;

	// Apply motion offset if entity is still interpolating
	sX += static_cast<int>(m_game.m_pMapData->m_pData[state.m_iDataX][state.m_iDataY].m_motion.fCurrentOffsetX);
	sY += static_cast<int>(m_game.m_pMapData->m_pData[state.m_iDataX][state.m_iDataY].m_motion.fCurrentOffsetY);

	// Invisibility check
	if (RenderHelpers::CheckInvisibility(m_game, state, bInv, bAdminInvis))
		return invalidRect;

	// Single-direction monster override
	RenderHelpers::ApplyDirectionOverride(state);

	// Frame halving for stop animation
	state.m_iFrame = state.m_iFrame / 2;

	// Calculate equipment indices — walking uses pose 1, standing uses pose 0
	bool isWalking = state.m_appearance.iIsWalking != 0;
	int bodyPose = isWalking ? 1 : 0;
	EquipmentIndices eq = EquipmentIndices::CalcPlayer(state, bodyPose, bodyPose, bodyPose);
	eq.CalcColors(state);

	// Crusade FOE indicator
	if (m_game.m_bIsCrusadeMode)
		m_game.DrawObjectFOE(sX, sY, state.m_iFrame);

	// NPC ground lights
	RenderHelpers::DrawNpcLight(m_game, state.m_sOwnerType, sX, sY);

	// Effect auras
	RenderHelpers::DrawEffectAuras(m_game, state, sX, sY);

	if (!bTrans)
	{
		m_game.CheckActiveAura(sX, sY, dwTime, state.m_sOwnerType);

		// Draw all equipment layers in correct z-order
		RenderHelpers::DrawPlayerLayers(m_game, eq, state, sX, sY, bInv, _cMantleDrawingOrder, 8, bAdminInvis);

		// Crop effects
		if (state.m_sOwnerType == hb::owner::Crops)
		{
			switch (state.m_iFrame) {
			case 0: m_game.m_pEffectSpr[84]->Draw(sX + 52, sY + 54, (dwTime % 3000) / 120, SpriteLib::DrawParams::Alpha(0.5f)); break;
			case 1: m_game.m_pEffectSpr[83]->Draw(sX + 53, sY + 59, (dwTime % 3000) / 120, SpriteLib::DrawParams::Alpha(0.5f)); break;
			case 2: m_game.m_pEffectSpr[82]->Draw(sX + 53, sY + 65, (dwTime % 3000) / 120, SpriteLib::DrawParams::Alpha(0.5f)); break;
			}
		}

		// Berserk glow
		RenderHelpers::DrawBerserkGlow(m_game, eq, state, sX, sY);

		// Angel + aura
		m_game.DrawAngel(40 + (state.m_iDir - 1), sX + 20, sY - 20, state.m_iFrame % 4, dwTime);
		m_game.CheckActiveAura2(sX, sY, dwTime, state.m_sOwnerType);
	}
	else if (strlen(state.m_cName.data()) > 0)
	{
		// bTrans mode: only draw name
		RenderHelpers::DrawName(m_game, state, sX, sY);
	}

	// Chat message (always)
	RenderHelpers::UpdateChat(m_game, state, sX, sY, indexX, indexY);

	// Abaddon effects (always)
	RenderHelpers::DrawAbaddonEffects(m_game, state, sX, sY);

	// GM mode (always)
	RenderHelpers::DrawGMEffect(m_game, state, sX, sY);

	return m_game.m_pSprite[eq.iBodyIndex + (state.m_iDir - 1)]->GetBoundRect();
}

SpriteLib::BoundRect CPlayerRenderer::DrawMove(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime)
{
	SpriteLib::BoundRect invalidRect = {0, -1, 0, 0};
	auto& state = m_game.m_entityState;
	bool bInv = false;
	bool bAdminInvis = false;

	// Invisibility check
	if (RenderHelpers::CheckInvisibility(m_game, state, bInv, bAdminInvis))
		return invalidRect;

	// Calculate equipment indices — walking uses pose 3, standing uses pose 2
	bool isWalking = state.m_appearance.iIsWalking != 0;
	int bodyPose = isWalking ? 3 : 2;
	EquipmentIndices eq = EquipmentIndices::CalcPlayer(state, bodyPose, bodyPose, bodyPose);
	eq.CalcColors(state);

	// Motion offset
	int dx = static_cast<int>(m_game.m_pMapData->m_pData[state.m_iDataX][state.m_iDataY].m_motion.fCurrentOffsetX);
	int dy = static_cast<int>(m_game.m_pMapData->m_pData[state.m_iDataX][state.m_iDataY].m_motion.fCurrentOffsetY);
	int fix_x = sX + dx;
	int fix_y = sY + dy;

	// Players (types 1-6) never get frame halving in OnMove

	// Crusade FOE indicator
	if (m_game.m_bIsCrusadeMode)
		m_game.DrawObjectFOE(fix_x, fix_y, state.m_iFrame);

	// Effect auras
	RenderHelpers::DrawEffectAuras(m_game, state, fix_x, fix_y);

	if (!bTrans)
	{
		m_game.CheckActiveAura(fix_x, fix_y, dwTime, state.m_sOwnerType);

		// Draw all equipment layers in correct z-order
		RenderHelpers::DrawPlayerLayers(m_game, eq, state, fix_x, fix_y, bInv, _cMantleDrawingOrder, 8, bAdminInvis);

		// Berserk glow
		RenderHelpers::DrawBerserkGlow(m_game, eq, state, fix_x, fix_y);

		// Angel + aura
		m_game.DrawAngel(40 + (state.m_iDir - 1), fix_x + 20, fix_y - 20, state.m_iFrame % 4, dwTime);
		m_game.CheckActiveAura2(fix_x, fix_y, dwTime, state.m_sOwnerType);
	}
	else if (strlen(state.m_cName.data()) > 0)
	{
		RenderHelpers::DrawName(m_game, state, fix_x, fix_y);
	}

	// Chat message (always)
	RenderHelpers::UpdateChat(m_game, state, fix_x, fix_y, indexX, indexY);

	// Store motion offsets
	state.m_iMoveOffsetX = dx;
	state.m_iMoveOffsetY = dy;

	// GM mode (always)
	RenderHelpers::DrawGMEffect(m_game, state, fix_x, fix_y);

	return m_game.m_pSprite[eq.iBodyIndex + (state.m_iDir - 1)]->GetBoundRect();
}

SpriteLib::BoundRect CPlayerRenderer::DrawRun(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime)
{
	SpriteLib::BoundRect invalidRect = {0, -1, 0, 0};
	auto& state = m_game.m_entityState;
	bool bInv = false;
	bool bAdminInvis = false;

	// Invisibility check
	if (RenderHelpers::CheckInvisibility(m_game, state, bInv, bAdminInvis))
		return invalidRect;

	// bodyPose=4, weaponPose=6, shieldPose=6 for running
	EquipmentIndices eq = EquipmentIndices::CalcPlayer(state, 4, 6, 6);
	eq.CalcColors(state);

	// Motion offset
	int dx = static_cast<int>(m_game.m_pMapData->m_pData[state.m_iDataX][state.m_iDataY].m_motion.fCurrentOffsetX);
	int dy = static_cast<int>(m_game.m_pMapData->m_pData[state.m_iDataX][state.m_iDataY].m_motion.fCurrentOffsetY);
	int fix_x = sX + dx;
	int fix_y = sY + dy;

	// Crusade FOE indicator
	if (m_game.m_bIsCrusadeMode)
		m_game.DrawObjectFOE(fix_x, fix_y, state.m_iFrame);

	// Effect auras
	RenderHelpers::DrawEffectAuras(m_game, state, fix_x, fix_y);

	if (!bTrans)
	{
		m_game.CheckActiveAura(fix_x, fix_y, dwTime, state.m_sOwnerType);

		// Draw all equipment layers — uses OnRun mantle order
		RenderHelpers::DrawPlayerLayers(m_game, eq, state, fix_x, fix_y, bInv, _cMantleDrawingOrderOnRun, 8, bAdminInvis);

		// Berserk glow
		RenderHelpers::DrawBerserkGlow(m_game, eq, state, fix_x, fix_y);

		// Angel + aura
		m_game.DrawAngel(40 + (state.m_iDir - 1), fix_x + 20, fix_y - 20, state.m_iFrame % 4, dwTime);
		m_game.CheckActiveAura2(fix_x, fix_y, dwTime, state.m_sOwnerType);

		// Haste trail effect
		if (state.m_status.bHaste)
		{
			int bodyDirIndex = eq.iBodyIndex + (state.m_iDir - 1);
			for (int i = 1; i <= 5; i++)
			{
				int tx = fix_x, ty = fix_y;
				switch (state.m_iDir) {
				case 1: ty += i * 5; break;
				case 2: tx -= i * 5; ty += i * 5; break;
				case 3: tx -= i * 5; break;
				case 4: tx -= i * 5; ty -= i * 5; break;
				case 5: ty -= i * 5; break;
				case 6: tx += i * 5; ty -= i * 5; break;
				case 7: tx += i * 5; break;
				case 8: tx += i * 5; ty += i * 5; break;
				}
				m_game.m_pSprite[bodyDirIndex]->Draw(tx, ty, state.m_iFrame,
					SpriteLib::DrawParams::TintedAlpha(GameColors::BlueTintThird.r, GameColors::BlueTintThird.g, GameColors::BlueTintThird.b, 0.7f));
			}
		}
	}
	else if (strlen(state.m_cName.data()) > 0)
	{
		RenderHelpers::DrawName(m_game, state, fix_x, fix_y);
	}

	// Chat message (always)
	RenderHelpers::UpdateChat(m_game, state, fix_x, fix_y, indexX, indexY);

	// Store motion offsets
	state.m_iMoveOffsetX = dx;
	state.m_iMoveOffsetY = dy;

	// GM mode (always)
	RenderHelpers::DrawGMEffect(m_game, state, fix_x, fix_y);

	return m_game.m_pSprite[eq.iBodyIndex + (state.m_iDir - 1)]->GetBoundRect();
}

SpriteLib::BoundRect CPlayerRenderer::DrawAttack(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime)
{
	SpriteLib::BoundRect invalidRect = {0, -1, 0, 0};
	auto& state = m_game.m_entityState;
	bool bInv = false;
	bool bAdminInvis = false;

	// Invisibility check
	if (RenderHelpers::CheckInvisibility(m_game, state, bInv, bAdminInvis))
		return invalidRect;

	// Attack poses depend on walking state and weapon type
	EquipmentIndices eq;
	if (state.m_appearance.iIsWalking != 0)
	{
		// Walking attack: body pose depends on weapon type (6 for one-hand, 7 for two-hand)
		int iWeapon = state.m_appearance.iWeaponType;
		int iAdd = ((iWeapon >= 40) && (iWeapon <= 59)) ? 7 : 6;
		eq = EquipmentIndices::CalcPlayer(state, iAdd, 4, 4);
	}
	else
	{
		// Standing attack: bodyPose=5, no weapon/shield drawn
		eq = EquipmentIndices::CalcPlayer(state, 5, -1, -1);
	}
	eq.CalcColors(state);

	// Crusade FOE indicator
	if (m_game.m_bIsCrusadeMode)
		m_game.DrawObjectFOE(sX, sY, state.m_iFrame);

	// Effect auras
	RenderHelpers::DrawEffectAuras(m_game, state, sX, sY);

	if (!bTrans)
	{
		m_game.CheckActiveAura(sX, sY, dwTime, state.m_sOwnerType);

		// Draw all equipment layers
		RenderHelpers::DrawPlayerLayers(m_game, eq, state, sX, sY, bInv, _cMantleDrawingOrder, 8, bAdminInvis);

		// Attack-specific: weapon swing trail at frame 3
		if (eq.iWeaponIndex != -1 && state.m_iFrame == 3)
			m_game.m_pSprite[eq.iWeaponIndex]->Draw(sX, sY, state.m_iFrame - 1,
				SpriteLib::DrawParams::TintedAlpha(GameColors::BlueTintThird.r, GameColors::BlueTintThird.g, GameColors::BlueTintThird.b, 0.7f));

		// Berserk glow
		RenderHelpers::DrawBerserkGlow(m_game, eq, state, sX, sY);

		// Angel + aura — OnAttack uses (dir-1) angel index, frame % 8
		m_game.DrawAngel(state.m_iDir - 1, sX + 20, sY - 20, state.m_iFrame % 8, dwTime);
		m_game.CheckActiveAura2(sX, sY, dwTime, state.m_sOwnerType);
	}
	else if (strlen(state.m_cName.data()) > 0)
	{
		RenderHelpers::DrawName(m_game, state, sX, sY);
	}

	// Chat message (always)
	RenderHelpers::UpdateChat(m_game, state, sX, sY, indexX, indexY);

	// Abaddon effects (always) — players can't be Abaddon, but keep for consistency
	RenderHelpers::DrawAbaddonEffects(m_game, state, sX, sY);

	// GM mode (always)
	RenderHelpers::DrawGMEffect(m_game, state, sX, sY);

	return m_game.m_pSprite[eq.iBodyIndex + (state.m_iDir - 1)]->GetBoundRect();
}

SpriteLib::BoundRect CPlayerRenderer::DrawAttackMove(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime)
{
	SpriteLib::BoundRect invalidRect = {0, -1, 0, 0};
	auto& state = m_game.m_entityState;
	bool bInv = false;
	bool bAdminInvis = false;

	// Invisibility check
	if (RenderHelpers::CheckInvisibility(m_game, state, bInv, bAdminInvis))
		return invalidRect;

	// Frame clamping — same as original
	switch (state.m_iFrame) {
	case 4: case 5: case 6: case 7: case 8: case 9: state.m_iFrame = 4; break;
	case 10: state.m_iFrame = 5; break;
	case 11: state.m_iFrame = 6; break;
	case 12: state.m_iFrame = 7; break;
	}

	// Same poses as OnAttack
	EquipmentIndices eq;
	if (state.m_appearance.iIsWalking != 0)
	{
		int iWeapon = state.m_appearance.iWeaponType;
		int iAdd = ((iWeapon >= 40) && (iWeapon <= 59)) ? 7 : 6;
		eq = EquipmentIndices::CalcPlayer(state, iAdd, 4, 4);
	}
	else
	{
		eq = EquipmentIndices::CalcPlayer(state, 5, -1, -1);
	}
	eq.CalcColors(state);

	// Frame-based motion offset
	int dx = 0, dy = 0;
	bool bDashDraw = false;
	int dsx = 0, dsy = 0;
	int cFrameMoveDots = 0;

	if ((state.m_iFrame >= 1) && (state.m_iFrame <= 3))
	{
		switch (state.m_iFrame) {
		case 1: cFrameMoveDots = 26; break;
		case 2: cFrameMoveDots = 16; break;
		case 3: cFrameMoveDots = 0;  break;
		}
		switch (state.m_iDir) {
		case 1: dy = cFrameMoveDots; break;
		case 2: dy = cFrameMoveDots; dx = -cFrameMoveDots; break;
		case 3: dx = -cFrameMoveDots; break;
		case 4: dx = -cFrameMoveDots; dy = -cFrameMoveDots; break;
		case 5: dy = -cFrameMoveDots; break;
		case 6: dy = -cFrameMoveDots; dx = cFrameMoveDots; break;
		case 7: dx = cFrameMoveDots; break;
		case 8: dx = cFrameMoveDots; dy = cFrameMoveDots; break;
		}
		switch (state.m_iFrame) {
		case 1: dy++;    break;
		case 2: dy += 2; break;
		case 3: dy++;    break;
		}
		switch (state.m_iFrame) {
		case 2: bDashDraw = true; cFrameMoveDots = 26; break;
		case 3: bDashDraw = true; cFrameMoveDots = 16; break;
		}
		switch (state.m_iDir) {
		case 1: dsy = cFrameMoveDots; break;
		case 2: dsy = cFrameMoveDots; dsx = -cFrameMoveDots; break;
		case 3: dsx = -cFrameMoveDots; break;
		case 4: dsx = -cFrameMoveDots; dsy = -cFrameMoveDots; break;
		case 5: dsy = -cFrameMoveDots; break;
		case 6: dsy = -cFrameMoveDots; dsx = cFrameMoveDots; break;
		case 7: dsx = cFrameMoveDots; break;
		case 8: dsx = cFrameMoveDots; dsy = cFrameMoveDots; break;
		}
	}
	else if (state.m_iFrame == 0)
	{
		switch (state.m_iDir) {
		case 1: dy = 32; break;
		case 2: dy = 32; dx = -32; break;
		case 3: dx = -32; break;
		case 4: dx = -32; dy = -32; break;
		case 5: dy = -32; break;
		case 6: dy = -32; dx = 32; break;
		case 7: dx = 32; break;
		case 8: dx = 32; dy = 32; break;
		}
	}

	// Crusade FOE indicator
	if (m_game.m_bIsCrusadeMode)
		m_game.DrawObjectFOE(sX + dx, sY + dy, state.m_iFrame);

	// Effect auras
	RenderHelpers::DrawEffectAuras(m_game, state, sX + dx, sY + dy);

	if (!bTrans)
	{
		m_game.CheckActiveAura(sX + dx, sY + dy, dwTime, state.m_sOwnerType);

		// Draw all equipment layers
		RenderHelpers::DrawPlayerLayers(m_game, eq, state, sX + dx, sY + dy, bInv, _cMantleDrawingOrder, 8, bAdminInvis);

		// Attack-specific: weapon swing trail at frame 3
		if (eq.iWeaponIndex != -1 && state.m_iFrame == 3)
			m_game.m_pSprite[eq.iWeaponIndex]->Draw(sX + dx, sY + dy, state.m_iFrame - 1,
				SpriteLib::DrawParams::TintedAlpha(GameColors::BlueTintThird.r, GameColors::BlueTintThird.g, GameColors::BlueTintThird.b, 0.7f));

		// Berserk glow
		RenderHelpers::DrawBerserkGlow(m_game, eq, state, sX + dx, sY + dy);

		// Angel + aura
		m_game.DrawAngel(8 + (state.m_iDir - 1), sX + dx + 20, sY + dy - 20, state.m_iFrame % 8, dwTime);
		m_game.CheckActiveAura2(sX + dx, sY + dy, dwTime, state.m_sOwnerType);

		// Dash ghost effect
		if (bDashDraw)
		{
			m_game.m_pSprite[eq.iBodyIndex + (state.m_iDir - 1)]->Draw(sX + dsx, sY + dsy, state.m_iFrame,
				SpriteLib::DrawParams::TintedAlpha(GameColors::BlueTintThird.r, GameColors::BlueTintThird.g, GameColors::BlueTintThird.b, 0.7f));
			if (eq.iWeaponIndex != -1)
				m_game.m_pSprite[eq.iWeaponIndex]->Draw(sX + dsx, sY + dsy, state.m_iFrame,
					SpriteLib::DrawParams::TintedAlpha(GameColors::BlueTintThird.r, GameColors::BlueTintThird.g, GameColors::BlueTintThird.b, 0.7f));
			if (eq.iShieldIndex != -1)
				m_game.m_pSprite[eq.iShieldIndex]->Draw(sX + dsx, sY + dsy, (state.m_iDir - 1) * 8 + state.m_iFrame,
					SpriteLib::DrawParams::TintedAlpha(GameColors::BlueTintThird.r, GameColors::BlueTintThird.g, GameColors::BlueTintThird.b, 0.7f));
		}
	}
	else if (strlen(state.m_cName.data()) > 0)
	{
		RenderHelpers::DrawName(m_game, state, sX + dx, sY + dy);
	}

	// Chat message (always)
	RenderHelpers::UpdateChat(m_game, state, sX + dx, sY + dy, indexX, indexY);

	// Store motion offsets
	state.m_iMoveOffsetX = dx;
	state.m_iMoveOffsetY = dy;

	// GM mode (always)
	RenderHelpers::DrawGMEffect(m_game, state, sX + dx, sY + dy);

	return m_game.m_pSprite[eq.iBodyIndex + (state.m_iDir - 1)]->GetBoundRect();
}

SpriteLib::BoundRect CPlayerRenderer::DrawMagic(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime)
{
	SpriteLib::BoundRect invalidRect = {0, -1, 0, 0};
	auto& state = m_game.m_entityState;
	bool bInv = false;
	bool bAdminInvis = false;

	// Magic has special invisibility handling — updates chat position for invisible enemies
	if (hb::owner::IsAlwaysInvisible(state.m_sOwnerType)) bInv = true;
	if (state.m_status.bInvisibility)
	{
		if (state.m_wObjectID == m_game.m_pPlayer->m_sPlayerObjectID)
			bInv = true;
		else
		{
			// Update chat position even for invisible enemies
			if (state.m_iChatIndex != 0)
			{
				if (m_game.m_pChatMsgList[state.m_iChatIndex] != 0)
				{
					m_game.m_pChatMsgList[state.m_iChatIndex]->m_sX = sX;
					m_game.m_pChatMsgList[state.m_iChatIndex]->m_sY = sY;
				}
				else
				{
					m_game.m_pMapData->ClearChatMsg(indexX, indexY);
				}
			}
			return invalidRect;
		}
	}

	// bodyPose=8, no weapon/shield
	EquipmentIndices eq = EquipmentIndices::CalcPlayer(state, 8, -1, -1);
	eq.CalcColors(state);

	// Crusade FOE indicator
	if (m_game.m_bIsCrusadeMode)
		m_game.DrawObjectFOE(sX, sY, state.m_iFrame);

	// Effect auras
	RenderHelpers::DrawEffectAuras(m_game, state, sX, sY);

	if (!bTrans)
	{
		m_game.CheckActiveAura(sX, sY, dwTime, state.m_sOwnerType);

		// Draw all equipment layers — equipFrameMul=16 for magic
		RenderHelpers::DrawPlayerLayers(m_game, eq, state, sX, sY, bInv, _cMantleDrawingOrder, 16, bAdminInvis);

		// Berserk glow
		RenderHelpers::DrawBerserkGlow(m_game, eq, state, sX, sY);

		// Angel + aura — OnMagic uses 32+dir-1, frame%16
		m_game.DrawAngel(32 + (state.m_iDir - 1), sX + 20, sY - 20, state.m_iFrame % 16, dwTime);
		m_game.CheckActiveAura2(sX, sY, dwTime, state.m_sOwnerType);
	}
	else if (strlen(state.m_cName.data()) > 0)
	{
		RenderHelpers::DrawName(m_game, state, sX, sY);
	}

	// Chat message (always)
	RenderHelpers::UpdateChat(m_game, state, sX, sY, indexX, indexY);

	return m_game.m_pSprite[eq.iBodyIndex + (state.m_iDir - 1)]->GetBoundRect();
}

SpriteLib::BoundRect CPlayerRenderer::DrawGetItem(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime)
{
	SpriteLib::BoundRect invalidRect = {0, -1, 0, 0};
	auto& state = m_game.m_entityState;
	bool bInv = false;
	bool bAdminInvis = false;

	// Invisibility check
	if (RenderHelpers::CheckInvisibility(m_game, state, bInv, bAdminInvis))
		return invalidRect;

	// bodyPose=9, no weapon/shield
	EquipmentIndices eq = EquipmentIndices::CalcPlayer(state, 9, -1, -1);
	eq.CalcColors(state);

	// Crusade FOE indicator
	if (m_game.m_bIsCrusadeMode)
		m_game.DrawObjectFOE(sX, sY, state.m_iFrame);

	// Effect auras
	RenderHelpers::DrawEffectAuras(m_game, state, sX, sY);

	if (!bTrans)
	{
		m_game.CheckActiveAura(sX, sY, dwTime, state.m_sOwnerType);

		// Draw all equipment layers — equipFrameMul=4 for get item
		RenderHelpers::DrawPlayerLayers(m_game, eq, state, sX, sY, bInv, _cMantleDrawingOrder, 4, bAdminInvis);

		// Berserk glow
		RenderHelpers::DrawBerserkGlow(m_game, eq, state, sX, sY);

		// Angel + aura — OnGetItem uses 40+dir-1, frame%4
		m_game.DrawAngel(40 + (state.m_iDir - 1), sX + 20, sY - 20, state.m_iFrame % 4, dwTime);
		m_game.CheckActiveAura2(sX, sY, dwTime, state.m_sOwnerType);
	}
	else if (strlen(state.m_cName.data()) > 0)
	{
		RenderHelpers::DrawName(m_game, state, sX, sY);
	}

	// Chat message (always)
	RenderHelpers::UpdateChat(m_game, state, sX, sY, indexX, indexY);

	return m_game.m_pSprite[eq.iBodyIndex + (state.m_iDir - 1)]->GetBoundRect();
}

SpriteLib::BoundRect CPlayerRenderer::DrawDamage(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime)
{
	SpriteLib::BoundRect invalidRect = {0, -1, 0, 0};
	auto& state = m_game.m_entityState;
	bool bInv = false;
	bool bAdminInvis = false;

	// Invisibility check
	if (RenderHelpers::CheckInvisibility(m_game, state, bInv, bAdminInvis))
		return invalidRect;

	// Two-state: cFrame<4 = idle, cFrame>=4 = damage recoil
	EquipmentIndices eq;
	int equipFrameMul;
	char cFrame = state.m_iFrame;

	if (cFrame < 4)
	{
		// Idle state — walking or standing, with weapon/shield
		bool isWalking = state.m_appearance.iIsWalking != 0;
		int bodyPose = isWalking ? 1 : 0;
		eq = EquipmentIndices::CalcPlayer(state, bodyPose, bodyPose, bodyPose);
		equipFrameMul = 8;
	}
	else
	{
		// Damage recoil state — pose 10, weapon pose 5, shield pose 5
		cFrame -= 4;
		state.m_iFrame = cFrame;
		eq = EquipmentIndices::CalcPlayer(state, 10, 5, 5);
		equipFrameMul = 4;
	}
	eq.CalcColors(state);

	// Crusade FOE indicator
	if (m_game.m_bIsCrusadeMode)
		m_game.DrawObjectFOE(sX, sY, cFrame);

	// Effect auras
	RenderHelpers::DrawEffectAuras(m_game, state, sX, sY);

	if (!bTrans)
	{
		m_game.CheckActiveAura(sX, sY, dwTime, state.m_sOwnerType);

		// Draw all equipment layers
		RenderHelpers::DrawPlayerLayers(m_game, eq, state, sX, sY, bInv, _cMantleDrawingOrder, equipFrameMul);

		// Berserk glow
		RenderHelpers::DrawBerserkGlow(m_game, eq, state, sX, sY);

		// Angel + aura — OnDamage uses 16+dir-1, cFrame%4
		m_game.DrawAngel(16 + (state.m_iDir - 1), sX + 20, sY - 20, cFrame % 4, dwTime);
		m_game.CheckActiveAura2(sX, sY, dwTime, state.m_sOwnerType);
	}
	else if (strlen(state.m_cName.data()) > 0)
	{
		RenderHelpers::DrawName(m_game, state, sX, sY);
	}

	// Chat message (always)
	RenderHelpers::UpdateChat(m_game, state, sX, sY, indexX, indexY);

	// Abaddon effects (always)
	RenderHelpers::DrawAbaddonEffects(m_game, state, sX, sY);

	// GM mode (always)
	RenderHelpers::DrawGMEffect(m_game, state, sX, sY);

	return m_game.m_pSprite[eq.iBodyIndex + (state.m_iDir - 1)]->GetBoundRect();
}

SpriteLib::BoundRect CPlayerRenderer::DrawDamageMove(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime)
{
	SpriteLib::BoundRect invalidRect = {0, -1, 0, 0};
	auto& state = m_game.m_entityState;
	bool bInv = false;
	bool bAdminInvis = false;

	// Invisibility check
	if (RenderHelpers::CheckInvisibility(m_game, state, bInv, bAdminInvis))
		return invalidRect;

	// Direction inversion (knockback is opposite direction)
	switch (state.m_iDir) {
	case 1: state.m_iDir = 5; break;
	case 2: state.m_iDir = 6; break;
	case 3: state.m_iDir = 7; break;
	case 4: state.m_iDir = 8; break;
	case 5: state.m_iDir = 1; break;
	case 6: state.m_iDir = 2; break;
	case 7: state.m_iDir = 3; break;
	case 8: state.m_iDir = 4; break;
	}

	// bodyPose=10, weaponPose=5, shieldPose=5
	EquipmentIndices eq = EquipmentIndices::CalcPlayer(state, 10, 5, 5);
	eq.CalcColors(state);

	// Motion offset
	int dx = static_cast<int>(m_game.m_pMapData->m_pData[state.m_iDataX][state.m_iDataY].m_motion.fCurrentOffsetX);
	int dy = static_cast<int>(m_game.m_pMapData->m_pData[state.m_iDataX][state.m_iDataY].m_motion.fCurrentOffsetY);
	int fix_x = sX + dx;
	int fix_y = sY + dy;

	int cFrame = state.m_iFrame;

	// Crusade FOE indicator
	if (m_game.m_bIsCrusadeMode)
		m_game.DrawObjectFOE(fix_x, fix_y, cFrame);

	// Effect auras
	RenderHelpers::DrawEffectAuras(m_game, state, fix_x, fix_y);

	if (!bTrans)
	{
		m_game.CheckActiveAura(fix_x, fix_y, dwTime, state.m_sOwnerType);

		// Draw all equipment layers — equipFrameMul=4 for damage move
		RenderHelpers::DrawPlayerLayers(m_game, eq, state, fix_x, fix_y, bInv, _cMantleDrawingOrder, 4, bAdminInvis);

		// Berserk glow
		RenderHelpers::DrawBerserkGlow(m_game, eq, state, fix_x, fix_y);

		// Angel + aura — OnDamageMove uses 16+dir-1, cFrame%4
		m_game.DrawAngel(16 + (state.m_iDir - 1), fix_x + 20, fix_y - 20, cFrame % 4, dwTime);
		m_game.CheckActiveAura2(fix_x, fix_y, dwTime, state.m_sOwnerType);
	}
	else if (strlen(state.m_cName.data()) > 0)
	{
		RenderHelpers::DrawName(m_game, state, fix_x, fix_y);
	}

	// Chat message (always)
	RenderHelpers::UpdateChat(m_game, state, fix_x, fix_y, indexX, indexY);

	// Store motion offsets
	state.m_iMoveOffsetX = dx;
	state.m_iMoveOffsetY = dy;

	// GM mode (always)
	RenderHelpers::DrawGMEffect(m_game, state, fix_x, fix_y);

	return m_game.m_pSprite[eq.iBodyIndex + (state.m_iDir - 1)]->GetBoundRect();
}

SpriteLib::BoundRect CPlayerRenderer::DrawDying(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime)
{
	SpriteLib::BoundRect invalidRect = {0, -1, 0, 0};
	auto& state = m_game.m_entityState;

	// No invisibility check for dying

	// Save original frame for angel drawing
	int originalFrame = state.m_iFrame;
	char cFrame = state.m_iFrame;

	// Two-state: cFrame<6 = standing idle, cFrame>=6 = dying animation
	EquipmentIndices eq;
	if (cFrame < 6)
	{
		// Standing idle — bodyPose=0, no weapon/shield
		eq = EquipmentIndices::CalcPlayer(state, 0, -1, -1);
	}
	else
	{
		// Dying animation — bodyPose=11, no weapon/shield
		cFrame -= 6;
		state.m_iFrame = cFrame;
		eq = EquipmentIndices::CalcPlayer(state, 11, -1, -1);
	}
	eq.CalcColors(state);

	// Crusade FOE indicator
	if (m_game.m_bIsCrusadeMode)
		m_game.DrawObjectFOE(sX, sY, cFrame);

	// Effect auras
	RenderHelpers::DrawEffectAuras(m_game, state, sX, sY);

	if (!bTrans)
	{
		// Draw all equipment layers — equipFrameMul=8
		RenderHelpers::DrawPlayerLayers(m_game, eq, state, sX, sY, false, _cMantleDrawingOrder);

		// Berserk glow
		RenderHelpers::DrawBerserkGlow(m_game, eq, state, sX, sY);

		// Angel + aura — OnDying uses 24+dir-1, ORIGINAL frame (not adjusted)
		m_game.DrawAngel(24 + (state.m_iDir - 1), sX + 20, sY - 20, originalFrame, dwTime);
		m_game.CheckActiveAura2(sX, sY, dwTime, state.m_sOwnerType);
	}
	else if (strlen(state.m_cName.data()) > 0)
	{
		RenderHelpers::DrawName(m_game, state, sX, sY);
	}

	// Chat message (always)
	RenderHelpers::UpdateChat(m_game, state, sX, sY, indexX, indexY);

	return m_game.m_pSprite[eq.iBodyIndex + (state.m_iDir - 1)]->GetBoundRect();
}

SpriteLib::BoundRect CPlayerRenderer::DrawDead(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime)
{
	SpriteLib::BoundRect invalidRect = {0, -1, 0, 0};
	auto& state = m_game.m_entityState;

	// bodyPose=11, frame=7 fixed for dead players
	EquipmentIndices eq = EquipmentIndices::CalcPlayer(state, 11, -1, -1);
	eq.CalcColors(state);
	int iFrame = 7;

	if (!bTrans)
	{
		if (state.m_iFrame == -1)
		{
			// Full corpse with equipment — just-died state
			state.m_iFrame = 7;
			RenderHelpers::DrawPlayerLayers(m_game, eq, state, sX, sY, false, _cMantleDrawingOrder);
		}
		else if (state.m_status.bBerserk)
		{
			// Berserk corpse fade — reddish tint
			m_game.m_pSprite[eq.iBodyIndex + (state.m_iDir - 1)]->Draw(sX, sY, iFrame,
				SpriteLib::DrawParams::TintedAlpha(-2 * state.m_iFrame + 5, -2 * state.m_iFrame - 5, -2 * state.m_iFrame - 5, 0.7f));
		}
		else
		{
			// Normal corpse fade
			m_game.m_pSprite[eq.iBodyIndex + (state.m_iDir - 1)]->Draw(sX, sY, iFrame,
				SpriteLib::DrawParams::TintedAlpha(-2 * state.m_iFrame, -2 * state.m_iFrame, -2 * state.m_iFrame, 0.7f));
		}
	}
	else if (strlen(state.m_cName.data()) > 0)
	{
		RenderHelpers::DrawName(m_game, state, sX, sY);
	}

	// Chat message — uses ClearDeadChatMsg for dead entities
	if (state.m_iChatIndex != 0)
	{
		if ((m_game.m_pChatMsgList[state.m_iChatIndex] != 0) &&
			(m_game.m_pChatMsgList[state.m_iChatIndex]->m_iObjectID == state.m_wObjectID))
		{
			m_game.m_pChatMsgList[state.m_iChatIndex]->m_sX = sX;
			m_game.m_pChatMsgList[state.m_iChatIndex]->m_sY = sY;
		}
		else
		{
			m_game.m_pMapData->ClearDeadChatMsg(indexX, indexY);
		}
	}

	return m_game.m_pSprite[eq.iBodyIndex + (state.m_iDir - 1)]->GetBoundRect();
}

#include "NpcRenderer.h"
#include "Game.h"
#include "EquipmentIndices.h"
#include "RenderHelpers.h"
#include "CommonTypes.h"

SpriteLib::BoundRect CNpcRenderer::DrawStop(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime)
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

	// NPC body index calculation
	EquipmentIndices eq = EquipmentIndices::CalcNpc(state, 0);
	eq.CalcColors(state);

	// Special frame from sRawAppr2
	if (state.m_appearance.sRawAppr2 != 0)
	{
		eq.iBodyIndex = DEF_SPRID_MOB + (state.m_sOwnerType - 10) * 8 * 7 + (4 * 8);
		state.m_iFrame = (state.m_appearance.sRawAppr2 & 0x00FF) - 1;
	}

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

		// Draw NPC body (shadow + body sprite)
		RenderHelpers::DrawNpcLayers(m_game, eq, state, sX, sY, bInv);

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

// Helper: returns true if this NPC type should keep full animation frames (no halving).
// Types NOT in this list get frame /= 2 during move animation.
static bool ShouldKeepFullFrames(short sOwnerType)
{
	switch (sOwnerType) {
	case hb::owner::Troll:
	case hb::owner::Ogre:
	case hb::owner::Liche:
	case hb::owner::Demon:
	case hb::owner::Unicorn:
	case hb::owner::WereWolf:
	case hb::owner::LightWarBeetle:
	case hb::owner::GodsHandKnight:
	case hb::owner::GodsHandKnightCK:
	case hb::owner::TempleKnight:
	case hb::owner::BattleGolem:
	case hb::owner::Stalker:
	case hb::owner::HellClaw:
	case hb::owner::TigerWorm:
	case hb::owner::Gargoyle:
	case hb::owner::Beholder:
	case hb::owner::DarkElf:
	case hb::owner::Bunny:
	case hb::owner::Cat:
	case hb::owner::GiantFrog:
	case hb::owner::MountainGiant:
	case hb::owner::Ettin:
	case hb::owner::CannibalPlant:
	case hb::owner::Rudolph:
	case hb::owner::DireBoar:
	case hb::owner::Frost:
	case hb::owner::IceGolem:
	case hb::owner::Wyvern:
	case hb::owner::Dragon:
	case hb::owner::Centaur:
	case hb::owner::ClawTurtle:
	case hb::owner::FireWyvern:
	case hb::owner::GiantCrayfish:
	case hb::owner::GiLizard:
	case hb::owner::GiTree:
	case hb::owner::MasterOrc:
	case hb::owner::Minaus:
	case hb::owner::Nizie:
	case hb::owner::Tentocle:
	case hb::owner::Abaddon:
	case hb::owner::Sorceress:
	case hb::owner::ATK:
	case hb::owner::MasterElf:
	case hb::owner::DSK:
	case hb::owner::HBT:
	case hb::owner::CT:
	case hb::owner::Barbarian:
	case hb::owner::AGC:
	case hb::owner::Gail:
		return true;
	default:
		return false;
	}
}

SpriteLib::BoundRect CNpcRenderer::DrawMove(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime)
{
	SpriteLib::BoundRect invalidRect = {0, -1, 0, 0};
	auto& state = m_game.m_entityState;
	bool bInv = false;
	bool bAdminInvis = false;

	// Invisibility check
	if (RenderHelpers::CheckInvisibility(m_game, state, bInv, bAdminInvis))
		return invalidRect;

	// NPC body index: HBT uses pose 0, all others use pose 1
	int npcPose = (state.m_sOwnerType == hb::owner::HBT) ? 0 : 1;
	EquipmentIndices eq = EquipmentIndices::CalcNpc(state, npcPose);
	eq.CalcColors(state);

	// Motion offset
	int dx = static_cast<int>(m_game.m_pMapData->m_pData[state.m_iDataX][state.m_iDataY].m_motion.fCurrentOffsetX);
	int dy = static_cast<int>(m_game.m_pMapData->m_pData[state.m_iDataX][state.m_iDataY].m_motion.fCurrentOffsetY);
	int fix_x = sX + dx;
	int fix_y = sY + dy;

	// Frame halving for non-exempt NPC types
	if (!ShouldKeepFullFrames(state.m_sOwnerType))
		state.m_iFrame = state.m_iFrame / 2;

	// Crusade FOE indicator
	if (m_game.m_bIsCrusadeMode)
		m_game.DrawObjectFOE(fix_x, fix_y, state.m_iFrame);

	// Effect auras
	RenderHelpers::DrawEffectAuras(m_game, state, fix_x, fix_y);

	// IceGolem particle effects
	if (state.m_sOwnerType == hb::owner::IceGolem)
	{
		switch (rand() % 3) {
		case 0: m_game.m_pEffectSpr[76]->Draw(fix_x, fix_y, state.m_iFrame, SpriteLib::DrawParams::Alpha(0.7f)); break;
		case 1: m_game.m_pEffectSpr[77]->Draw(fix_x, fix_y, state.m_iFrame, SpriteLib::DrawParams::Alpha(0.7f)); break;
		case 2: m_game.m_pEffectSpr[78]->Draw(fix_x, fix_y, state.m_iFrame, SpriteLib::DrawParams::Alpha(0.7f)); break;
		}
	}

	if (!bTrans)
	{
		m_game.CheckActiveAura(fix_x, fix_y, dwTime, state.m_sOwnerType);

		// Draw NPC body (shadow + body sprite)
		RenderHelpers::DrawNpcLayers(m_game, eq, state, fix_x, fix_y, bInv);

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

	// Abaddon effects — surrounding effects use sX/sY (tile-anchored),
	// direction-based effects use fix_x/fix_y (entity-following)
	if (state.m_sOwnerType == hb::owner::Abaddon)
	{
		int randFrame = state.m_iEffectFrame % 12;
		m_game.m_pEffectSpr[154]->Draw(sX - 50, sY - 50, randFrame, SpriteLib::DrawParams::Alpha(0.7f));
		m_game.m_pEffectSpr[155]->Draw(sX - 20, sY - 80, randFrame, SpriteLib::DrawParams::Alpha(0.7f));
		m_game.m_pEffectSpr[156]->Draw(sX + 70, sY - 50, randFrame, SpriteLib::DrawParams::Alpha(0.7f));
		m_game.m_pEffectSpr[157]->Draw(sX - 30, sY, randFrame, SpriteLib::DrawParams::Alpha(0.7f));
		m_game.m_pEffectSpr[158]->Draw(sX - 60, sY + 90, randFrame, SpriteLib::DrawParams::Alpha(0.7f));
		m_game.m_pEffectSpr[159]->Draw(sX + 65, sY + 85, randFrame, SpriteLib::DrawParams::Alpha(0.7f));
		int ef = state.m_iEffectFrame;
		switch (state.m_iDir) {
		case 1:
			m_game.m_pEffectSpr[153]->Draw(fix_x, fix_y + 108, ef % 28, SpriteLib::DrawParams::Alpha(0.7f));
			m_game.m_pEffectSpr[164]->Draw(fix_x - 50, fix_y + 10, ef % 15, SpriteLib::DrawParams::Alpha(0.7f));
			break;
		case 2:
			m_game.m_pEffectSpr[153]->Draw(fix_x, fix_y + 95, ef % 28, SpriteLib::DrawParams::Alpha(0.7f));
			m_game.m_pEffectSpr[164]->Draw(fix_x - 70, fix_y + 10, ef % 15, SpriteLib::DrawParams::Alpha(0.7f));
			break;
		case 3:
			m_game.m_pEffectSpr[153]->Draw(fix_x, fix_y + 105, ef % 28, SpriteLib::DrawParams::Alpha(0.7f));
			m_game.m_pEffectSpr[164]->Draw(fix_x - 90, fix_y + 10, ef % 15, SpriteLib::DrawParams::Alpha(0.7f));
			break;
		case 4:
			m_game.m_pEffectSpr[153]->Draw(fix_x - 35, fix_y + 100, ef % 28, SpriteLib::DrawParams::Alpha(0.7f));
			m_game.m_pEffectSpr[164]->Draw(fix_x - 80, fix_y + 10, ef % 15, SpriteLib::DrawParams::Alpha(0.7f));
			break;
		case 5:
			m_game.m_pEffectSpr[153]->Draw(fix_x, fix_y + 95, ef % 28, SpriteLib::DrawParams::Alpha(0.7f));
			m_game.m_pEffectSpr[164]->Draw(fix_x - 65, fix_y - 5, ef % 15, SpriteLib::DrawParams::Alpha(0.7f));
			break;
		case 6:
			m_game.m_pEffectSpr[153]->Draw(fix_x + 45, fix_y + 95, ef % 28, SpriteLib::DrawParams::Alpha(0.7f));
			m_game.m_pEffectSpr[164]->Draw(fix_x - 31, fix_y + 10, ef % 15, SpriteLib::DrawParams::Alpha(0.7f));
			break;
		case 7:
			m_game.m_pEffectSpr[153]->Draw(fix_x + 40, fix_y + 110, ef % 28, SpriteLib::DrawParams::Alpha(0.7f));
			m_game.m_pEffectSpr[164]->Draw(fix_x - 30, fix_y + 10, ef % 15, SpriteLib::DrawParams::Alpha(0.7f));
			break;
		case 8:
			m_game.m_pEffectSpr[153]->Draw(fix_x + 20, fix_y + 110, ef % 28, SpriteLib::DrawParams::Alpha(0.7f));
			m_game.m_pEffectSpr[164]->Draw(fix_x - 20, fix_y + 16, ef % 15, SpriteLib::DrawParams::Alpha(0.7f));
			break;
		}
	}

	// GM mode (always)
	RenderHelpers::DrawGMEffect(m_game, state, fix_x, fix_y);

	return m_game.m_pSprite[eq.iBodyIndex + (state.m_iDir - 1)]->GetBoundRect();
}

SpriteLib::BoundRect CNpcRenderer::DrawRun(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime)
{
	// NPCs don't normally run, but the function still handles the NPC default case
	SpriteLib::BoundRect invalidRect = {0, -1, 0, 0};
	auto& state = m_game.m_entityState;
	bool bInv = false;
	bool bAdminInvis = false;

	// Invisibility check
	if (RenderHelpers::CheckInvisibility(m_game, state, bInv, bAdminInvis))
		return invalidRect;

	// NPC default in OnRun: all equipment -1, no specific body index pose override
	// Original code has no iBodyIndex assignment for NPC default in OnRun
	EquipmentIndices eq = EquipmentIndices::CalcNpc(state, 1);
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

		// Draw NPC body
		RenderHelpers::DrawNpcLayers(m_game, eq, state, fix_x, fix_y, bInv);

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

SpriteLib::BoundRect CNpcRenderer::DrawAttack(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime)
{
	SpriteLib::BoundRect invalidRect = {0, -1, 0, 0};
	auto& state = m_game.m_entityState;
	bool bInv = false;
	bool bAdminInvis = false;

	// Invisibility check
	if (RenderHelpers::CheckInvisibility(m_game, state, bInv, bAdminInvis))
		return invalidRect;

	// NPC attack pose calculation with per-mob-type overrides
	EquipmentIndices eq;
	if (state.m_appearance.sRawAppr2 != 0)
	{
		eq = EquipmentIndices::CalcNpc(state, 4);
		state.m_iFrame = state.m_appearance.sRawAppr2 - 1;
	}
	else if (state.m_sOwnerType == hb::owner::Wyvern || state.m_sOwnerType == hb::owner::FireWyvern)
		eq = EquipmentIndices::CalcNpc(state, 0);
	else if (state.m_sOwnerType == hb::owner::HBT || state.m_sOwnerType == hb::owner::CT || state.m_sOwnerType == hb::owner::AGC)
		eq = EquipmentIndices::CalcNpc(state, 1);
	else
		eq = EquipmentIndices::CalcNpc(state, 2);
	eq.CalcColors(state);

	// Crusade FOE indicator
	if (m_game.m_bIsCrusadeMode)
		m_game.DrawObjectFOE(sX, sY, state.m_iFrame);

	// Effect auras
	RenderHelpers::DrawEffectAuras(m_game, state, sX, sY);

	if (!bTrans)
	{
		m_game.CheckActiveAura(sX, sY, dwTime, state.m_sOwnerType);

		// Draw NPC body
		RenderHelpers::DrawNpcLayers(m_game, eq, state, sX, sY, bInv);

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

	// Abaddon effects (always)
	RenderHelpers::DrawAbaddonEffects(m_game, state, sX, sY);

	// GM mode (always)
	RenderHelpers::DrawGMEffect(m_game, state, sX, sY);

	return m_game.m_pSprite[eq.iBodyIndex + (state.m_iDir - 1)]->GetBoundRect();
}

SpriteLib::BoundRect CNpcRenderer::DrawAttackMove(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime)
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

	// NPC attack-move uses pose 2
	EquipmentIndices eq = EquipmentIndices::CalcNpc(state, 2);
	eq.CalcColors(state);

	// Frame-based motion offset — same as player
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

		// Draw NPC body
		RenderHelpers::DrawNpcLayers(m_game, eq, state, sX + dx, sY + dy, bInv);

		// Berserk glow
		RenderHelpers::DrawBerserkGlow(m_game, eq, state, sX + dx, sY + dy);

		// Angel + aura
		m_game.DrawAngel(8 + (state.m_iDir - 1), sX + dx + 20, sY + dy - 20, state.m_iFrame % 8, dwTime);
		m_game.CheckActiveAura2(sX + dx, sY + dy, dwTime, state.m_sOwnerType);

		// Dash ghost effect
		if (bDashDraw)
		{
			m_game.m_pSprite[eq.iBodyIndex + (state.m_iDir - 1)]->Draw(sX + dsx, sY + dsy, state.m_iFrame,
				SpriteLib::DrawParams::TintedAlpha(126, 192, 242, 0.7f));
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

SpriteLib::BoundRect CNpcRenderer::DrawMagic(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime)
{
	// NPCs don't have a magic animation in the original code — the original OnMagic
	// only handles player types 1-6. For NPCs, we just draw them stopped.
	return DrawStop(indexX, indexY, sX, sY, bTrans, dwTime);
}

SpriteLib::BoundRect CNpcRenderer::DrawGetItem(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime)
{
	// NPCs don't have a get-item animation in the original code — the original OnGetItem
	// has no iBodyIndex set for the NPC default case. Fall back to stop.
	return DrawStop(indexX, indexY, sX, sY, bTrans, dwTime);
}

SpriteLib::BoundRect CNpcRenderer::DrawDamage(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime)
{
	SpriteLib::BoundRect invalidRect = {0, -1, 0, 0};
	auto& state = m_game.m_entityState;
	bool bInv = false;
	bool bAdminInvis = false;

	// Invisibility check
	if (RenderHelpers::CheckInvisibility(m_game, state, bInv, bAdminInvis))
		return invalidRect;

	// Two-state NPC damage: complex per-mob overrides
	char cFrame = state.m_iFrame;
	int npcPose;

	if (cFrame < 4)
	{
		// Idle state — per-mob overrides
		if (state.m_appearance.sRawAppr2 != 0)
			npcPose = 4; // special frame from sRawAppr2
		else if (state.m_sOwnerType == hb::owner::Wyvern) npcPose = 0;
		else if (state.m_sOwnerType == hb::owner::McGaffin) npcPose = 0;
		else if (state.m_sOwnerType == hb::owner::Perry) npcPose = 0;
		else if (state.m_sOwnerType == hb::owner::Devlin) npcPose = 0;
		else if (state.m_sOwnerType == hb::owner::FireWyvern) npcPose = 0;
		else if (state.m_sOwnerType == hb::owner::Abaddon) npcPose = 2;
		else if (state.m_sOwnerType == hb::owner::HBT) npcPose = 2;
		else if (state.m_sOwnerType == hb::owner::CT) npcPose = 2;
		else if (state.m_sOwnerType == hb::owner::AGC) npcPose = 2;
		else if (state.m_sOwnerType == hb::owner::Gate) npcPose = 0;
		else npcPose = 0;
	}
	else
	{
		cFrame -= 4;
		// Damage recoil state — per-mob overrides
		if (state.m_appearance.sRawAppr2 != 0)
			npcPose = 4;
		else if (state.m_sOwnerType == hb::owner::Wyvern) npcPose = 0;
		else if (state.m_sOwnerType == hb::owner::McGaffin) npcPose = 0;
		else if (state.m_sOwnerType == hb::owner::Perry) npcPose = 0;
		else if (state.m_sOwnerType == hb::owner::Devlin) npcPose = 0;
		else if (state.m_sOwnerType == hb::owner::FireWyvern) npcPose = 0;
		else if (state.m_sOwnerType == hb::owner::Abaddon) npcPose = 2;
		else if (state.m_sOwnerType == hb::owner::HBT) npcPose = 2;
		else if (state.m_sOwnerType == hb::owner::CT) npcPose = 2;
		else if (state.m_sOwnerType == hb::owner::AGC) npcPose = 2;
		else if (state.m_sOwnerType == hb::owner::Gate) npcPose = 1;
		else npcPose = 3;
	}

	EquipmentIndices eq = EquipmentIndices::CalcNpc(state, npcPose);
	eq.CalcColors(state);

	// Apply sRawAppr2 frame override
	if (state.m_appearance.sRawAppr2 != 0)
		cFrame = state.m_appearance.sRawAppr2 - 1;

	state.m_iFrame = cFrame;

	// Crusade FOE indicator
	if (m_game.m_bIsCrusadeMode)
		m_game.DrawObjectFOE(sX, sY, cFrame);

	// Effect auras
	RenderHelpers::DrawEffectAuras(m_game, state, sX, sY);

	if (!bTrans)
	{
		m_game.CheckActiveAura(sX, sY, dwTime, state.m_sOwnerType);

		// Draw NPC body
		RenderHelpers::DrawNpcLayers(m_game, eq, state, sX, sY, bInv);

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

SpriteLib::BoundRect CNpcRenderer::DrawDamageMove(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime)
{
	SpriteLib::BoundRect invalidRect = {0, -1, 0, 0};
	auto& state = m_game.m_entityState;
	bool bInv = false;
	bool bAdminInvis = false;

	// Early return for McGaffin/Perry/Devlin/Abaddon
	if (state.m_sOwnerType == hb::owner::McGaffin ||
		state.m_sOwnerType == hb::owner::Perry ||
		state.m_sOwnerType == hb::owner::Devlin ||
		state.m_sOwnerType == hb::owner::Abaddon)
		return invalidRect;

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

	// Per-mob pose overrides
	int npcPose;
	if (state.m_sOwnerType == hb::owner::Wyvern) npcPose = 0;
	else if (state.m_sOwnerType == hb::owner::FireWyvern) npcPose = 0;
	else if (state.m_sOwnerType == hb::owner::HBT) npcPose = 2;
	else if (state.m_sOwnerType == hb::owner::CT) npcPose = 2;
	else if (state.m_sOwnerType == hb::owner::AGC) npcPose = 2;
	else npcPose = 3;

	EquipmentIndices eq = EquipmentIndices::CalcNpc(state, npcPose);
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

		// Draw NPC body
		RenderHelpers::DrawNpcLayers(m_game, eq, state, fix_x, fix_y, bInv);

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

SpriteLib::BoundRect CNpcRenderer::DrawDying(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime)
{
	SpriteLib::BoundRect invalidRect = {0, -1, 0, 0};
	auto& state = m_game.m_entityState;

	// No invisibility check for dying

	int originalFrame = state.m_iFrame;
	char cFrame = state.m_iFrame;

	// NPC dying: two-state with per-mob overrides
	int npcPose;

	if (cFrame < 4)
	{
		if (state.m_appearance.sRawAppr2 != 0)
			npcPose = 4;
		else if (state.m_sOwnerType == hb::owner::Wyvern) npcPose = 2;
		else if (state.m_sOwnerType == hb::owner::FireWyvern) npcPose = 2;
		else if (state.m_sOwnerType == hb::owner::Abaddon) npcPose = 3;
		else if (state.m_sOwnerType == hb::owner::HBT) npcPose = 3;
		else if (state.m_sOwnerType == hb::owner::CT) npcPose = 3;
		else if (state.m_sOwnerType == hb::owner::AGC) npcPose = 3;
		else if (state.m_sOwnerType == hb::owner::Gate) npcPose = 2;
		else npcPose = 0;

		// Guard tower types: sRawAppr2==0 → cFrame=0
		switch (state.m_sOwnerType) {
		case hb::owner::ArrowGuardTower:
		case hb::owner::CannonGuardTower:
		case hb::owner::ManaCollector:
		case hb::owner::Detector:
		case hb::owner::EnergyShield:
		case hb::owner::GrandMagicGenerator:
		case hb::owner::ManaStone:
			if (state.m_appearance.sRawAppr2 == 0) cFrame = 0;
			break;
		case hb::owner::Catapult: cFrame = 0; break;
		}
	}
	else
	{
		switch (state.m_sOwnerType) {
		case hb::owner::Catapult: cFrame = 0; break;
		default: cFrame -= 4; break;
		}

		if (state.m_appearance.sRawAppr2 != 0)
			npcPose = 4;
		else if (state.m_sOwnerType == hb::owner::Wyvern) npcPose = 2;
		else if (state.m_sOwnerType == hb::owner::FireWyvern) npcPose = 2;
		else if (state.m_sOwnerType == hb::owner::Abaddon) npcPose = 3;
		else if (state.m_sOwnerType == hb::owner::HBT) npcPose = 3;
		else if (state.m_sOwnerType == hb::owner::CT) npcPose = 3;
		else if (state.m_sOwnerType == hb::owner::AGC) npcPose = 3;
		else if (state.m_sOwnerType == hb::owner::Gate) npcPose = 2;
		else npcPose = 4;
	}

	EquipmentIndices eq = EquipmentIndices::CalcNpc(state, npcPose);
	eq.CalcColors(state);

	// Apply sRawAppr2 frame override
	if (state.m_appearance.sRawAppr2 != 0)
		cFrame = state.m_appearance.sRawAppr2 - 1;

	state.m_iFrame = cFrame;

	// Crusade FOE indicator
	if (m_game.m_bIsCrusadeMode)
		m_game.DrawObjectFOE(sX, sY, cFrame);

	// Effect auras
	RenderHelpers::DrawEffectAuras(m_game, state, sX, sY);

	if (!bTrans)
	{
		// Shadow — includes Wyvern/FireWyvern in skip list for dying
		RenderHelpers::DrawShadow(m_game, eq.iBodyIndex, sX, sY, cFrame, false, state.m_sOwnerType);

		// Abaddon death effects
		if (state.m_sOwnerType == hb::owner::Abaddon)
		{
			m_game.m_pEffectSpr[152]->Draw(sX - 80, sY - 15, state.m_iEffectFrame % 27, SpriteLib::DrawParams::Alpha(0.7f));
			m_game.m_pEffectSpr[152]->Draw(sX, sY - 15, state.m_iEffectFrame % 27, SpriteLib::DrawParams::Alpha(0.7f));
			m_game.m_pEffectSpr[152]->Draw(sX - 40, sY, state.m_iEffectFrame % 27, SpriteLib::DrawParams::Alpha(0.7f));
			m_game.m_pEffectSpr[163]->Draw(sX - 90, sY - 80, state.m_iEffectFrame % 12, SpriteLib::DrawParams::Alpha(0.7f));
			m_game.m_pEffectSpr[160]->Draw(sX - 60, sY - 50, state.m_iEffectFrame % 12, SpriteLib::DrawParams::Alpha(0.7f));
			m_game.m_pEffectSpr[161]->Draw(sX - 30, sY - 20, state.m_iEffectFrame % 12, SpriteLib::DrawParams::Alpha(0.7f));
			m_game.m_pEffectSpr[162]->Draw(sX, sY - 100, state.m_iEffectFrame % 12, SpriteLib::DrawParams::Alpha(0.7f));
			m_game.m_pEffectSpr[163]->Draw(sX + 30, sY - 30, state.m_iEffectFrame % 12, SpriteLib::DrawParams::Alpha(0.7f));
			m_game.m_pEffectSpr[162]->Draw(sX + 60, sY - 90, state.m_iEffectFrame % 12, SpriteLib::DrawParams::Alpha(0.7f));
			m_game.m_pEffectSpr[163]->Draw(sX + 90, sY - 50, state.m_iEffectFrame % 12, SpriteLib::DrawParams::Alpha(0.7f));
			switch (state.m_iDir) {
			case 1: m_game.m_pEffectSpr[140]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Alpha(0.7f)); break;
			case 2: m_game.m_pEffectSpr[141]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Alpha(0.7f)); break;
			case 3: m_game.m_pEffectSpr[142]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Alpha(0.7f)); break;
			case 4: m_game.m_pEffectSpr[143]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Alpha(0.7f)); break;
			case 5: m_game.m_pEffectSpr[144]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Alpha(0.7f)); break;
			case 6: m_game.m_pEffectSpr[145]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Alpha(0.7f)); break;
			case 7: m_game.m_pEffectSpr[146]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Alpha(0.7f)); break;
			case 8: m_game.m_pEffectSpr[147]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Alpha(0.7f)); break;
			}
		}
		else if (state.m_sOwnerType == hb::owner::Wyvern)
		{
			m_game.m_pSprite[eq.iBodyIndex + (state.m_iDir - 1)]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Alpha(0.5f));
		}
		else if (state.m_sOwnerType == hb::owner::FireWyvern)
		{
			m_game.m_pSprite[33]->Draw(sX, sY, cFrame, SpriteLib::DrawParams::Alpha(0.5f));
			switch (state.m_iDir) {
			case 1: m_game.m_pEffectSpr[141]->Draw(sX, sY, cFrame + 8, SpriteLib::DrawParams::Alpha(0.7f)); break;
			case 2: m_game.m_pEffectSpr[142]->Draw(sX, sY, cFrame + 8, SpriteLib::DrawParams::Alpha(0.7f)); break;
			case 3: m_game.m_pEffectSpr[143]->Draw(sX, sY, cFrame + 8, SpriteLib::DrawParams::Alpha(0.7f)); break;
			case 4: m_game.m_pEffectSpr[144]->Draw(sX, sY, cFrame + 8, SpriteLib::DrawParams::Alpha(0.7f)); break;
			case 5: m_game.m_pEffectSpr[145]->Draw(sX, sY, cFrame + 8, SpriteLib::DrawParams::Alpha(0.7f)); break;
			case 6: m_game.m_pEffectSpr[146]->Draw(sX, sY, cFrame + 8, SpriteLib::DrawParams::Alpha(0.7f)); break;
			case 7: m_game.m_pEffectSpr[147]->Draw(sX, sY, cFrame + 8, SpriteLib::DrawParams::Alpha(0.7f)); break;
			case 8: m_game.m_pEffectSpr[141]->Draw(sX, sY, cFrame + 8, SpriteLib::DrawParams::Alpha(0.7f)); break;
			}
		}
		else
		{
			// Normal NPC body draw
			RenderHelpers::DrawBody(m_game, eq.iBodyIndex, sX, sY, cFrame, false, state.m_sOwnerType, state.m_status.bFrozen);
		}

		{ auto br = m_game.m_pSprite[eq.iBodyIndex + (state.m_iDir - 1)]->GetBoundRect();
		  m_game.m_rcBodyRect = GameRectangle(br.left, br.top, br.right - br.left, br.bottom - br.top); }

		// Berserk glow
		RenderHelpers::DrawBerserkGlow(m_game, eq, state, sX, sY);

		// Angel + aura — OnDying uses 24+dir-1, ORIGINAL frame
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

SpriteLib::BoundRect CNpcRenderer::DrawDead(int indexX, int indexY, int sX, int sY, bool bTrans, uint32_t dwTime)
{
	SpriteLib::BoundRect invalidRect = {0, -1, 0, 0};
	auto& state = m_game.m_entityState;

	// Wyvern: early return — no corpse
	if (state.m_sOwnerType == hb::owner::Wyvern) return invalidRect;

	// Per-mob frame and pose table
	int iFrame;
	int npcPose;

	switch (state.m_sOwnerType) {
	case hb::owner::Troll:
	case hb::owner::Ogre:
	case hb::owner::Liche:
	case hb::owner::Demon:
	case hb::owner::Frost:
		iFrame = 5;
		npcPose = 4;
		break;

	case hb::owner::Unicorn:
	case hb::owner::WereWolf:
	case hb::owner::LightWarBeetle:
	case hb::owner::GodsHandKnight:
	case hb::owner::GodsHandKnightCK:
	case hb::owner::TempleKnight:
	case hb::owner::BattleGolem:
	case hb::owner::Stalker:
	case hb::owner::HellClaw:
	case hb::owner::TigerWorm:
	case hb::owner::Beholder:
	case hb::owner::DarkElf:
	case hb::owner::Bunny:
	case hb::owner::Cat:
	case hb::owner::GiantFrog:
	case hb::owner::MountainGiant:
	case hb::owner::Ettin:
	case hb::owner::CannibalPlant:
	case hb::owner::Rudolph:
	case hb::owner::DireBoar:
	case hb::owner::Crops:
	case hb::owner::IceGolem:
	case hb::owner::Dragon:
	case hb::owner::Centaur:
	case hb::owner::ClawTurtle:
	case hb::owner::GiantCrayfish:
	case hb::owner::GiLizard:
	case hb::owner::GiTree:
	case hb::owner::MasterOrc:
	case hb::owner::Minaus:
	case hb::owner::Nizie:
	case hb::owner::Tentocle:
	case hb::owner::Sorceress:
	case hb::owner::ATK:
	case hb::owner::MasterElf:
	case hb::owner::DSK:
	case hb::owner::Barbarian:
		iFrame = 7;
		npcPose = 4;
		break;

	case hb::owner::HBT:
	case hb::owner::CT:
	case hb::owner::AGC:
		iFrame = 7;
		npcPose = 3;
		break;

	case hb::owner::Wyvern:
		iFrame = 15;
		npcPose = 2;
		break;

	case hb::owner::FireWyvern:
		iFrame = 7;
		npcPose = 2;
		bTrans = true;
		break;

	case hb::owner::Abaddon:
		iFrame = 0;
		npcPose = 3;
		bTrans = true;
		break;

	case hb::owner::Catapult:
		iFrame = 0;
		npcPose = 4;
		break;

	case hb::owner::Gargoyle:
		iFrame = 11;
		npcPose = 4;
		break;

	case hb::owner::Gate:
		iFrame = 5;
		npcPose = 2;
		break;

	default:
		iFrame = 3;
		npcPose = 4;
		break;
	}

	EquipmentIndices eq = EquipmentIndices::CalcNpc(state, npcPose);
	eq.CalcColors(state);

	if (!bTrans)
	{
		if (state.m_iFrame == -1)
		{
			// Full corpse draw — just-died state
			state.m_iFrame = iFrame;
			RenderHelpers::DrawBody(m_game, eq.iBodyIndex, sX, sY, iFrame, false, state.m_sOwnerType, state.m_status.bFrozen);
		}
		else if (state.m_status.bBerserk)
		{
			// Berserk corpse fade
			m_game.m_pSprite[eq.iBodyIndex + (state.m_iDir - 1)]->Draw(sX, sY, iFrame,
				SpriteLib::DrawParams::TintedAlpha(202 - 4 * state.m_iFrame, 182 - 4 * state.m_iFrame, 182 - 4 * state.m_iFrame, 0.7f));
		}
		else
		{
			// Normal corpse fade
			m_game.m_pSprite[eq.iBodyIndex + (state.m_iDir - 1)]->Draw(sX, sY, iFrame,
				SpriteLib::DrawParams::TintedAlpha(192 - 4 * state.m_iFrame, 192 - 4 * state.m_iFrame, 192 - 4 * state.m_iFrame, 0.7f));
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

	// Abaddon corpse effects
	if (state.m_sOwnerType == hb::owner::Abaddon)
	{
		m_game.Abaddon_corpse(sX, sY);
	}
	else if (state.m_sOwnerType == hb::owner::FireWyvern)
	{
		m_game.m_pEffectSpr[35]->Draw(sX + 20, sY - 15, rand() % 10, SpriteLib::DrawParams::Alpha(0.7f));
	}

	return m_game.m_pSprite[eq.iBodyIndex + (state.m_iDir - 1)]->GetBoundRect();
}

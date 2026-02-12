#include "RenderHelpers.h"
#include "Game.h"
#include "CommonTypes.h"
#include "ConfigManager.h"

// -----------------------------------------------------------------------
// Drawing order arrays (moved from Game.cpp file scope)
// Indexed by direction (1-8). Element [0] is unused.
// -----------------------------------------------------------------------

namespace RenderHelpers
{

// -----------------------------------------------------------------------
// Helper: should this mob type skip shadow rendering?
// -----------------------------------------------------------------------
static bool ShouldSkipShadow(short sOwnerType)
{
	switch (sOwnerType) {
	case hb::shared::owner::Slime:
	case hb::shared::owner::EnergySphere:
	case hb::shared::owner::TigerWorm:
	case hb::shared::owner::Catapult:
	case hb::shared::owner::CannibalPlant:
	case hb::shared::owner::IceGolem:
	case hb::shared::owner::Abaddon:
	case hb::shared::owner::Gate:
		return true;
	default:
		return false;
	}
}

// -----------------------------------------------------------------------
void DrawEquipLayer(CGame& game, int spriteIndex, int sX, int sY, int frame,
                    bool bInv, int colorIndex)
{
	if (spriteIndex == -1) return;

	if (bInv)
	{
		game.m_pSprite[spriteIndex]->Draw(sX, sY, frame, hb::shared::sprite::DrawParams::Alpha(0.25f));
	}
	else if (colorIndex == 0)
	{
		game.m_pSprite[spriteIndex]->Draw(sX, sY, frame);
	}
	else
	{
		auto c = GameColors::Items[colorIndex];
		game.m_pSprite[spriteIndex]->Draw(sX, sY, frame,
			hb::shared::sprite::DrawParams::Tint(c.r, c.g, c.b));
	}
}

// -----------------------------------------------------------------------
void DrawWeapon(CGame& game, const EquipmentIndices& eq, int sX, int sY,
                int weaponFrame, bool bInv)
{
	if (eq.iWeaponIndex == -1) return;

	if (bInv)
	{
		game.m_pSprite[eq.iWeaponIndex]->Draw(sX, sY, weaponFrame, hb::shared::sprite::DrawParams::Alpha(0.25f));
	}
	else if (eq.iWeaponColor == 0)
	{
		game.m_pSprite[eq.iWeaponIndex]->Draw(sX, sY, weaponFrame);
	}
	else
	{
		auto c = GameColors::Weapons[eq.iWeaponColor];
		game.m_pSprite[eq.iWeaponIndex]->Draw(sX, sY, weaponFrame,
			hb::shared::sprite::DrawParams::Tint(c.r, c.g, c.b));
	}

	// DK set glare
	int weaponGlare = eq.iWeaponGlare;
	game.DKGlare(eq.iWeaponColor, eq.iWeaponIndex, &weaponGlare);
	switch (weaponGlare) {
	case 0: break;
	case 1: game.m_pSprite[eq.iWeaponIndex]->Draw(sX, sY, weaponFrame, hb::shared::sprite::DrawParams::AdditiveColored(game.m_iDrawFlag, 0, 0)); break;
	case 2: game.m_pSprite[eq.iWeaponIndex]->Draw(sX, sY, weaponFrame, hb::shared::sprite::DrawParams::AdditiveColored(0, game.m_iDrawFlag, 0)); break;
	case 3: game.m_pSprite[eq.iWeaponIndex]->Draw(sX, sY, weaponFrame, hb::shared::sprite::DrawParams::AdditiveColored(0, 0, game.m_iDrawFlag)); break;
	}
}

// -----------------------------------------------------------------------
void DrawShield(CGame& game, const EquipmentIndices& eq, int sX, int sY,
                int frame, bool bInv)
{
	if (eq.iShieldIndex == -1) return;

	if (bInv)
	{
		game.m_pSprite[eq.iShieldIndex]->Draw(sX, sY, frame, hb::shared::sprite::DrawParams::Alpha(0.25f));
	}
	else if (eq.iShieldColor == 0)
	{
		game.m_pSprite[eq.iShieldIndex]->Draw(sX, sY, frame);
	}
	else
	{
		auto c = GameColors::Items[eq.iShieldColor];
		game.m_pSprite[eq.iShieldIndex]->Draw(sX, sY, frame,
			hb::shared::sprite::DrawParams::Tint(c.r, c.g, c.b));
	}

	// Shield glare
	switch (eq.iShieldGlare) {
	case 0: break;
	case 1:
		// GM sprite (m_pEffectSpr[45]) is only drawn by DrawGMEffect when bGMMode is true
		// fallthrough to case 2 for green-tinted additive shield overlay
	case 2: game.m_pSprite[eq.iShieldIndex]->Draw(sX, sY, frame, hb::shared::sprite::DrawParams::AdditiveColored(0, game.m_iDrawFlag, 0)); break;
	case 3: game.m_pSprite[eq.iShieldIndex]->Draw(sX, sY, frame, hb::shared::sprite::DrawParams::AdditiveColored(0, 0, game.m_iDrawFlag)); break;
	}
}

// -----------------------------------------------------------------------
void DrawShadow(CGame& game, int iBodyDirIndex, int sX, int sY, int frame,
                bool bInv, short sOwnerType)
{
	if (ShouldSkipShadow(sOwnerType)) return;
	if (ConfigManager::Get().GetDetailLevel() == 0) return;
	if (bInv) return;

	game.m_pSprite[iBodyDirIndex]->Draw(sX, sY, frame, hb::shared::sprite::DrawParams::Shadow());
}

// -----------------------------------------------------------------------
void DrawBody(CGame& game, int iBodyDirIndex, int sX, int sY, int frame,
              bool bInv, short sOwnerType, bool bFrozen, bool bAdminInvis)
{
	if (sOwnerType == hb::shared::owner::Abaddon)
	{
		game.m_pSprite[iBodyDirIndex]->Draw(sX, sY, frame, hb::shared::sprite::DrawParams::Alpha(0.5f));
	}
	else if (bAdminInvis)
	{
		game.m_pSprite[iBodyDirIndex]->Draw(sX, sY, frame,
			hb::shared::sprite::DrawParams::TintedAlpha(255, 132, 132, 0.5f));
	}
	else if (bInv)
	{
		game.m_pSprite[iBodyDirIndex]->Draw(sX, sY, frame, hb::shared::sprite::DrawParams::Alpha(0.5f));
	}
	else if (bFrozen)
	{
		game.m_pSprite[iBodyDirIndex]->Draw(sX, sY, frame,
			hb::shared::sprite::DrawParams::Tint(94, 160, 208));
	}
	else
	{
		game.m_pSprite[iBodyDirIndex]->Draw(sX, sY, frame);
	}

	// Capture body bounding rect
	auto br = game.m_pSprite[iBodyDirIndex]->GetBoundRect();
	game.m_rcBodyRect = hb::shared::geometry::GameRectangle(br.left, br.top, br.right - br.left, br.bottom - br.top);
}

// -----------------------------------------------------------------------
// Internal: draw the non-weapon/shield equipment layers in correct order
// -----------------------------------------------------------------------
static void DrawEquipmentStack(CGame& game, const EquipmentIndices& eq,
                               const CEntityRenderState& state, int sX, int sY,
                               bool bInv, const char* mantleOrder, int equipFrameMul = 8)
{
	int dir = state.m_iDir;
	int frame = state.m_iFrame;
	int dirFrame = (dir - 1) * equipFrameMul + frame;

	// Mantle behind body (order 0)
	if (eq.iMantleIndex != -1 && mantleOrder[dir] == 0)
		DrawEquipLayer(game, eq.iMantleIndex, sX, sY, dirFrame, bInv, eq.iMantleColor);

	// Undies
	DrawEquipLayer(game, eq.iUndiesIndex, sX, sY, dirFrame, bInv, 0);

	// Hair (only if no helm)
	if (eq.iHairIndex != -1 && eq.iHelmIndex == -1)
	{
		const auto& hc = GameColors::Hair[state.m_appearance.iHairColor];
		game.m_pSprite[eq.iHairIndex]->Draw(sX, sY, dirFrame, hb::shared::sprite::DrawParams::Tint(hc.r, hc.g, hc.b));
	}

	// Boots before pants if wearing skirt
	if (eq.iSkirtDraw == 1)
		DrawEquipLayer(game, eq.iBootsIndex, sX, sY, dirFrame, bInv, eq.iBootsColor);

	// Pants
	DrawEquipLayer(game, eq.iPantsIndex, sX, sY, dirFrame, bInv, eq.iPantsColor);

	// Arm armor
	DrawEquipLayer(game, eq.iArmArmorIndex, sX, sY, dirFrame, bInv, eq.iArmColor);

	// Boots after pants if not wearing skirt
	if (eq.iSkirtDraw == 0)
		DrawEquipLayer(game, eq.iBootsIndex, sX, sY, dirFrame, bInv, eq.iBootsColor);

	// Body armor
	DrawEquipLayer(game, eq.iBodyArmorIndex, sX, sY, dirFrame, bInv, eq.iArmorColor);

	// Helm
	DrawEquipLayer(game, eq.iHelmIndex, sX, sY, dirFrame, bInv, eq.iHelmColor);

	// Mantle over armor (order 2)
	if (eq.iMantleIndex != -1 && mantleOrder[dir] == 2)
		DrawEquipLayer(game, eq.iMantleIndex, sX, sY, dirFrame, bInv, eq.iMantleColor);

	// Shield + glare
	DrawShield(game, eq, sX, sY, dirFrame, bInv);

	// Mantle in front (order 1)
	if (eq.iMantleIndex != -1 && mantleOrder[dir] == 1)
		DrawEquipLayer(game, eq.iMantleIndex, sX, sY, dirFrame, bInv, eq.iMantleColor);
}

// -----------------------------------------------------------------------
void DrawPlayerLayers(CGame& game, const EquipmentIndices& eq,
                      const CEntityRenderState& state, int sX, int sY,
                      bool bInv, const char* mantleOrder, int equipFrameMul,
                      bool bAdminInvis)
{
	int dir = state.m_iDir;
	int frame = state.m_iFrame;
	int bodyDirIndex = eq.iBodyIndex + (dir - 1);

	if (weapon_draw_order[dir] == 1)
	{
		// Weapon before body
		DrawWeapon(game, eq, sX, sY, frame, bInv);
		DrawShadow(game, bodyDirIndex, sX, sY, frame, bInv, state.m_sOwnerType);

		// Energy sphere light
		if (state.m_sOwnerType == hb::shared::owner::EnergySphere)
			game.m_pEffectSpr[0]->Draw(sX, sY, 1, hb::shared::sprite::DrawParams::Alpha(0.5f));

		DrawBody(game, bodyDirIndex, sX, sY, frame, bInv, state.m_sOwnerType, state.m_status.bFrozen, bAdminInvis);
		DrawEquipmentStack(game, eq, state, sX, sY, bInv, mantleOrder, equipFrameMul);
	}
	else
	{
		// Body before weapon
		DrawShadow(game, bodyDirIndex, sX, sY, frame, bInv, state.m_sOwnerType);

		if (state.m_sOwnerType == hb::shared::owner::EnergySphere)
			game.m_pEffectSpr[0]->Draw(sX, sY, 1, hb::shared::sprite::DrawParams::Alpha(0.5f));

		DrawBody(game, bodyDirIndex, sX, sY, frame, bInv, state.m_sOwnerType, state.m_status.bFrozen, bAdminInvis);
		DrawEquipmentStack(game, eq, state, sX, sY, bInv, mantleOrder, equipFrameMul);
		DrawWeapon(game, eq, sX, sY, frame, bInv);
	}
}

// -----------------------------------------------------------------------
void DrawNpcLayers(CGame& game, const EquipmentIndices& eq,
                   const CEntityRenderState& state, int sX, int sY,
                   bool bInv)
{
	int dir = state.m_iDir;
	int frame = state.m_iFrame;
	int bodyDirIndex = eq.iBodyIndex + (dir - 1);

	DrawShadow(game, bodyDirIndex, sX, sY, frame, bInv, state.m_sOwnerType);

	if (state.m_sOwnerType == hb::shared::owner::EnergySphere)
		game.m_pEffectSpr[0]->Draw(sX, sY, 1, hb::shared::sprite::DrawParams::Alpha(0.5f));

	DrawBody(game, bodyDirIndex, sX, sY, frame, bInv, state.m_sOwnerType, state.m_status.bFrozen);
}

// -----------------------------------------------------------------------
bool CheckInvisibility(CGame& game, const CEntityRenderState& state, bool& bInv, bool& bAdminInvis)
{
	bAdminInvis = false;

	if (hb::shared::owner::IsAlwaysInvisible(state.m_sOwnerType))
		bInv = true;

	if (state.m_status.bInvisibility)
	{
		// Admin invisibility: bInvisibility + bGMMode combo means admin invis
		// Always draw with red-tinted transparency for any viewer that receives this packet
		if (state.m_status.bGMMode)
		{
			bInv = true;
			bAdminInvis = true;
			return false; // Draw with admin invis transparency
		}

		if (state.m_wObjectID == game.m_pPlayer->m_sPlayerObjectID)
			bInv = true;
		else if (IsFriendly(state.m_status.iRelationship))
			bInv = true;
		else
			return true; // Don't draw at all
	}
	return false; // Draw (possibly with bInv transparency)
}

// -----------------------------------------------------------------------
void ApplyDirectionOverride(CEntityRenderState& state)
{
	switch (state.m_sOwnerType) {
	case hb::shared::owner::AirElemental:
		state.m_iDir = 1;
		break;
	case hb::shared::owner::Gate:
		if (state.m_iDir <= 3) state.m_iDir = 3;
		else state.m_iDir = 5;
		break;
	}
}

// -----------------------------------------------------------------------
void DrawName(CGame& game, const CEntityRenderState& state, int sX, int sY)
{
	if (state.m_cName[0] != '\0')
	{
		if (state.IsPlayer())
			game.DrawObjectName(sX, sY, state.m_cName.data(), state.m_status, state.m_wObjectID);
		else
			game.DrawNpcName(sX, sY, state.m_sOwnerType, state.m_status, state.m_sNpcConfigId);
	}
}

// -----------------------------------------------------------------------
void UpdateChat(CGame& game, const CEntityRenderState& state,
                int sX, int sY, int indexX, int indexY)
{
	if (state.m_iChatIndex == 0) return;

	if (game.m_floatingText.IsValid(state.m_iChatIndex, state.m_wObjectID))
	{
		game.m_floatingText.UpdatePosition(state.m_iChatIndex, static_cast<short>(sX), static_cast<short>(sY));
	}
	else
	{
		game.m_pMapData->ClearChatMsg(indexX, indexY);
	}
}

// -----------------------------------------------------------------------
void DrawNpcLight(CGame& game, short sOwnerType, int sX, int sY)
{
	switch (sOwnerType) {
	case hb::shared::owner::ShopKeeper:
	case hb::shared::owner::Gandalf:
	case hb::shared::owner::Howard:
	case hb::shared::owner::Tom:
	case hb::shared::owner::William:
	case hb::shared::owner::Kennedy:
	case hb::shared::owner::Catapult:
	case hb::shared::owner::HBT:
	case hb::shared::owner::Gail:
		game.m_pEffectSpr[0]->Draw(sX, sY, 1, hb::shared::sprite::DrawParams::Alpha(0.5f));
		break;
	}
}

// -----------------------------------------------------------------------
void DrawEffectAuras(CGame& game, const CEntityRenderState& state, int sX, int sY)
{
	if (state.m_iEffectType == 0) return;
	switch (state.m_iEffectType) {
	case 1: game.m_pEffectSpr[26]->Draw(sX, sY, state.m_iEffectFrame, hb::shared::sprite::DrawParams::Alpha(0.5f)); break;
	case 2: game.m_pEffectSpr[27]->Draw(sX, sY, state.m_iEffectFrame, hb::shared::sprite::DrawParams::Alpha(0.5f)); break;
	}
}

// -----------------------------------------------------------------------
void DrawBerserkGlow(CGame& game, const EquipmentIndices& eq, const CEntityRenderState& state,
                     int sX, int sY)
{
	if (!state.m_status.bBerserk) return;
	int bodyDirIndex = eq.iBodyIndex + (state.m_iDir - 1);
	game.m_pSprite[bodyDirIndex]->Draw(sX, sY, state.m_iFrame,
		hb::shared::sprite::DrawParams::AdditiveColored(GameColors::BerserkGlow.r, GameColors::BerserkGlow.g, GameColors::BerserkGlow.b, 0.7f));
}

// -----------------------------------------------------------------------
void DrawAbaddonEffects(CGame& game, const CEntityRenderState& state, int sX, int sY)
{
	if (state.m_sOwnerType != hb::shared::owner::Abaddon) return;

	int randFrame = state.m_iFrame % 12;
	game.m_pEffectSpr[154]->Draw(sX - 50, sY - 50, randFrame, hb::shared::sprite::DrawParams::Alpha(0.7f));
	game.m_pEffectSpr[155]->Draw(sX - 20, sY - 80, randFrame, hb::shared::sprite::DrawParams::Alpha(0.7f));
	game.m_pEffectSpr[156]->Draw(sX + 70, sY - 50, randFrame, hb::shared::sprite::DrawParams::Alpha(0.7f));
	game.m_pEffectSpr[157]->Draw(sX - 30, sY, randFrame, hb::shared::sprite::DrawParams::Alpha(0.7f));
	game.m_pEffectSpr[158]->Draw(sX - 60, sY + 90, randFrame, hb::shared::sprite::DrawParams::Alpha(0.7f));
	game.m_pEffectSpr[159]->Draw(sX + 65, sY + 85, randFrame, hb::shared::sprite::DrawParams::Alpha(0.7f));

	int ef = state.m_iEffectFrame;
	switch (state.m_iDir) {
	case 1:
		game.m_pEffectSpr[153]->Draw(sX, sY + 108, ef % 28, hb::shared::sprite::DrawParams::Alpha(0.7f));
		game.m_pEffectSpr[164]->Draw(sX - 50, sY + 10, ef % 15, hb::shared::sprite::DrawParams::Alpha(0.7f));
		break;
	case 2:
		game.m_pEffectSpr[153]->Draw(sX, sY + 95, ef % 28, hb::shared::sprite::DrawParams::Alpha(0.7f));
		game.m_pEffectSpr[164]->Draw(sX - 70, sY + 10, ef % 15, hb::shared::sprite::DrawParams::Alpha(0.7f));
		break;
	case 3:
		game.m_pEffectSpr[153]->Draw(sX, sY + 105, ef % 28, hb::shared::sprite::DrawParams::Alpha(0.7f));
		game.m_pEffectSpr[164]->Draw(sX - 90, sY + 10, ef % 15, hb::shared::sprite::DrawParams::Alpha(0.7f));
		break;
	case 4:
		game.m_pEffectSpr[153]->Draw(sX - 35, sY + 100, ef % 28, hb::shared::sprite::DrawParams::Alpha(0.7f));
		game.m_pEffectSpr[164]->Draw(sX - 80, sY + 10, ef % 15, hb::shared::sprite::DrawParams::Alpha(0.7f));
		break;
	case 5:
		game.m_pEffectSpr[153]->Draw(sX, sY + 95, ef % 28, hb::shared::sprite::DrawParams::Alpha(0.7f));
		game.m_pEffectSpr[164]->Draw(sX - 65, sY - 5, ef % 15, hb::shared::sprite::DrawParams::Alpha(0.7f));
		break;
	case 6:
		game.m_pEffectSpr[153]->Draw(sX + 45, sY + 95, ef % 28, hb::shared::sprite::DrawParams::Alpha(0.7f));
		game.m_pEffectSpr[164]->Draw(sX - 31, sY + 10, ef % 15, hb::shared::sprite::DrawParams::Alpha(0.7f));
		break;
	case 7:
		game.m_pEffectSpr[153]->Draw(sX + 40, sY + 110, ef % 28, hb::shared::sprite::DrawParams::Alpha(0.7f));
		game.m_pEffectSpr[164]->Draw(sX - 30, sY + 10, ef % 15, hb::shared::sprite::DrawParams::Alpha(0.7f));
		break;
	case 8:
		game.m_pEffectSpr[153]->Draw(sX + 20, sY + 110, ef % 28, hb::shared::sprite::DrawParams::Alpha(0.7f));
		game.m_pEffectSpr[164]->Draw(sX - 20, sY + 16, ef % 15, hb::shared::sprite::DrawParams::Alpha(0.7f));
		break;
	}
}

// -----------------------------------------------------------------------
void DrawGMEffect(CGame& game, const CEntityRenderState& state, int sX, int sY)
{
	if (state.m_status.bGMMode && state.IsPlayer())
		game.m_pEffectSpr[45]->Draw(sX - 13, sY - 34, 0, hb::shared::sprite::DrawParams::Additive(1.0f));
}

// -----------------------------------------------------------------------
void DrawAfkEffect(CGame& game, const CEntityRenderState& state, int sX, int sY, uint32_t dwTime)
{
	if (!state.IsPlayer()) return;
	if (!state.m_status.bAfk) return;

	// effect9.pak sprite 19 = m_pEffectSpr[66 + 19 = 85], 17 frames (0-16)
	constexpr int AFK_SPRITE_INDEX = 85;
	constexpr int AFK_MAX_FRAMES = 17;

	// Note: static frame counter is shared across all AFK entities, so they animate in sync.
	// Per-entity state would require storing frame/time in CEntityRenderState.
	static int s_iAfkFrame = 0;
	static uint32_t s_dwNextFrameTime = 0;

	if (dwTime >= s_dwNextFrameTime)
	{
		s_iAfkFrame = (s_iAfkFrame + 1) % AFK_MAX_FRAMES;
		s_dwNextFrameTime = dwTime + 100 + (rand() % 101); // 100-200ms
	}

	game.m_pEffectSpr[AFK_SPRITE_INDEX]->Draw(sX + 56, sY+32, s_iAfkFrame, hb::shared::sprite::DrawParams::Alpha(0.8f));
}

} // namespace RenderHelpers

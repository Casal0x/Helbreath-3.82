#pragma once

#include "EquipmentIndices.h"

class CGame;
class CEntityRenderState;

// Drawing order arrays — indexed by direction (1-8), element [0] unused.
// _cDrawingOrder: 1 = draw weapon BEFORE body, 0 = draw weapon AFTER body
// _cMantleDrawingOrder: 0 = behind body, 1 = in front, 2 = over armor
// _cMantleDrawingOrderOnRun: same semantics but for running animation
extern char _cDrawingOrder[9];
extern char _cMantleDrawingOrder[9];
extern char _cMantleDrawingOrderOnRun[9];

namespace RenderHelpers
{
	// Draw a single equipment layer (undies, armor, pants, boots, arm, helm, mantle)
	// with optional color tint. Does nothing if spriteIndex == -1.
	// frame: usually (dir-1)*8 + animFrame for equipment layers
	void DrawEquipLayer(CGame& game, int spriteIndex, int sX, int sY, int frame,
	                    bool bInv, int colorIndex);

	// Draw weapon sprite with color tint + DKGlare overlay.
	// weaponFrame: usually just animFrame (direction is baked into weapon index).
	void DrawWeapon(CGame& game, const EquipmentIndices& eq, int sX, int sY,
	                int weaponFrame, bool bInv);

	// Draw shield sprite with color tint + glare overlay.
	// frame: usually (dir-1)*8 + animFrame
	void DrawShield(CGame& game, const EquipmentIndices& eq, int sX, int sY,
	                int frame, bool bInv);

	// Draw body shadow. Checks mob type skip list and detail level.
	// Does nothing for shadow-exempt mob types or when detail level is 0.
	void DrawShadow(CGame& game, int iBodyDirIndex, int sX, int sY, int frame,
	                bool bInv, short sOwnerType);

	// Draw body sprite with frozen tint, invisibility alpha, or Abaddon handling.
	// Also captures the body bounding rect into game.m_rcBodyRect.
	// bAdminInvis: true = render with red-tinted 50% alpha (admin invisible to higher-level GMs)
	void DrawBody(CGame& game, int iBodyDirIndex, int sX, int sY, int frame,
	              bool bInv, short sOwnerType, bool bFrozen, bool bAdminInvis = false);

	// Draw the full player equipment stack in correct z-order.
	// This is the main consolidation function that replaces ~200 lines per DrawObject function.
	// Draws: shadow, weapon/body/equipment/shield in correct order based on _cDrawingOrder[dir].
	// mantleOrder: pointer to _cMantleDrawingOrder or _cMantleDrawingOrderOnRun
	// equipFrameMul: frame multiplier for equipment layers (default 8, OnMagic uses 16, OnGetItem/OnDamage use 4)
	void DrawPlayerLayers(CGame& game, const EquipmentIndices& eq,
	                      const CEntityRenderState& state, int sX, int sY,
	                      bool bInv, const char* mantleOrder, int equipFrameMul = 8,
	                      bool bAdminInvis = false);

	// Draw the full NPC body (just body sprite, no equipment).
	// Same shadow/body/frozen logic as players but simpler.
	void DrawNpcLayers(CGame& game, const EquipmentIndices& eq,
	                   const CEntityRenderState& state, int sX, int sY,
	                   bool bInv);

	// Check invisibility state. Returns true if entity should NOT be drawn at all.
	// Sets bInv to true if entity should be drawn with transparency (friendly invisible, always-invisible).
	// Sets bAdminInvis to true if entity is admin invisible (red-tinted transparency).
	bool CheckInvisibility(CGame& game, const CEntityRenderState& state, bool& bInv, bool& bAdminInvis);

	// Apply single-direction monster overrides (Air Elemental always dir 1, Gate dir 3 or 5).
	void ApplyDirectionOverride(CEntityRenderState& state);

	// Draw entity name (player or NPC).
	void DrawName(CGame& game, const CEntityRenderState& state, int sX, int sY);

	// Update chat message position or clear expired chat.
	// indexX/indexY needed for chat message clearing when message expires.
	void UpdateChat(CGame& game, const CEntityRenderState& state,
	                int sX, int sY, int indexX, int indexY);

	// Draw NPC ground light effect (shopkeepers, guards, etc.)
	void DrawNpcLight(CGame& game, short sOwnerType, int sX, int sY);

	// Draw special ability effect auras (attack/protect effects).
	void DrawEffectAuras(CGame& game, const CEntityRenderState& state, int sX, int sY);

	// Draw berserk glow overlay on body sprite.
	void DrawBerserkGlow(CGame& game, const EquipmentIndices& eq, const CEntityRenderState& state,
	                     int sX, int sY);

	// Draw Abaddon-specific particle effects.
	void DrawAbaddonEffects(CGame& game, const CEntityRenderState& state, int sX, int sY);

	// Draw GM mode crown effect.
	void DrawGMEffect(CGame& game, const CEntityRenderState& state, int sX, int sY);

	// Draw AFK indicator sprite above local player's head.
	void DrawAfkEffect(CGame& game, const CEntityRenderState& state, int sX, int sY, uint32_t dwTime);
}

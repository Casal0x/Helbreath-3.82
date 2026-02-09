// EffectManager.h: Visual effects system manager
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include <cstdint>
#include "SpriteCollection.h"
#include "EffectType.h"
#include "GameConstants.h"

// Forward declarations only - avoid including heavy headers
class CGame;
class CEffect;

class EffectManager
{
public:
	explicit EffectManager(CGame* pGame);
	~EffectManager();

	// Initialization
	void SetEffectSprites(SpriteLib::SpriteCollection& effectSpr);

	// Main API
	void AddEffect(EffectType sType, int sX, int sY, int dX, int dY, char cStartFrame, int iV1 = 1);
	void Update();   // Updates effect frames, removes finished effects
	
	// Render methods (separate for correct draw order: lights before objects, effects after)
	void DrawEffects();     // Draw particle effects (call after DrawObjects)
	void DrawEffectLights(); // Draw lighting effects (call before DrawObjects)

	// Cleanup
	void ClearAllEffects();

private:
	// Private implementation methods (defined in separate .cpp files)
	void AddEffectImpl(EffectType sType, int sX, int sY, int dX, int dY, char cStartFrame, int iV1 = 0);
	void UpdateEffectsImpl();
	void DrawEffectsImpl();
	void DrawEffectLightsImpl();

	CGame* m_pGame;
	CEffect* m_pEffectList[game_limits::max_effects];
	SpriteLib::SpriteCollection* m_pEffectSpr;  // Reference, not owned
};

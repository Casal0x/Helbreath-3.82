// AnimationState.h: Clean animation controller replacing scattered CTile fields
//
// Manages frame advancement, timing, looping, and completion detection
// for entity animations. Caller computes final frame time with modifiers.
//////////////////////////////////////////////////////////////////////

#pragma once

#include <cstdint>

struct AnimationState
{
	// --- Config (set when action changes) ---
	int8_t  cAction        = 0;    // hb::shared::action::Type::Stop, MOVE, RUN, ATTACK, etc.
	int8_t  cDir           = 0;    // Facing direction (1-8)
	int16_t sMaxFrame      = 0;    // Total frame count
	int16_t sFrameTime     = 0;    // Milliseconds per frame (final, after modifiers)
	bool    bLoop          = true; // STOP/MOVE/RUN loop; others play once

	// --- Playback State ---
	int8_t   cCurrentFrame  = 0;
	int8_t   cPreviousFrame = -1;  // -1 forces FrameChanged() true initially
	uint32_t dwLastFrameTime = 0;  // 0 = not started yet
	bool     bFinished       = false;

	// --- Lifecycle ---
	void Reset();
	void SetAction(int8_t action, int8_t dir,
	               int16_t maxFrame, int16_t frameTime, bool loop,
	               int8_t startFrame = 0);
	void SetDirection(int8_t dir);

	// --- Per-frame update. Returns true if frame changed ---
	bool Update(uint32_t dwCurrentTime);

	// --- Queries ---
	bool IsFinished() const    { return bFinished; }
	bool FrameChanged() const  { return cCurrentFrame != cPreviousFrame; }
	bool JustChangedTo(int8_t frame) const {
		return cCurrentFrame == frame && cPreviousFrame != frame;
	}
};

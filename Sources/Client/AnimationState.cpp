// AnimationState.cpp: Animation controller implementation
//
//////////////////////////////////////////////////////////////////////

#include "AnimationState.h"

//=============================================================================
// Reset - Clear all state back to defaults
//=============================================================================
void AnimationState::Reset()
{
	cAction        = 0;
	cDir           = 0;
	sMaxFrame      = 0;
	sFrameTime     = 0;
	bLoop          = true;
	cCurrentFrame  = 0;
	cPreviousFrame = -1;
	dwLastFrameTime = 0;
	bFinished       = false;
}

//=============================================================================
// SetAction - Configure for a new action (resets playback)
//=============================================================================
void AnimationState::SetAction(int8_t action, int8_t dir,
                               int16_t maxFrame, int16_t frameTime, bool loop,
                               int8_t startFrame)
{
	cAction        = action;
	cDir           = dir;
	sMaxFrame      = maxFrame;
	sFrameTime     = frameTime;
	bLoop          = loop;
	cCurrentFrame  = startFrame;
	cPreviousFrame = -1;  // Force FrameChanged() true on first check
	dwLastFrameTime = 0;  // Will be set on first Update()
	bFinished       = false;
}

//=============================================================================
// SetDirection - Change facing without resetting animation
//=============================================================================
void AnimationState::SetDirection(int8_t dir)
{
	cDir = dir;
}

//=============================================================================
// Update - Advance animation based on elapsed time
//
// Preserves original Helbreath frame-advance behavior:
// - First call sets timestamp, shows first frame for full duration
// - Checks elapsed time against sFrameTime
// - Frame skip up to 3 on lag (original catchup behavior)
// - Non-looping: bFinished when past sMaxFrame
// - Looping: wraps to 0
//
// Returns true if frame changed this call
//=============================================================================
bool AnimationState::Update(uint32_t dwCurrentTime)
{
	if (bFinished) return false;
	if (sFrameTime <= 0) return false;

	cPreviousFrame = cCurrentFrame;

	// First call: set timestamp, show first frame for full duration
	if (dwLastFrameTime == 0)
	{
		dwLastFrameTime = dwCurrentTime;
		return false;
	}

	uint32_t elapsed = dwCurrentTime - dwLastFrameTime;
	if (elapsed <= static_cast<uint32_t>(sFrameTime))
		return false;

	// Frame skip on lag (original behavior: skip up to 3 frames)
	if (elapsed >= static_cast<uint32_t>(sFrameTime + sFrameTime))
	{
		int iSkipFrame = static_cast<int>(elapsed / static_cast<uint32_t>(sFrameTime));
		if (iSkipFrame > 3) iSkipFrame = 3;
		cCurrentFrame += static_cast<int8_t>(iSkipFrame);
	}
	else
	{
		cCurrentFrame++;
	}

	dwLastFrameTime = dwCurrentTime;

	// Check frame boundary
	if (cCurrentFrame > sMaxFrame)
	{
		if (bLoop)
		{
			cCurrentFrame = 0;
		}
		else
		{
			bFinished = true;
		}
	}

	return cCurrentFrame != cPreviousFrame;
}

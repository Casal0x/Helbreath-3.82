#include "FrameTiming.h"
#include <cstring>
#include <algorithm>

// Static member initialization
FrameTiming::TimePoint FrameTiming::s_frameStart;
FrameTiming::TimePoint FrameTiming::s_lastFrame;

double FrameTiming::s_deltaTime = 0.0;
double FrameTiming::s_accumulator = 0.0;
uint32_t FrameTiming::s_profileFrameCount = 0;

// Profiling static members
bool FrameTiming::s_profilingEnabled = false;
bool FrameTiming::s_bFrameRendered = false;
FrameTiming::TimePoint FrameTiming::s_stageStart[FrameTiming::STAGE_COUNT];
double FrameTiming::s_stageTimeMS[FrameTiming::STAGE_COUNT] = { 0 };
double FrameTiming::s_stageAccumMS[FrameTiming::STAGE_COUNT] = { 0 };
double FrameTiming::s_stageAvgMS[FrameTiming::STAGE_COUNT] = { 0 };

void FrameTiming::initialize()
{
	s_lastFrame = Clock::now();
	s_deltaTime = 0.0;
	s_accumulator = 0.0;
	s_profileFrameCount = 0;

	s_profilingEnabled = false;
	std::fill(s_stageTimeMS, s_stageTimeMS + STAGE_COUNT, 0.0);
	std::fill(s_stageAccumMS, s_stageAccumMS + STAGE_COUNT, 0.0);
	std::fill(s_stageAvgMS, s_stageAvgMS + STAGE_COUNT, 0.0);
}

void FrameTiming::begin_frame()
{
	s_frameStart = Clock::now();

	// Calculate delta time in seconds
	auto elapsed = std::chrono::duration<double>(s_frameStart - s_lastFrame);
	s_deltaTime = elapsed.count();

	// clamp delta to prevent spiral of death (e.g., debugger pause)
	if (s_deltaTime > 0.25) s_deltaTime = 0.25;

	// reset per-frame profiling times
	s_bFrameRendered = false;
	if (s_profilingEnabled)
	{
		std::fill(s_stageTimeMS, s_stageTimeMS + STAGE_COUNT, 0.0);
		begin_profile(ProfileStage::FrameTotal);
	}
}

void FrameTiming::end_frame()
{
	// End total frame profiling — only count frames that were actually rendered
	if (s_profilingEnabled && s_bFrameRendered)
	{
		end_profile(ProfileStage::FrameTotal);

		// Accumulate for averaging
		for (int i = 0; i < STAGE_COUNT; i++)
		{
			s_stageAccumMS[i] += s_stageTimeMS[i];
		}
		s_profileFrameCount++;
	}

	s_lastFrame = s_frameStart;
	s_accumulator += s_deltaTime;

	// Cap accumulator to prevent drift after long pauses (e.g., debugger, system sleep)
	if (s_accumulator > 2.0) s_accumulator = 0.0;

	// update profile averages every second
	if (s_accumulator >= 1.0)
	{
		s_accumulator -= 1.0;

		if (s_profilingEnabled && s_profileFrameCount > 0)
		{
			for (int i = 0; i < STAGE_COUNT; i++)
			{
				s_stageAvgMS[i] = s_stageAccumMS[i] / s_profileFrameCount;
				s_stageAccumMS[i] = 0.0;
			}
			s_profileFrameCount = 0;
		}
	}
}

double FrameTiming::get_delta_time()
{
	return s_deltaTime;
}

double FrameTiming::get_delta_time_ms()
{
	return s_deltaTime * 1000.0;
}

// Profiling implementation
void FrameTiming::set_profiling_enabled(bool enabled)
{
	if (enabled && !s_profilingEnabled)
	{
		// reset when enabling
		s_profileFrameCount = 0;
		std::fill(s_stageTimeMS, s_stageTimeMS + STAGE_COUNT, 0.0);
		std::fill(s_stageAccumMS, s_stageAccumMS + STAGE_COUNT, 0.0);
		std::fill(s_stageAvgMS, s_stageAvgMS + STAGE_COUNT, 0.0);
	}
	s_profilingEnabled = enabled;
}

bool FrameTiming::is_profiling_enabled()
{
	return s_profilingEnabled;
}

void FrameTiming::set_frame_rendered(bool rendered)
{
	s_bFrameRendered = rendered;
}

void FrameTiming::begin_profile(ProfileStage stage)
{
	if (!s_profilingEnabled) return;
	int idx = static_cast<int>(stage);
	if (idx >= 0 && idx < STAGE_COUNT)
	{
		s_stageStart[idx] = Clock::now();
	}
}

void FrameTiming::end_profile(ProfileStage stage)
{
	if (!s_profilingEnabled) return;
	int idx = static_cast<int>(stage);
	if (idx >= 0 && idx < STAGE_COUNT)
	{
		auto now = Clock::now();
		auto elapsed = std::chrono::duration<double, std::milli>(now - s_stageStart[idx]);
		s_stageTimeMS[idx] += elapsed.count();  // += allows nested/multiple calls
	}
}

double FrameTiming::get_profile_time_ms(ProfileStage stage)
{
	int idx = static_cast<int>(stage);
	if (idx >= 0 && idx < STAGE_COUNT)
		return s_stageTimeMS[idx];
	return 0.0;
}

double FrameTiming::get_profile_avg_time_ms(ProfileStage stage)
{
	int idx = static_cast<int>(stage);
	if (idx >= 0 && idx < STAGE_COUNT)
		return s_stageAvgMS[idx];
	return 0.0;
}

const char* FrameTiming::get_stage_name(ProfileStage stage)
{
	switch (stage)
	{
	case ProfileStage::update:           return "update";
	case ProfileStage::ClearBuffer:      return "ClearBuf";
	case ProfileStage::draw_background:   return "Background";
	case ProfileStage::draw_effect_lights: return "Lights";
	case ProfileStage::draw_objects:      return "Objects";
	case ProfileStage::draw_effects:      return "Effects";
	case ProfileStage::DrawWeather:      return "Weather";
	case ProfileStage::DrawChat:         return "Chat";
	case ProfileStage::DrawDialogs:      return "Dialogs";
	case ProfileStage::DrawMisc:         return "Misc";
	case ProfileStage::Flip:             return "Flip";
	case ProfileStage::FrameTotal:       return "Total";
	default:                             return "Unknown";
	}
}

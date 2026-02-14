#pragma once

#include <chrono>
#include <stdint.h>

// Profiling stage IDs - add new stages here
enum class ProfileStage {
	update,           // UpdateScreen logic
	ClearBuffer,      // ClearBackB4
	draw_background,   // Map tiles
	draw_effect_lights, // Lighting effects
	draw_objects,      // Characters, NPCs, items
	draw_effects,      // Particle effects
	DrawWeather,      // Weather effects
	DrawChat,         // Chat messages
	DrawDialogs,      // Dialog boxes/UI
	DrawMisc,         // Misc rendering (tooltips, cursor, etc.)
	Flip,             // flip to display
	FrameTotal,       // Total frame time
	COUNT             // Must be last
};

// FrameTiming: Per-frame delta timing and stage profiling
//
// FPS and frame counting are handled engine-side (hb::shared::render::IRenderer::get_fps).
// FrameTiming provides: Per-frame delta and per-stage profiling.
//
// Usage:
//   FrameTiming::initialize();  // Call once at startup
//
//   // In game loop:
//   FrameTiming::begin_frame();
//   // ... update and render ...
//   FrameTiming::end_frame();
//
//   double dt = FrameTiming::get_delta_time();  // Seconds since last frame
//
// Profiling Usage:
//   FrameTiming::begin_profile(ProfileStage::draw_objects);
//   draw_objects();
//   FrameTiming::end_profile(ProfileStage::draw_objects);
//
//   double ms = FrameTiming::get_profile_time_ms(ProfileStage::draw_objects);
//
class FrameTiming
{
public:
	static void initialize();
	static void begin_frame();
	static void end_frame();

	// Accessors
	static double get_delta_time();        // Seconds since last frame
	static double get_delta_time_ms();      // Milliseconds since last frame

	// Profiling
	static void set_profiling_enabled(bool enabled);
	static bool is_profiling_enabled();
	static void set_frame_rendered(bool rendered);  // Call after skip check in RenderFrame
	static void begin_profile(ProfileStage stage);
	static void end_profile(ProfileStage stage);
	static double get_profile_time_ms(ProfileStage stage);     // Current frame time
	static double get_profile_avg_time_ms(ProfileStage stage);  // Averaged over ~1 second
	static const char* get_stage_name(ProfileStage stage);

private:
	using Clock = std::chrono::steady_clock;
	using TimePoint = std::chrono::steady_clock::time_point;

	static TimePoint s_frameStart;        // Frame start time
	static TimePoint s_lastFrame;         // Previous frame start

	static double s_deltaTime;            // Current delta in seconds
	static double s_accumulator;          // Time accumulator for profile averaging
	static uint32_t s_profileFrameCount;  // Frames accumulated for averaging

	// Profiling data
	static constexpr int STAGE_COUNT = static_cast<int>(ProfileStage::COUNT);
	static bool s_profilingEnabled;
	static bool s_bFrameRendered;  // True when RenderFrame actually presents
	static TimePoint s_stageStart[STAGE_COUNT];
	static double s_stageTimeMS[STAGE_COUNT];     // Current frame timing
	static double s_stageAccumMS[STAGE_COUNT];    // Accumulated for averaging
	static double s_stageAvgMS[STAGE_COUNT];      // Averaged timing (updated each second)
};

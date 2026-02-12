#pragma once

#include <cstdint>
#include <array>
#include "GameConstants.h"

// Forward declarations only — full headers in .cpp to avoid GlobalDef.h/RenderConstants.h ordering
namespace hb::shared::render { class IRenderer; }
namespace hb::shared::sprite { class SpriteCollection; }
class CCamera;
class CMapData;

struct WeatherParticle
{
	short sX = 0;
	short sY = 0;
	short sBX = 0;
	char cStep = 0;
};

class WeatherManager
{
public:
	static WeatherManager& Get();

	// Lifecycle
	void Initialize();
	void Shutdown();

	// Core API
	void Draw();
	void Update(uint32_t current_time);
	void SetWeather(bool start, char effect_type);
	void DrawThunderEffect(int sX, int sY, int dX, int dY, int rX, int rY, char cType);
	void ResetParticles();

	// Dependencies (call from Screen_Loading after loading effect sprites)
	void SetDependencies(hb::shared::render::IRenderer& renderer,
	                     hb::shared::sprite::SpriteCollection& effect_sprites,
	                     CCamera& camera);
	void SetMapData(CMapData* map_data);
	void SetXmas(bool is_xmas) { m_is_xmas = is_xmas; }

	// Accessors
	char GetEffectType() const { return m_effect_type; }
	char GetWeatherStatus() const { return m_weather_status; }
	void SetWeatherStatus(char status) { m_weather_status = status; }
	bool IsActive() const { return m_is_active; }
	bool IsRaining() const { return m_effect_type >= 1 && m_effect_type <= 3; }
	bool IsSnowing() const { return m_effect_type >= 4; }

	// Ambient light (day/night)
	void SetAmbientLight(char level);
	char GetAmbientLight() const { return m_ambient_light_level; }
	bool IsNight() const { return m_ambient_light_level == 2; }

private:
	WeatherManager() = default;
	~WeatherManager() = default;
	WeatherManager(const WeatherManager&) = delete;
	WeatherManager& operator=(const WeatherManager&) = delete;

	// Particle state
	std::array<WeatherParticle, game_limits::max_weather_objects> m_particles{};

	// Weather state
	bool m_is_active = false;
	char m_effect_type = 0;
	char m_weather_status = 0;
	uint32_t m_last_update_time = 0;
	bool m_is_xmas = false;
	char m_ambient_light_level = 1;

	// Dependencies (non-owning)
	hb::shared::render::IRenderer* m_renderer = nullptr;
	hb::shared::sprite::SpriteCollection* m_effect_sprites = nullptr;
	CCamera* m_camera = nullptr;
	CMapData* m_map_data = nullptr;
};

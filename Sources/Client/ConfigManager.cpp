#include "ConfigManager.h"
#include "GlobalDef.h"
#include "json.hpp"
#include <fstream>
#include <cstring>
#include <cstdlib>

using json = nlohmann::json;

// 4:3 resolutions for validation (must match DialogBox_SysMenu)
static const struct { int w, h; } s_ValidResolutions[] = {
	//{ 640, 480 },
	{ 800, 600 },
	{ 1024, 768 },
	{ 1280, 960 },
	{ 1440, 1080 },
	{ 1920, 1440 }
};
static const int s_NumValidResolutions = sizeof(s_ValidResolutions) / sizeof(s_ValidResolutions[0]);

ConfigManager& ConfigManager::Get()
{
	static ConfigManager instance;
	return instance;
}

void ConfigManager::Initialize()
{
	// Only initialize once - don't reset if already initialized
	if (m_bInitialized)
		return;

	SetDefaults();
	m_bInitialized = true;
	m_bDirty = false;
}

void ConfigManager::Shutdown()
{
	// Auto-save if dirty
	if (m_bDirty)
	{
		Save();
	}
	m_bInitialized = false;
}

void ConfigManager::SetDefaults()
{
	// Shortcut defaults (none assigned)
	m_magicShortcut = -1;
	m_recentShortcut = -1;
	for (int i = 0; i < MAX_SHORTCUTS; i++)
	{
		m_shortcuts[i] = -1;
	}

	// Audio defaults
	m_masterVolume = 100;
	m_soundVolume = 100;
	m_musicVolume = 100;
	m_ambientVolume = 100;
	m_uiVolume = 100;
	m_bMasterEnabled = true;
	m_bSoundEnabled = true;
	m_bMusicEnabled = true;
	m_bAmbientEnabled = true;
	m_bUIEnabled = true;

	// hb::shared::render::Window defaults
	m_windowWidth = 800;
	m_windowHeight = 600;

	// Display/Detail defaults
	m_bShowFPS = false;
	m_bShowLatency = false;
	m_cDetailLevel = 2;
	m_bZoomMap = true;
	m_bDialogTrans = false;
	m_bRunningMode = false;
#ifdef DEF_WINDOWED_MODE
	m_bFullscreen = false;
#else
	m_bFullscreen = true;
#endif
	m_bCaptureMouse = true;
	m_bBorderless = true;
	m_bVSync = false;
	m_iFpsLimit = 60;
	m_bFullscreenStretch = false;
	m_bTileGrid = false;     // Simple tile grid off by default
	m_bPatchingGrid = false; // Patching debug grid off by default

	m_bDirty = false;
}

int ConfigManager::Clamp(int value, int min, int max)
{
	if (value < min) return min;
	if (value > max) return max;
	return value;
}

bool ConfigManager::Load(const char* filename)
{
	std::ifstream file(filename);
	if (!file.is_open())
	{
		// No config file - save defaults and return success
		Save(filename);
		return true;
	}

	try
	{
		json j = json::parse(file);

		// Shortcuts
		if (j.contains("shortcuts"))
		{
			auto& shortcuts = j["shortcuts"];
			if (shortcuts.contains("magic"))
			{
				int slot = shortcuts["magic"].get<int>();
				m_magicShortcut = (slot >= 0 && slot < MAX_MAGIC_SLOT) ? static_cast<short>(slot) : -1;
			}
			if (shortcuts.contains("slots") && shortcuts["slots"].is_array())
			{
				auto& slots = shortcuts["slots"];
				for (size_t i = 0; i < slots.size() && i < MAX_SHORTCUTS; i++)
				{
					int slot = slots[i].get<int>();
					m_shortcuts[i] = (slot >= 0 && slot < MAX_SHORTCUT_SLOT) ? static_cast<short>(slot) : -1;
				}
			}
		}

		// Audio settings
		if (j.contains("audio"))
		{
			auto& audio = j["audio"];
			if (audio.contains("masterVolume"))
			{
				m_masterVolume = Clamp(audio["masterVolume"].get<int>(), 0, 100);
			}
			if (audio.contains("soundVolume"))
			{
				m_soundVolume = Clamp(audio["soundVolume"].get<int>(), 0, 100);
			}
			if (audio.contains("musicVolume"))
			{
				m_musicVolume = Clamp(audio["musicVolume"].get<int>(), 0, 100);
			}
			if (audio.contains("ambientVolume"))
			{
				m_ambientVolume = Clamp(audio["ambientVolume"].get<int>(), 0, 100);
			}
			if (audio.contains("uiVolume"))
			{
				m_uiVolume = Clamp(audio["uiVolume"].get<int>(), 0, 100);
			}
			if (audio.contains("masterEnabled"))
			{
				m_bMasterEnabled = audio["masterEnabled"].get<bool>();
			}
			if (audio.contains("soundEnabled"))
			{
				m_bSoundEnabled = audio["soundEnabled"].get<bool>();
			}
			if (audio.contains("musicEnabled"))
			{
				m_bMusicEnabled = audio["musicEnabled"].get<bool>();
			}
			if (audio.contains("ambientEnabled"))
			{
				m_bAmbientEnabled = audio["ambientEnabled"].get<bool>();
			}
			if (audio.contains("uiEnabled"))
			{
				m_bUIEnabled = audio["uiEnabled"].get<bool>();
			}
		}

		// hb::shared::render::Window settings
		if (j.contains("window"))
		{
			auto& window = j["window"];
			if (window.contains("width"))
			{
				m_windowWidth = Clamp(window["width"].get<int>(), 640, 3840);
			}
			if (window.contains("height"))
			{
				m_windowHeight = Clamp(window["height"].get<int>(), 480, 2160);
			}
		}

		// Display/Detail settings
		if (j.contains("display"))
		{
			auto& display = j["display"];
			if (display.contains("showFps"))
			{
				m_bShowFPS = display["showFps"].get<bool>();
			}
			if (display.contains("showLatency"))
			{
				m_bShowLatency = display["showLatency"].get<bool>();
			}
			if (display.contains("detailLevel"))
			{
				m_cDetailLevel = Clamp(display["detailLevel"].get<int>(), 0, 2);
			}
			if (display.contains("zoomMap"))
			{
				m_bZoomMap = display["zoomMap"].get<bool>();
			}
			if (display.contains("dialogTransparency"))
			{
				m_bDialogTrans = display["dialogTransparency"].get<bool>();
			}
			if (display.contains("runningMode"))
			{
				m_bRunningMode = display["runningMode"].get<bool>();
			}
			if (display.contains("fullscreen"))
			{
				m_bFullscreen = display["fullscreen"].get<bool>();
			}
			if (display.contains("captureMouse"))
			{
				m_bCaptureMouse = display["captureMouse"].get<bool>();
			}
			if (display.contains("borderless"))
			{
				m_bBorderless = display["borderless"].get<bool>();
			}
			if (display.contains("tileGrid"))
			{
				m_bTileGrid = display["tileGrid"].get<bool>();
			}
			if (display.contains("patchingGrid"))
			{
				m_bPatchingGrid = display["patchingGrid"].get<bool>();
			}
			if (display.contains("vsync"))
			{
				m_bVSync = display["vsync"].get<bool>();
			}
			if (display.contains("fpsLimit"))
			{
				m_iFpsLimit = display["fpsLimit"].get<int>();
				if (m_iFpsLimit < 0) m_iFpsLimit = 0;
			}
			if (display.contains("fullscreenStretch"))
			{
				m_bFullscreenStretch = display["fullscreenStretch"].get<bool>();
			}
		}

		// Validate resolution to nearest 4:3 option
		bool bValidResolution = false;
		for (int i = 0; i < s_NumValidResolutions; i++) {
			if (m_windowWidth == s_ValidResolutions[i].w && m_windowHeight == s_ValidResolutions[i].h) {
				bValidResolution = true;
				break;
			}
		}
		if (!bValidResolution) {
			// Find nearest 4:3 resolution
			int bestIndex = 0;
			int bestDiff = abs(s_ValidResolutions[0].w - m_windowWidth) + abs(s_ValidResolutions[0].h - m_windowHeight);
			for (int i = 1; i < s_NumValidResolutions; i++) {
				int diff = abs(s_ValidResolutions[i].w - m_windowWidth) + abs(s_ValidResolutions[i].h - m_windowHeight);
				if (diff < bestDiff) {
					bestDiff = diff;
					bestIndex = i;
				}
			}
			m_windowWidth = s_ValidResolutions[bestIndex].w;
			m_windowHeight = s_ValidResolutions[bestIndex].h;
			m_bDirty = true; // Mark dirty so corrected value is saved
		}
	}
	catch (const json::exception&)
	{
		// Parse error - keep defaults
		return false;
	}

	// Save immediately to persist any defaults for new keys or corrected values
	m_bDirty = true;
	Save();
	return true;
}

bool ConfigManager::Save(const char* filename)
{
	json j;

	// Server settings
	j["server"]["address"] = DEF_SERVER_IP;
	j["server"]["port"] = DEF_SERVER_PORT;
	j["server"]["gamePort"] = DEF_GSERVER_PORT;

	// Shortcuts
	j["shortcuts"]["magic"] = m_magicShortcut;
	j["shortcuts"]["slots"] = json::array();
	for (int i = 0; i < MAX_SHORTCUTS; i++)
	{
		j["shortcuts"]["slots"].push_back(m_shortcuts[i]);
	}

	// Audio settings
	j["audio"]["masterVolume"] = m_masterVolume;
	j["audio"]["soundVolume"] = m_soundVolume;
	j["audio"]["musicVolume"] = m_musicVolume;
	j["audio"]["ambientVolume"] = m_ambientVolume;
	j["audio"]["uiVolume"] = m_uiVolume;
	j["audio"]["masterEnabled"] = m_bMasterEnabled;
	j["audio"]["soundEnabled"] = m_bSoundEnabled;
	j["audio"]["musicEnabled"] = m_bMusicEnabled;
	j["audio"]["ambientEnabled"] = m_bAmbientEnabled;
	j["audio"]["uiEnabled"] = m_bUIEnabled;

	// hb::shared::render::Window settings
	j["window"]["width"] = m_windowWidth;
	j["window"]["height"] = m_windowHeight;

	// Display/Detail settings
	j["display"]["showFps"] = m_bShowFPS;
	j["display"]["showLatency"] = m_bShowLatency;
	j["display"]["detailLevel"] = m_cDetailLevel;
	j["display"]["zoomMap"] = m_bZoomMap;
	j["display"]["dialogTransparency"] = m_bDialogTrans;
	j["display"]["runningMode"] = m_bRunningMode;
	j["display"]["fullscreen"] = m_bFullscreen;
	j["display"]["captureMouse"] = m_bCaptureMouse;
	j["display"]["borderless"] = m_bBorderless;
	j["display"]["tileGrid"] = m_bTileGrid;
	j["display"]["patchingGrid"] = m_bPatchingGrid;
	j["display"]["vsync"] = m_bVSync;
	j["display"]["fpsLimit"] = m_iFpsLimit;
	j["display"]["fullscreenStretch"] = m_bFullscreenStretch;

	std::ofstream file(filename);
	if (!file.is_open())
	{
		return false;
	}

	file << j.dump(4); // Pretty print with 4-space indent
	m_bDirty = false;
	return true;
}

void ConfigManager::SetMagicShortcut(short slot)
{
	if (slot >= -1 && slot < MAX_MAGIC_SLOT)
	{
		if (m_magicShortcut != slot)
		{
			m_magicShortcut = slot;
			m_bDirty = true;
		}
	}
}

short ConfigManager::GetShortcut(int index) const
{
	if (index >= 0 && index < MAX_SHORTCUTS)
	{
		return m_shortcuts[index];
	}
	return -1;
}

void ConfigManager::SetShortcut(int index, short slot)
{
	if (index >= 0 && index < MAX_SHORTCUTS)
	{
		if (slot >= -1 && slot < MAX_SHORTCUT_SLOT)
		{
			if (m_shortcuts[index] != slot)
			{
				m_shortcuts[index] = slot;
				m_bDirty = true;
			}
		}
	}
}

void ConfigManager::SetMasterVolume(int volume)
{
	volume = Clamp(volume, 0, 100);
	if (m_masterVolume != volume)
	{
		m_masterVolume = volume;
		m_bDirty = true;
		Save();
	}
}

void ConfigManager::SetMasterEnabled(bool enabled)
{
	if (m_bMasterEnabled != enabled)
	{
		m_bMasterEnabled = enabled;
		m_bDirty = true;
		Save();
	}
}

void ConfigManager::SetSoundVolume(int volume)
{
	volume = Clamp(volume, 0, 100);
	if (m_soundVolume != volume)
	{
		m_soundVolume = volume;
		m_bDirty = true;
		Save();
	}
}

void ConfigManager::SetMusicVolume(int volume)
{
	volume = Clamp(volume, 0, 100);
	if (m_musicVolume != volume)
	{
		m_musicVolume = volume;
		m_bDirty = true;
		Save();
	}
}

void ConfigManager::SetSoundEnabled(bool enabled)
{
	if (m_bSoundEnabled != enabled)
	{
		m_bSoundEnabled = enabled;
		m_bDirty = true;
		Save();
	}
}

void ConfigManager::SetMusicEnabled(bool enabled)
{
	if (m_bMusicEnabled != enabled)
	{
		m_bMusicEnabled = enabled;
		m_bDirty = true;
		Save();
	}
}

void ConfigManager::SetAmbientVolume(int volume)
{
	volume = Clamp(volume, 0, 100);
	if (m_ambientVolume != volume)
	{
		m_ambientVolume = volume;
		m_bDirty = true;
		Save();
	}
}

void ConfigManager::SetUIVolume(int volume)
{
	volume = Clamp(volume, 0, 100);
	if (m_uiVolume != volume)
	{
		m_uiVolume = volume;
		m_bDirty = true;
		Save();
	}
}

void ConfigManager::SetAmbientEnabled(bool enabled)
{
	if (m_bAmbientEnabled != enabled)
	{
		m_bAmbientEnabled = enabled;
		m_bDirty = true;
		Save();
	}
}

void ConfigManager::SetUIEnabled(bool enabled)
{
	if (m_bUIEnabled != enabled)
	{
		m_bUIEnabled = enabled;
		m_bDirty = true;
		Save();
	}
}

void ConfigManager::SetWindowSize(int width, int height)
{
	// Validate to nearest 4:3 resolution
	bool bValid = false;
	for (int i = 0; i < s_NumValidResolutions; i++) {
		if (width == s_ValidResolutions[i].w && height == s_ValidResolutions[i].h) {
			bValid = true;
			break;
		}
	}
	if (!bValid) {
		// Snap to nearest valid resolution
		int bestIndex = 0;
		int bestDiff = abs(s_ValidResolutions[0].w - width) + abs(s_ValidResolutions[0].h - height);
		for (int i = 1; i < s_NumValidResolutions; i++) {
			int diff = abs(s_ValidResolutions[i].w - width) + abs(s_ValidResolutions[i].h - height);
			if (diff < bestDiff) {
				bestDiff = diff;
				bestIndex = i;
			}
		}
		width = s_ValidResolutions[bestIndex].w;
		height = s_ValidResolutions[bestIndex].h;
	}

	if (m_windowWidth != width || m_windowHeight != height)
	{
		m_windowWidth = width;
		m_windowHeight = height;
		m_bDirty = true;
		Save();
	}
}

void ConfigManager::SetShowFpsEnabled(bool enabled)
{
	if (m_bShowFPS != enabled)
	{
		m_bShowFPS = enabled;
		m_bDirty = true;
		Save();
	}
}

void ConfigManager::SetShowLatencyEnabled(bool enabled)
{
	if (m_bShowLatency != enabled)
	{
		m_bShowLatency = enabled;
		m_bDirty = true;
		Save();
	}
}

void ConfigManager::SetDetailLevel(int level)
{
	level = Clamp(level, 0, 2);
	if (m_cDetailLevel != level)
	{
		m_cDetailLevel = level;
		m_bDirty = true;
		Save();
	}
}

void ConfigManager::SetZoomMapEnabled(bool enabled)
{
	if (m_bZoomMap != enabled)
	{
		m_bZoomMap = enabled;
		m_bDirty = true;
		Save();
	}
}

void ConfigManager::SetDialogTransparencyEnabled(bool enabled)
{
	if (m_bDialogTrans != enabled)
	{
		m_bDialogTrans = enabled;
		m_bDirty = true;
		Save();
	}
}

void ConfigManager::SetRunningModeEnabled(bool enabled)
{
	if (m_bRunningMode != enabled)
	{
		m_bRunningMode = enabled;
		m_bDirty = true;
		Save();
	}
}

void ConfigManager::SetFullscreenEnabled(bool enabled)
{
	if (m_bFullscreen != enabled)
	{
		m_bFullscreen = enabled;
		m_bDirty = true;
		Save();
	}
}

void ConfigManager::SetMouseCaptureEnabled(bool enabled)
{
	if (m_bCaptureMouse != enabled)
	{
		m_bCaptureMouse = enabled;
		m_bDirty = true;
		Save();
	}
}

void ConfigManager::SetBorderlessEnabled(bool enabled)
{
	if (m_bBorderless != enabled)
	{
		m_bBorderless = enabled;
		m_bDirty = true;
		Save();
	}
}

void ConfigManager::SetTileGridEnabled(bool enabled)
{
	if (m_bTileGrid != enabled)
	{
		m_bTileGrid = enabled;
		m_bDirty = true;
		Save();
	}
}

void ConfigManager::SetPatchingGridEnabled(bool enabled)
{
	if (m_bPatchingGrid != enabled)
	{
		m_bPatchingGrid = enabled;
		m_bDirty = true;
		Save();
	}
}

void ConfigManager::SetVSyncEnabled(bool enabled)
{
	if (m_bVSync != enabled)
	{
		m_bVSync = enabled;
		m_bDirty = true;
		Save();
	}
}

void ConfigManager::SetFpsLimit(int limit)
{
	if (limit < 0) limit = 0;
	if (m_iFpsLimit != limit)
	{
		m_iFpsLimit = limit;
		m_bDirty = true;
		Save();
	}
}

void ConfigManager::SetFullscreenStretchEnabled(bool enabled)
{
	if (m_bFullscreenStretch != enabled)
	{
		m_bFullscreenStretch = enabled;
		m_bDirty = true;
		Save();
	}
}

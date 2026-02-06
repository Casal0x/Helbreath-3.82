#pragma once

#include <cstdint>

// Maximum shortcuts matching game limits
static const int MAX_SHORTCUTS = 5;
static const int MAX_MAGIC_SLOT = 100;
static const int MAX_SHORTCUT_SLOT = 200;

class ConfigManager
{
public:
	static ConfigManager& Get();

	// Lifecycle
	void Initialize();
	void Shutdown();

	// File operations - replaces registry-based ReadSettings/WriteSettings
	bool Load(const char* filename = "settings.json");
	bool Save(const char* filename = "settings.json");

	// Server address - uses DEF_SERVER_IP/PORT macros as defaults
	//const char* GetServerAddress() const { return DEF_SERVER_IP; }
	//int GetServerPort() const { return DEF_SERVER_PORT; }
	//int GetGameServerPort() const { return DEF_GSERVER_PORT; }

	// Shortcuts - replaces registry storage
	// Magic shortcut: -1 = none, 0-99 = valid slot
	short GetMagicShortcut() const { return m_magicShortcut; }
	void SetMagicShortcut(short slot);

	// Item/Skill shortcuts: -1 = none, 0-199 = valid slot
	short GetShortcut(int index) const;
	void SetShortcut(int index, short slot);

	// Recent shortcut (runtime only, not persisted)
	short GetRecentShortcut() const { return m_recentShortcut; }
	void SetRecentShortcut(short slot) { m_recentShortcut = slot; }

	// Audio settings - adds persistence to existing runtime values
	int GetMasterVolume() const { return m_masterVolume; }
	int GetSoundVolume() const { return m_soundVolume; }
	int GetMusicVolume() const { return m_musicVolume; }
	int GetAmbientVolume() const { return m_ambientVolume; }
	int GetUIVolume() const { return m_uiVolume; }
	bool IsMasterEnabled() const { return m_bMasterEnabled; }
	bool IsSoundEnabled() const { return m_bSoundEnabled; }
	bool IsMusicEnabled() const { return m_bMusicEnabled; }
	bool IsAmbientEnabled() const { return m_bAmbientEnabled; }
	bool IsUIEnabled() const { return m_bUIEnabled; }

	void SetMasterVolume(int volume);
	void SetSoundVolume(int volume);
	void SetMusicVolume(int volume);
	void SetAmbientVolume(int volume);
	void SetUIVolume(int volume);
	void SetMasterEnabled(bool enabled);
	void SetSoundEnabled(bool enabled);
	void SetMusicEnabled(bool enabled);
	void SetAmbientEnabled(bool enabled);
	void SetUIEnabled(bool enabled);

	// Window/Resolution settings
	int GetWindowWidth() const { return m_windowWidth; }
	int GetWindowHeight() const { return m_windowHeight; }
	void SetWindowSize(int width, int height);

	// Display/Detail settings
	bool IsShowFpsEnabled() const { return m_bShowFPS; }
	bool IsShowLatencyEnabled() const { return m_bShowLatency; }
	int GetDetailLevel() const { return m_cDetailLevel; }
	bool IsZoomMapEnabled() const { return m_bZoomMap; }
	bool IsDialogTransparencyEnabled() const { return m_bDialogTrans; }
	bool IsRunningModeEnabled() const { return m_bRunningMode; }
	bool IsFullscreenEnabled() const { return m_bFullscreen; }

	void SetShowFpsEnabled(bool enabled);
	void SetShowLatencyEnabled(bool enabled);
	void SetDetailLevel(int level);
	void SetZoomMapEnabled(bool enabled);
	void SetDialogTransparencyEnabled(bool enabled);
	void SetRunningModeEnabled(bool enabled);
	void SetFullscreenEnabled(bool enabled);

	// Mouse capture
	bool IsMouseCaptureEnabled() const { return m_bCaptureMouse; }
	void SetMouseCaptureEnabled(bool enabled);

	// Tile grid overlay (simple dark lines)
	bool IsTileGridEnabled() const { return m_bTileGrid; }
	void SetTileGridEnabled(bool enabled);

	// Patching grid overlay (debug with zone colors)
	bool IsPatchingGridEnabled() const { return m_bPatchingGrid; }
	void SetPatchingGridEnabled(bool enabled);

	// Borderless window
	bool IsBorderlessEnabled() const { return m_bBorderless; }
	void SetBorderlessEnabled(bool enabled);

	// Quick Actions - always enabled (pickup during movement, 95% unlock, responsive stops)
	bool IsQuickActionsEnabled() const { return true; }

	// Base resolution (640x480 or 800x600) - determines logical render size
	int GetBaseResolutionWidth() const { return m_baseResolutionWidth; }
	int GetBaseResolutionHeight() const { return m_baseResolutionHeight; }
	void SetBaseResolution(int width, int height);

	// Dirty flag - indicates unsaved changes
	bool IsDirty() const { return m_bDirty; }
	void MarkClean() { m_bDirty = false; }

private:
	ConfigManager() = default;
	~ConfigManager() = default;
	ConfigManager(const ConfigManager&) = delete;
	ConfigManager& operator=(const ConfigManager&) = delete;

	void SetDefaults();
	int Clamp(int value, int min, int max);

	// Shortcuts (matches m_sMagicShortCut, m_sShortCut[5], m_sRecentShortCut)
	short m_magicShortcut;
	short m_shortcuts[MAX_SHORTCUTS];
	short m_recentShortcut;

	// Audio (matches m_cSoundVolume, m_cMusicVolume, m_bSoundStat, m_bMusicStat)
	int m_masterVolume;
	int m_soundVolume;
	int m_musicVolume;
	int m_ambientVolume;
	int m_uiVolume;
	bool m_bMasterEnabled;
	bool m_bSoundEnabled;
	bool m_bMusicEnabled;
	bool m_bAmbientEnabled;
	bool m_bUIEnabled;

	// Window/Resolution
	int m_windowWidth;
	int m_windowHeight;

	// Display/Detail
	bool m_bShowFPS;
	bool m_bShowLatency;
	int m_cDetailLevel;
	bool m_bZoomMap;
	bool m_bDialogTrans;
	bool m_bRunningMode;
	bool m_bFullscreen;
	bool m_bCaptureMouse;
	bool m_bBorderless;
	bool m_bTileGrid;
	bool m_bPatchingGrid;

	// Base resolution (640x480 or 800x600)
	int m_baseResolutionWidth;
	int m_baseResolutionHeight;

	// State
	bool m_bDirty;
	bool m_bInitialized;
};

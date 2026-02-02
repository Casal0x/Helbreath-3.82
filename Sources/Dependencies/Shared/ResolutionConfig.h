#pragma once

// ResolutionConfig - Singleton that provides resolution-dependent values
//
// This allows the game to run at different base resolutions (640x480 or 800x600)
// while keeping all resolution-dependent calculations centralized.
//
// The base resolution determines:
// - Logical render target size
// - Number of visible tiles
//
// Window resolution (display size) is separate and can scale the base resolution.

class ResolutionConfig
{
public:
	// Initialize with base and window resolution - must be called before Get()
	// baseWidth/baseHeight: logical game resolution (640x480 or 800x600)
	// windowWidth/windowHeight: actual window size for centering calculations
	static void Initialize(int baseWidth, int baseHeight, int windowWidth, int windowHeight);

	// Get the singleton instance (must call Initialize first)
	static ResolutionConfig& Get();

	// Update window size (for when window is resized)
	void SetWindowSize(int windowWidth, int windowHeight);

	// Update base resolution (requires game restart for full effect, but updates calculations)
	void SetBaseResolution(int baseWidth, int baseHeight);

	// Logical resolution (render target size)
	int LogicalWidth() const { return m_width; }
	int LogicalHeight() const { return m_height; }

	// Screen offset for centering content in window
	// Auto-calculated based on window size vs logical size
	int ScreenX() const { return m_screenX; }
	int ScreenY() const { return m_screenY; }

	// Max coordinates for bounds checking
	int LogicalMaxX() const { return m_width - 1; }
	int LogicalMaxY() const { return m_height - 1; }

	// Tile counts based on resolution
	// Tiles are 32x32 pixels
	int ViewTileWidth() const { return m_width / 32; }
	// ViewTileHeight accounts for HUD at bottom (53px)
	// 640x480: (480-53)/32 = 13 tiles, 800x600: (600-53)/32 = 17 tiles
	int ViewTileHeight() const { return (m_height - 53) / 32; }

	// Center tile position (player position on screen)
	// X is symmetric: player centered horizontally
	int ViewCenterTileX() const { return ViewTileWidth() / 2; }
	// Y includes +1 offset for player sprite positioning (previously added in camera calc)
	// This gives symmetric view: 800x600 = 8 up/8 down, 640x480 = 6 up/6 down
	int ViewCenterTileY() const { return ViewTileHeight() / 2 + 1; }

	// HUD panel dimensions
	int IconPanelWidth() const { return m_width; }
	int IconPanelHeight() const { return 53; }
	int IconPanelOffsetX() const { return 0; }

	// Chat input position
	int ChatInputX() const { return 10; }
	int ChatInputY() const { return m_height - IconPanelHeight() - 16; }

	// Event list base Y position
	int EventList2BaseY() const { return ChatInputY() - (6 * 15) - 4; }

	// Level up text position
	int LevelUpTextX() const { return m_width - 90; }
	int LevelUpTextY() const { return EventList2BaseY() + (5 * 15); }

	// Check if using high resolution mode
	bool IsHighResolution() const { return m_width >= 800; }

	// Menu offset - for centering 640x480 menu backgrounds in larger resolutions
	// At 800x600: MenuOffsetX = (800-640)/2 = 80, MenuOffsetY = (600-480)/2 = 60
	// At 640x480: MenuOffsetX = 0, MenuOffsetY = 0
	int MenuOffsetX() const { return 0; } //{ return (m_width - 640) / 2; }
	int MenuOffsetY() const { return 0; } //{ return (m_height - 480) / 2; }

private:
	ResolutionConfig() = default;
	~ResolutionConfig() = default;
	ResolutionConfig(const ResolutionConfig&) = delete;
	ResolutionConfig& operator=(const ResolutionConfig&) = delete;

	void RecalculateScreenOffset();

	int m_width = 640;
	int m_height = 480;
	int m_windowWidth = 640;
	int m_windowHeight = 480;
	int m_screenX = 0;
	int m_screenY = 0;

	static bool s_bInitialized;
};

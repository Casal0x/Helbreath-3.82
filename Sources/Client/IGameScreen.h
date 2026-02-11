// IGameScreen.h: Interface for game screens with lifecycle management
//
// Provides an object-oriented approach to screen management with:
// - Lifecycle methods: on_initialize, on_uninitialize, on_update, on_render
// - Type identification via SCREEN_TYPE macro
// - Helper methods that delegate to CGame for common operations
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "CommonTypes.h"
#include <cstdint>

class CGame;
class GameModeManager;

// Macro to define screen type - converts class name to string automatically
// Usage: Add SCREEN_TYPE(ClassName) in the public section of your screen class
#define SCREEN_TYPE(ClassName) \
    static constexpr ScreenTypeId screen_type_id = #ClassName; \
    ScreenTypeId get_type_id() const override { return screen_type_id; }

class IGameScreen
{
public:
    using ScreenTypeId = const char*;

    explicit IGameScreen(CGame* pGame);
    virtual ~IGameScreen() = default;

    // Prevent copying/moving - screens own state and should not be copied
    IGameScreen(const IGameScreen&) = delete;
    IGameScreen& operator=(const IGameScreen&) = delete;
    IGameScreen(IGameScreen&&) = delete;
    IGameScreen& operator=(IGameScreen&&) = delete;

    // ============== Core Lifecycle (pure virtual) ==============
    // These must be implemented by each screen

    // Called once when screen becomes active (after previous screen's on_uninitialize)
    virtual void on_initialize() = 0;

    // Called once when screen is about to be replaced (before new screen's on_initialize)
    virtual void on_uninitialize() = 0;

    // Called each frame to update logic, handle input, process state changes
    virtual void on_update() = 0;

    // Called each frame to render the screen (after BeginFrame, before EndFrame)
    virtual void on_render() = 0;

    // Returns the screen type identifier for runtime type checking
    virtual ScreenTypeId get_type_id() const = 0;

    // Whether the overlay system should draw a full-screen dim behind this overlay.
    // Override to false for overlays that draw their own background (e.g. DevConsole).
    virtual bool wants_background_dim() const { return true; }

    // Called when a server response arrives in LogResponseHandler.
    // Return true if this screen handled the response (stops further processing),
    // false to fall through to default handling. Optional — not all screens need this.
    virtual bool on_net_response(uint16_t wResponseType, char* pData) { return false; }

protected:
    // ============== Helper Methods (delegate to CGame) ==============
    // These provide convenient access to common CGame functionality

    // Drawing helpers
    void DrawNewDialogBox(char cType, int sX, int sY, int iFrame,
                          bool bIsNoColorKey = false, bool bIsTrans = false);
    // Computes centered position for a dialog sprite frame within the logical resolution
    void GetCenteredDialogPos(char cType, int iFrame, int& outX, int& outY);
    void PutString(int iX, int iY, const char* pString, const hb::shared::render::Color& color);
    void PutAlignedString(int iX1, int iX2, int iY, const char* pString,
                          const hb::shared::render::Color& color = GameColors::UIBlack);
    void PutString_SprFont(int iX, int iY, const char* pStr, uint8_t r, uint8_t g, uint8_t b);
    void DrawVersion();

    // Audio helpers
    void PlayGameSound(char cType, int iNum, int iDist, long lPan = 0);

    // Event/message helpers
    void AddEventList(const char* pTxt, char cColor = 0, bool bDupAllow = true);

    // Input string helpers (for text entry screens)
    void StartInputString(int sX, int sY, unsigned char iLen, char* pBuffer, bool bIsHide = false);
    void EndInputString();
    void ClearInputString();
    void ShowReceivedString(bool bIsHide = false);

    // Screen transition helper - request transition to a new screen
    // This delegates to GameModeManager::set_screen<T>()
    template<typename T, typename... Args>
    void set_screen(Args&&... args);

    // Overlay helpers - show/hide overlay screens on top of base screen
    // Overlays automatically block input to the base screen
    template<typename T, typename... Args>
    void set_overlay(Args&&... args);

    void clear_overlay();

    // Timing helper - returns milliseconds since this screen/overlay was initialized
    uint32_t get_elapsed_ms() const;

    // Access to owning game instance
    CGame* m_pGame;
};

// Template implementation - defined inline since it just forwards to GameModeManager
// The actual implementation requires GameModeManager.h, so screens must include both headers

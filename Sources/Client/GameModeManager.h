// GameModeManager.h: Manages game mode transitions with fade effects and screen objects
//
// Singleton manager for screen transitions with fade-out to black and fade-in effects
// Supports both legacy mode-based screens and new IGameScreen objects
//
// Usage:
//   GameModeManager::Initialize(pGame);  // Call once at startup
//   GameModeManager::set_screen<Screen_Login>();  // Transition to new screen
//   GameModeManager::ChangeMode(GameMode::MainMenu);  // Legacy mode change
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include <cstdint>
#include <memory>
#include <functional>
#include <type_traits>
#include <tuple>

class CGame;
class IGameScreen;

// Game mode constants - matches DEF_GAMEMODE_* values for backwards compatibility
enum class GameMode : int8_t {
    Null = -2,
    Quit = -1,
    MainMenu = 0,
    Connecting = 1,
    Loading = 2,
    WaitingInitData = 3,
    MainGame = 4,
    ConnectionLost = 5,
    Msg = 6,
    CreateNewAccount = 7,
    Login = 8,
    QueryForceLogin = 9,
    SelectCharacter = 10,
    CreateNewCharacter = 11,
    WaitingResponse = 12,
    QueryDeleteCharacter = 13,
    LogResMsg = 14,
    ChangePassword = 15,
    // 16 is skipped in original
    VersionNotMatch = 17,
    Introduction = 18,
    Agreement = 19,
    InputKeyCode = 20
};

// Transition state for fade animations
enum class TransitionState : uint8_t {
    None,       // No transition in progress - normal rendering
    FadeOut,    // Fading current screen to black
    Switching,  // At full black - switching screens
    FadeIn      // Fading from black to new screen
};

class GameModeManager {
public:
    // Fade duration constants (in seconds)
    static constexpr float FAST_FADE_DURATION = 0.15f;
    static constexpr float DEFAULT_FADE_DURATION = 0.25f;
    static constexpr float SLOW_FADE_DURATION = 0.5f;

    // Transition configuration
    struct TransitionConfig {
        float fade_out_duration = DEFAULT_FADE_DURATION;
        float fade_in_duration = DEFAULT_FADE_DURATION;
    };

    // Screen type identifier (for type checking without RTTI)
    using ScreenTypeId = const char*;

    // ============== Singleton Access ==============

    static GameModeManager& Get();

    // ============== Initialization ==============

    static void Initialize(CGame* pGame) { Get().InitializeImpl(pGame); }
    static void Shutdown() { Get().ShutdownImpl(); }

    // ============== Template-Based Screen Creation (Static API) ==============

    template<typename T, typename... Args>
    static void set_screen(Args&&... args) {
        Get().set_screen_impl<T>(std::forward<Args>(args)...);
    }

    // ============== Overlay System (Static API) ==============

    template<typename T, typename... Args>
    static void set_overlay(Args&&... args) {
        Get().set_overlay_impl<T>(std::forward<Args>(args)...);
    }

    static void clear_overlay() { Get().clear_overlay_impl(); }
    static bool has_overlay() { return Get().m_pActiveOverlay != nullptr; }
    static IGameScreen* GetActiveOverlay() { return Get().m_pActiveOverlay.get(); }

    template<typename T>
    static T* GetActiveOverlayAs() { return dynamic_cast<T*>(Get().m_pActiveOverlay.get()); }

    template<typename T>
    static bool overlay_is() {
        static_assert(std::is_base_of_v<IGameScreen, T>, "T must derive from IGameScreen");
        auto& inst = Get();
        if (!inst.m_pActiveOverlay) return false;
        return inst.m_pActiveOverlay->get_type_id() == T::screen_type_id;
    }

    // ============== Legacy Mode Support (Static API) ==============

    static GameMode GetMode() { return Get().m_currentMode; }
    static int8_t GetModeValue() { return static_cast<int8_t>(Get().m_currentMode); }
    static void ChangeMode(GameMode newMode, bool instant = false) { Get().ChangeModeImpl(newMode, instant); }

    // Set legacy mode value without triggering a transition (for IGameScreen compatibility)
    static void SetLegacyMode(GameMode mode) { Get().m_currentMode = mode; }

    // ============== Frame Update (Static API) ==============

    static void Update() { Get().UpdateImpl(); }

    // ============== Transition State Queries (Static API) ==============

    static bool IsTransitioning() { return Get().m_transitionState != TransitionState::None; }
    static TransitionState GetTransitionState() { return Get().m_transitionState; }
    static float GetFadeAlpha() { return Get().GetFadeAlphaImpl(); }

    // ============== Active Screen Access (Static API) ==============

    static IGameScreen* GetActiveScreen() { return Get().m_pCurrentScreen.get(); }

    template<typename T>
    static T* GetActiveScreenAs() { return dynamic_cast<T*>(Get().m_pCurrentScreen.get()); }

    // ============== Type Checking (Static API) ==============

    static ScreenTypeId get_previous_screen_type() { return Get().m_previousScreenType; }
    static ScreenTypeId get_current_screen_type() { return Get().get_current_screen_type_impl(); }

    template<typename T>
    static bool previous_screen_is() {
        static_assert(std::is_base_of_v<IGameScreen, T>, "T must derive from IGameScreen");
        auto& inst = Get();
        return inst.m_previousScreenType != nullptr && inst.m_previousScreenType == T::screen_type_id;
    }

    template<typename T>
    static bool current_screen_is() {
        static_assert(std::is_base_of_v<IGameScreen, T>, "T must derive from IGameScreen");
        auto& inst = Get();
        if (!inst.m_pCurrentScreen) return false;
        return inst.m_pCurrentScreen->get_type_id() == T::screen_type_id;
    }

    // ============== Configuration (Static API) ==============

    static void set_transition_config(const TransitionConfig& config) { Get().m_config = config; }
    static const TransitionConfig& get_transition_config() { return Get().m_config; }

    // ============== Frame Counter - Legacy Compatibility (Static API) ==============

    static int GetFrameCount() { return Get().m_frameCount; }
    static void IncrementFrameCount() { auto& inst = Get(); if (inst.m_frameCount < inst.m_frameCountCap) inst.m_frameCount++; }
    static void ResetFrameCount() { Get().m_frameCount = 0; }
    static void SetFrameCountCap(int cap) { Get().m_frameCountCap = cap; }
    static int GetFrameCountCap() { return Get().m_frameCountCap; }

    // ============== Timing (Static API) ==============

    static uint32_t GetModeStartTime() { return Get().m_modeStartTime; }

    // ============== CGame Access (for screens) ==============

    static CGame* GetGame() { return Get().m_pGame; }

private:
    // Private constructor/destructor for singleton
    GameModeManager();
    ~GameModeManager();
    GameModeManager(const GameModeManager&) = delete;
    GameModeManager& operator=(const GameModeManager&) = delete;

    // ============== Implementation Methods ==============

    void InitializeImpl(CGame* pGame);
    void ShutdownImpl();
    void ChangeModeImpl(GameMode newMode, bool instant);
    void UpdateImpl();
    float GetFadeAlphaImpl() const;
    ScreenTypeId get_current_screen_type_impl() const;

    template<typename T, typename... Args>
    void set_screen_impl(Args&&... args) {
        static_assert(std::is_base_of_v<IGameScreen, T>, "T must derive from IGameScreen");

        // Clear any active overlay when transitioning to a new screen
        clear_overlay_impl();

        // Store a factory that will create the screen during the Switching phase
        // Use tuple to capture variadic args (C++17 compatible)
        auto argsTuple = std::make_tuple(std::forward<Args>(args)...);
        m_pendingScreenFactory = [this, argsTuple = std::move(argsTuple)]() mutable {
            return std::apply([this](auto&&... capturedArgs) {
                return std::make_unique<T>(m_pGame, std::forward<decltype(capturedArgs)>(capturedArgs)...);
            }, std::move(argsTuple));
        };

        // Begin fade-out transition
        m_transitionState = TransitionState::FadeOut;
        m_transitionTime = 0.0f;
    }

    template<typename T, typename... Args>
    void set_overlay_impl(Args&&... args) {
        static_assert(std::is_base_of_v<IGameScreen, T>, "T must derive from IGameScreen");

        // Clear existing overlay first
        clear_overlay_impl();

        // Create overlay immediately (no fade transition for overlays)
        m_pActiveOverlay = std::make_unique<T>(m_pGame, std::forward<Args>(args)...);
        m_pActiveOverlay->on_initialize();
    }

    void clear_overlay_impl();

    void ApplyModeChange();
    void ApplyScreenChange();

    // ============== State ==============

    CGame* m_pGame = nullptr;

    // Current screen object (owned) - null for legacy screens
    std::unique_ptr<IGameScreen> m_pCurrentScreen;

    // Overlay screen (owned) - displayed on top of current screen with shadow box
    // When active, base screen still updates but doesn't receive input
    std::unique_ptr<IGameScreen> m_pActiveOverlay;

    // Factory to create the next screen (set by set_screen<T>)
    std::function<std::unique_ptr<IGameScreen>()> m_pendingScreenFactory;

    // Screen type tracking
    ScreenTypeId m_previousScreenType = nullptr;

    // Current and pending modes (for legacy support)
    GameMode m_currentMode = GameMode::Loading;
    GameMode m_pendingMode = GameMode::Loading;

    // Transition state machine (time-based)
    TransitionState m_transitionState = TransitionState::None;
    TransitionConfig m_config;
    float m_transitionTime = 0.0f;

    // Legacy frame counter for DrawScreen_* compatibility
    int m_frameCount = 0;
    int m_frameCountCap = DEFAULT_FRAME_COUNT_CAP;

    // Timing
    uint32_t m_modeStartTime = 0;

    // Configuration
    static constexpr int DEFAULT_FRAME_COUNT_CAP = 120;
};

// ============== IGameScreen Template Implementations ==============

#include "IGameScreen.h"

template<typename T, typename... Args>
void IGameScreen::set_screen(Args&&... args)
{
    GameModeManager::set_screen<T>(std::forward<Args>(args)...);
}

template<typename T, typename... Args>
void IGameScreen::set_overlay(Args&&... args)
{
    GameModeManager::set_overlay<T>(std::forward<Args>(args)...);
}

inline void IGameScreen::clear_overlay()
{
    GameModeManager::clear_overlay();
}

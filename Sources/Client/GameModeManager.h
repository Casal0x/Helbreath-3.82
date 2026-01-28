// GameModeManager.h: Manages game mode transitions with fade effects and screen objects
//
// Singleton manager for screen transitions with fade-out to black and fade-in effects
//
// Usage:
//   GameModeManager::Initialize(pGame);  // Call once at startup
//   GameModeManager::set_screen<Screen_Login>();  // Transition to new screen
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
    Splash = -3,    // Splash screen before loading
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

    // ============== Mode Tracking (Static API) ==============
    // Mode tracking is still needed for code that checks what screen is active
    // Screens call SetCurrentMode() in on_initialize() to set their mode value

    static GameMode GetMode() { return Get().m_currentMode; }
    static int8_t GetModeValue() { return static_cast<int8_t>(Get().m_currentMode); }
    static void SetCurrentMode(GameMode mode) { Get().m_currentMode = mode; }

    // ============== Frame Update (Static API) ==============

    static void Update() { Get().UpdateImpl(); }
    static void UpdateScreens() { Get().UpdateScreensImpl(); }
    static void Render() { Get().RenderImpl(); }

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
    void UpdateImpl();
    void UpdateScreensImpl();
    void RenderImpl();
    float GetFadeAlphaImpl() const;
    ScreenTypeId get_current_screen_type_impl() const;

    template<typename T, typename... Args>
    void set_screen_impl(Args&&... args) {
        static_assert(std::is_base_of_v<IGameScreen, T>, "T must derive from IGameScreen");

        // Ignore if already transitioning to the same screen type
        if (m_pendingScreenType == T::screen_type_id)
            return;

        // Clear any active overlay when transitioning to a new screen
        clear_overlay_impl();

        // Track the pending screen type
        m_pendingScreenType = T::screen_type_id;

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
    ScreenTypeId m_pendingScreenType = nullptr;  // Type of screen being transitioned to

    // Current mode value (for code that checks what screen is active)
    GameMode m_currentMode = GameMode::Loading;

    // Transition state machine (time-based)
    TransitionState m_transitionState = TransitionState::None;
    TransitionConfig m_config;
    float m_transitionTime = 0.0f;

    // Timing
    uint32_t m_modeStartTime = 0;
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

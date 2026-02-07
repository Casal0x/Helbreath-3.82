// GameModeManager.cpp: Implementation of game mode transitions with fade effects
//
//////////////////////////////////////////////////////////////////////

// Game.h must come first to ensure winsock2 is loaded before winsock
#include "Game.h"         // For CGame, m_Renderer, LOGICAL_MAX_X()/Y
#include "GameModeManager.h"
#include "IGameScreen.h"
#include "CommonTypes.h"  // For GameClock
#include "FrameTiming.h"  // For FrameTiming::GetDeltaTime()
#include "IInput.h"       // For Input::SetSuppressed

// ============== Singleton ==============

GameModeManager& GameModeManager::Get()
{
    static GameModeManager instance;
    return instance;
}

// ============== Constructor/Destructor ==============

GameModeManager::GameModeManager()
{
    // Default initialization - actual setup happens in InitializeImpl
}

GameModeManager::~GameModeManager()
{
    ShutdownImpl();
}

// ============== Initialization ==============

void GameModeManager::InitializeImpl(CGame* pGame)
{
    m_pGame = pGame;
    m_modeStartTime = GameClock::GetTimeMS();
}

void GameModeManager::ShutdownImpl()
{
    // Clear overlay first
    clear_overlay_impl();

    // Ensure screen is properly uninitialized before destruction
    if (m_pCurrentScreen)
    {
        m_pCurrentScreen->on_uninitialize();
        m_pCurrentScreen.reset();
    }
    m_pendingScreenFactory = nullptr;
    m_pGame = nullptr;
}

// ============== Overlay Implementation ==============

void GameModeManager::clear_overlay_impl()
{
    if (m_pActiveOverlay)
    {
        m_pActiveOverlay->on_uninitialize();
        m_pActiveOverlay.reset();
    }
}

// ============== Frame Update ==============

void GameModeManager::UpdateImpl()
{
    if (m_transitionState == TransitionState::None)
        return;

    float dt = static_cast<float>(FrameTiming::GetDeltaTime());

    switch (m_transitionState)
    {
    case TransitionState::FadeOut:
        m_transitionTime += dt;
        if (m_transitionTime >= m_config.fade_out_duration)
        {
            // Fade-out complete, enter switching phase
            m_transitionState = TransitionState::Switching;
            m_transitionTime = 0.0f;
        }
        break;

    case TransitionState::Switching:
        // Perform the actual screen switch at full opacity
        ApplyScreenChange();

        // Begin fade-in
        m_transitionState = TransitionState::FadeIn;
        m_transitionTime = 0.0f;
        break;

    case TransitionState::FadeIn:
        m_transitionTime += dt;
        if (m_transitionTime >= m_config.fade_in_duration)
        {
            // Transition complete
            m_transitionState = TransitionState::None;
            m_transitionTime = 0.0f;
        }
        break;

    case TransitionState::None:
        // Should not reach here, but handle it gracefully
        break;
    }
}

// ============== Screen Update/Render ==============

void GameModeManager::UpdateScreensImpl()
{
    // Skip during switching phase (screen is being replaced)
    if (m_transitionState == TransitionState::Switching)
        return;

    bool bTransitioning = (m_transitionState != TransitionState::None);

    // Suppress input for base screen during transitions or when overlay is active
    if (bTransitioning || m_pActiveOverlay)
    {
        Input::SetSuppressed(true);
    }

    // Update base screen (input suppressed if transitioning or overlay active)
    if (m_pCurrentScreen)
    {
        m_pCurrentScreen->on_update();
    }

    // Always restore input before overlay update
    Input::SetSuppressed(false);

    // Update overlay (receives input only when not transitioning)
    if (m_pActiveOverlay && !bTransitioning)
    {
        m_pActiveOverlay->on_update();
    }
}

void GameModeManager::RenderImpl()
{
    // Render base screen
    if (m_pCurrentScreen)
    {
        m_pCurrentScreen->on_render();
    }

    // Render overlay on top with shadow box
    if (m_pActiveOverlay)
    {
        if (m_pActiveOverlay->wants_background_dim())
            m_pGame->m_Renderer->DrawRectFilled(0, 0, LOGICAL_MAX_X(), LOGICAL_MAX_Y(), Color::Black(128));
        m_pActiveOverlay->on_render();
    }
}

// ============== Fade Alpha ==============

float GameModeManager::GetFadeAlphaImpl() const
{
    switch (m_transitionState)
    {
    case TransitionState::FadeOut:
    {
        // Fading out: 0.0 -> 1.0 (transparent to black)
        if (m_config.fade_out_duration <= 0.0f) return 1.0f;
        float progress = m_transitionTime / m_config.fade_out_duration;
        return progress > 1.0f ? 1.0f : progress;
    }

    case TransitionState::Switching:
        // At full black during switch
        return 1.0f;

    case TransitionState::FadeIn:
    {
        // Fading in: 1.0 -> 0.0 (black to transparent)
        if (m_config.fade_in_duration <= 0.0f) return 0.0f;
        float alpha = 1.0f - (m_transitionTime / m_config.fade_in_duration);
        return alpha < 0.0f ? 0.0f : alpha;
    }

    case TransitionState::None:
    default:
        return 0.0f;
    }
}

// ============== Type Checking ==============

GameModeManager::ScreenTypeId GameModeManager::get_current_screen_type_impl() const
{
    if (!m_pCurrentScreen) return nullptr;
    return m_pCurrentScreen->get_type_id();
}

// ============== Screen Application ==============

void GameModeManager::ApplyScreenChange()
{
    // Uninitialize current screen
    if (m_pCurrentScreen)
    {
        m_previousScreenType = m_pCurrentScreen->get_type_id();
        m_pCurrentScreen->on_uninitialize();
        m_pCurrentScreen.reset();
    }

    // Create and initialize new screen from factory
    if (m_pendingScreenFactory)
    {
        m_pCurrentScreen = m_pendingScreenFactory();
        m_pendingScreenFactory = nullptr;

        if (m_pCurrentScreen)
        {
            m_pCurrentScreen->on_initialize();
        }
    }

    // Clear pending screen type now that transition is complete
    m_pendingScreenType = nullptr;

    // Reset timing for new screen
    m_modeStartTime = GameClock::GetTimeMS();
}

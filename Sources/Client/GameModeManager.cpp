// GameModeManager.cpp: Implementation of game mode transitions with fade effects
//
//////////////////////////////////////////////////////////////////////

#include "GameModeManager.h"
#include "IGameScreen.h"
#include "CommonTypes.h"  // For GameClock
#include "FrameTiming.h"  // For FrameTiming::GetDeltaTime()

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

// ============== Legacy Mode Change ==============

void GameModeManager::ChangeModeImpl(GameMode newMode, bool instant)
{
    // If we have an active screen object, clear it when switching to legacy mode
    if (m_pCurrentScreen)
    {
        // Clear the pending screen factory since we're going to legacy mode
        m_pendingScreenFactory = nullptr;
    }

    // Already in requested mode with no transition pending - ignore
    if (m_currentMode == newMode && m_transitionState == TransitionState::None)
        return;

    // If already transitioning, update the pending mode
    if (m_transitionState != TransitionState::None)
    {
        m_pendingMode = newMode;
        return;
    }

    m_pendingMode = newMode;

    if (instant)
    {
        // Instant transition: Apply mode change immediately
        // For screen objects, uninitialize current screen
        if (m_pCurrentScreen)
        {
            m_previousScreenType = m_pCurrentScreen->get_type_id();
            m_pCurrentScreen->on_uninitialize();
            m_pCurrentScreen.reset();
        }

        ApplyModeChange();

        // Start fade-in from black
        m_transitionState = TransitionState::FadeIn;
        m_transitionTime = 0.0f;
    }
    else
    {
        // Normal transition: Start fade-out first
        m_transitionState = TransitionState::FadeOut;
        m_transitionTime = 0.0f;
    }
}

// ============== Frame Update ==============

void GameModeManager::UpdateImpl()
{
    if (m_transitionState == TransitionState::None)
        return;

    float dt = FrameTiming::GetDeltaTime();

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
        // Perform the actual screen/mode switch at full opacity

        // If we have a pending screen factory, use screen-based transition
        if (m_pendingScreenFactory)
        {
            ApplyScreenChange();
        }
        else
        {
            // Legacy mode-based transition
            // Uninitialize current screen object if present
            if (m_pCurrentScreen)
            {
                m_previousScreenType = m_pCurrentScreen->get_type_id();
                m_pCurrentScreen->on_uninitialize();
                m_pCurrentScreen.reset();
            }

            ApplyModeChange();
        }

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

// ============== Mode/Screen Application ==============

void GameModeManager::ApplyModeChange()
{
    m_currentMode = m_pendingMode;
    m_frameCount = 0;
    m_bModeChangedThisFrame = true;  // Skip IncrementFrameCount this frame
    m_modeStartTime = GameClock::GetTimeMS();
}

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

    // Reset frame counter and timing
    m_frameCount = 0;
    m_bModeChangedThisFrame = true;  // Skip IncrementFrameCount this frame
    m_modeStartTime = GameClock::GetTimeMS();
}

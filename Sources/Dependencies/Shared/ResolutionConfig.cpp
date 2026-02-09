#include "ResolutionConfig.h"

bool ResolutionConfig::s_bInitialized = false;

void ResolutionConfig::Initialize(int windowWidth, int windowHeight)
{
	Get().m_width = BASE_RESOLUTION_WIDTH;
	Get().m_height = BASE_RESOLUTION_HEIGHT;

	// Store window size and calculate screen offset
	Get().m_windowWidth = windowWidth;
	Get().m_windowHeight = windowHeight;
	Get().RecalculateScreenOffset();

	s_bInitialized = true;
}

ResolutionConfig& ResolutionConfig::Get()
{
	static ResolutionConfig instance;
	return instance;
}

void ResolutionConfig::SetWindowSize(int windowWidth, int windowHeight)
{
	m_windowWidth = windowWidth;
	m_windowHeight = windowHeight;
	RecalculateScreenOffset();
}


void ResolutionConfig::RecalculateScreenOffset()
{
	// Center the logical resolution within the window
	// This is used when window size differs from logical size
	m_screenX = (m_windowWidth - m_width) / 2;
	m_screenY = (m_windowHeight - m_height) / 2;

	// Ensure non-negative offsets
	if (m_screenX < 0) m_screenX = 0;
	if (m_screenY < 0) m_screenY = 0;
}

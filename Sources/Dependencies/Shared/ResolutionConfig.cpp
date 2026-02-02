#include "ResolutionConfig.h"

bool ResolutionConfig::s_bInitialized = false;

void ResolutionConfig::Initialize(int baseWidth, int baseHeight, int windowWidth, int windowHeight)
{
	// Validate base resolution - only 640x480 or 800x600 are valid
	if (baseWidth == 800 && baseHeight == 600)
	{
		Get().m_width = 800;
		Get().m_height = 600;
	}
	else
	{
		// Default to 640x480 for any other value
		Get().m_width = 640;
		Get().m_height = 480;
	}

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

void ResolutionConfig::SetBaseResolution(int baseWidth, int baseHeight)
{
	// Validate base resolution - only 640x480 or 800x600 are valid
	if (baseWidth == 800 && baseHeight == 600)
	{
		m_width = 800;
		m_height = 600;
	}
	else
	{
		// Default to 640x480 for any other value
		m_width = 640;
		m_height = 480;
	}
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

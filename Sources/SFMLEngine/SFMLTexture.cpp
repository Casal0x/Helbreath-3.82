// SFMLTexture.cpp: SFML implementation of hb::shared::render::ITexture interface
//
// Part of SFMLEngine static library
//////////////////////////////////////////////////////////////////////

#include "SFMLTexture.h"

SFMLTexture::SFMLTexture(uint16_t width, uint16_t height)
    : m_width(width)
    , m_height(height)
{
    // Create the texture with the specified size
    if (!m_texture.resize({width, height}))
    {
        // Texture creation failed - this shouldn't happen with a valid size
    }

    // Create render texture for blit operations
    if (!m_renderTexture.resize({width, height}))
    {
        // RenderTexture creation failed
    }

    // Initialize render texture to black
    m_renderTexture.clear(sf::Color::Black);
    m_renderTexture.display();
}

SFMLTexture::~SFMLTexture()
{
    // SFML handles cleanup automatically
}

uint16_t SFMLTexture::get_width() const
{
    return m_width;
}

uint16_t SFMLTexture::get_height() const
{
    return m_height;
}



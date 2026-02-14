// SFMLTexture.cpp: SFML implementation of hb::shared::render::ITexture interface
//
// Part of SFMLEngine static library
//////////////////////////////////////////////////////////////////////

#include "SFMLTexture.h"
#include <SFML/Graphics/Sprite.hpp>

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

bool SFMLTexture::Blt(RECT* destRect, hb::shared::render::ITexture* src, RECT* srcRect, uint32_t flags)
{
    if (!src)
        return false;

    SFMLTexture* sfmlSrc = static_cast<SFMLTexture*>(src);

    // Determine source rectangle
    sf::IntRect srcIntRect;
    if (srcRect)
    {
        srcIntRect = sf::IntRect(
            {static_cast<int>(srcRect->left), static_cast<int>(srcRect->top)},
            {static_cast<int>(srcRect->right - srcRect->left), static_cast<int>(srcRect->bottom - srcRect->top)}
        );
    }
    else
    {
        srcIntRect = sf::IntRect({0, 0}, {static_cast<int>(sfmlSrc->get_width()), static_cast<int>(sfmlSrc->get_height())});
    }

    // Determine destination position
    float destX = 0, destY = 0;
    float scaleX = 1.0f, scaleY = 1.0f;

    if (destRect)
    {
        destX = static_cast<float>(destRect->left);
        destY = static_cast<float>(destRect->top);

        int destWidth = destRect->right - destRect->left;
        int destHeight = destRect->bottom - destRect->top;

        if (destWidth != srcIntRect.size.x || destHeight != srcIntRect.size.y)
        {
            scaleX = static_cast<float>(destWidth) / srcIntRect.size.x;
            scaleY = static_cast<float>(destHeight) / srcIntRect.size.y;
        }
    }

    // Create sprite for drawing
    sf::Sprite sprite(sfmlSrc->GetRenderTexture()->getTexture(), srcIntRect);
    sprite.setPosition({destX, destY});
    sprite.setScale({scaleX, scaleY});

    m_renderTexture.draw(sprite);
    m_renderTexture.display();

    return true;
}

bool SFMLTexture::BltFast(int x, int y, hb::shared::render::ITexture* src, RECT* srcRect, uint32_t flags)
{
    if (!src)
        return false;

    SFMLTexture* sfmlSrc = static_cast<SFMLTexture*>(src);

    // Determine source rectangle
    sf::IntRect srcIntRect;
    if (srcRect)
    {
        srcIntRect = sf::IntRect(
            {static_cast<int>(srcRect->left), static_cast<int>(srcRect->top)},
            {static_cast<int>(srcRect->right - srcRect->left), static_cast<int>(srcRect->bottom - srcRect->top)}
        );
    }
    else
    {
        srcIntRect = sf::IntRect({0, 0}, {static_cast<int>(sfmlSrc->get_width()), static_cast<int>(sfmlSrc->get_height())});
    }

    // Create sprite for drawing
    sf::Sprite sprite(sfmlSrc->GetRenderTexture()->getTexture(), srcIntRect);
    sprite.setPosition({static_cast<float>(x), static_cast<float>(y)});

    m_renderTexture.draw(sprite);
    m_renderTexture.display();

    return true;
}

void* SFMLTexture::GetNativeHandle()
{
    return &m_texture;
}

bool SFMLTexture::IsLost() const
{
    // SFML textures don't get "lost" like DirectDraw surfaces
    return false;
}

bool SFMLTexture::Restore()
{
    // No restoration needed for SFML
    return true;
}


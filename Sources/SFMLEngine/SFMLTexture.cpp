// SFMLTexture.cpp: SFML implementation of ITexture interface
//
// Part of SFMLEngine static library
//////////////////////////////////////////////////////////////////////

#include "SFMLTexture.h"
#include "PixelConversion.h"
#include <SFML/Graphics/Sprite.hpp>

SFMLTexture::SFMLTexture(uint16_t width, uint16_t height)
    : m_width(width)
    , m_height(height)
    , m_colorKey(0)
    , m_locked(false)
{
    // Create the image with the specified size
    m_image.resize({width, height}, sf::Color::Black);

    // Create the texture from the image
    if (!m_texture.loadFromImage(m_image))
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

uint16_t SFMLTexture::GetWidth() const
{
    return m_width;
}

uint16_t SFMLTexture::GetHeight() const
{
    return m_height;
}

void SFMLTexture::SetColorKey(uint16_t colorKey)
{
    m_colorKey = colorKey;
}

void SFMLTexture::SetColorKeyRGB(uint8_t r, uint8_t g, uint8_t b)
{
    m_colorKey = PixelConversion::MakeRGB565(r, g, b);
}

uint16_t* SFMLTexture::Lock(int* pitch)
{
    if (m_locked)
        return nullptr;

    m_locked = true;

    // Resize locked buffer if needed
    size_t bufferSize = static_cast<size_t>(m_width) * m_height;
    if (m_lockedBuffer.size() != bufferSize)
    {
        m_lockedBuffer.resize(bufferSize);
    }

    // Copy current texture data to locked buffer (convert RGBA to RGB565)
    // Get the render texture's content
    sf::Image currentImage = m_renderTexture.getTexture().copyToImage();

    for (int y = 0; y < m_height; y++)
    {
        for (int x = 0; x < m_width; x++)
        {
            sf::Color pixel = currentImage.getPixel({static_cast<unsigned int>(x), static_cast<unsigned int>(y)});

            // Convert RGBA to RGB565
            if (pixel.a == 0)
            {
                m_lockedBuffer[y * m_width + x] = m_colorKey;
            }
            else
            {
                m_lockedBuffer[y * m_width + x] = PixelConversion::MakeRGB565(pixel.r, pixel.g, pixel.b);
            }
        }
    }

    // Return pitch in pixels (not bytes)
    if (pitch)
        *pitch = m_width;

    return m_lockedBuffer.data();
}

void SFMLTexture::Unlock()
{
    if (!m_locked)
        return;

    // Convert RGB565 buffer back to RGBA and update the texture
    for (int y = 0; y < m_height; y++)
    {
        for (int x = 0; x < m_width; x++)
        {
            uint16_t pixel = m_lockedBuffer[y * m_width + x];

            if (pixel == m_colorKey)
            {
                m_image.setPixel({static_cast<unsigned int>(x), static_cast<unsigned int>(y)}, sf::Color::Transparent);
            }
            else
            {
                uint8_t r, g, b;
                PixelConversion::ExtractRGB565(pixel, r, g, b);
                m_image.setPixel({static_cast<unsigned int>(x), static_cast<unsigned int>(y)}, sf::Color(r, g, b, 255));
            }
        }
    }

    // Update the texture from the image
    m_texture.loadFromImage(m_image);

    // Update the render texture
    m_renderTexture.clear(sf::Color::Transparent);
    sf::Sprite sprite(m_texture);
    m_renderTexture.draw(sprite);
    m_renderTexture.display();

    m_locked = false;
}

bool SFMLTexture::Blt(RECT* destRect, ITexture* src, RECT* srcRect, uint32_t flags)
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
        srcIntRect = sf::IntRect({0, 0}, {static_cast<int>(sfmlSrc->GetWidth()), static_cast<int>(sfmlSrc->GetHeight())});
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

bool SFMLTexture::BltFast(int x, int y, ITexture* src, RECT* srcRect, uint32_t flags)
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
        srcIntRect = sf::IntRect({0, 0}, {static_cast<int>(sfmlSrc->GetWidth()), static_cast<int>(sfmlSrc->GetHeight())});
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

void SFMLTexture::UpdateFromImage()
{
    m_texture.loadFromImage(m_image);

    // Update the render texture
    m_renderTexture.clear(sf::Color::Transparent);
    sf::Sprite sprite(m_texture);
    m_renderTexture.draw(sprite);
    m_renderTexture.display();
}

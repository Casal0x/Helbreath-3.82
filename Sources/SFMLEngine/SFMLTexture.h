// SFMLTexture.h: SFML implementation of ITexture interface
//
// Part of SFMLEngine static library
// Wraps sf::Texture with sf::Image for Lock/Unlock support
//////////////////////////////////////////////////////////////////////

#pragma once

#include "ITexture.h"
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <vector>

class SFMLTexture : public ITexture
{
public:
    SFMLTexture(uint16_t width, uint16_t height);
    virtual ~SFMLTexture();

    // ============== ITexture Implementation ==============

    // Dimensions
    uint16_t GetWidth() const override;
    uint16_t GetHeight() const override;

    // Color Key
    void SetColorKey(uint16_t colorKey) override;
    void SetColorKeyRGB(uint8_t r, uint8_t g, uint8_t b) override;

    // Direct Pixel Access
    uint16_t* Lock(int* pitch) override;
    void Unlock() override;

    // Blitting
    bool Blt(RECT* destRect, ITexture* src, RECT* srcRect, uint32_t flags) override;
    bool BltFast(int x, int y, ITexture* src, RECT* srcRect, uint32_t flags) override;

    // Native Handle (returns sf::Texture*)
    void* GetNativeHandle() override;

    // Device loss (no-op for SFML)
    bool IsLost() const override;
    bool Restore() override;

    // ============== SFML-Specific Access ==============

    // Get the underlying SFML texture
    sf::Texture& GetTexture() { return m_texture; }
    const sf::Texture& GetTexture() const { return m_texture; }

    // Get the SFML render texture (for rendering operations)
    sf::RenderTexture* GetRenderTexture() { return &m_renderTexture; }

    // Update texture from internal image data
    void UpdateFromImage();

    // Get color key value (RGB565)
    uint16_t GetColorKey() const { return m_colorKey; }

private:
    sf::Texture m_texture;
    sf::RenderTexture m_renderTexture;
    sf::Image m_image;  // For Lock/Unlock operations

    uint16_t m_width;
    uint16_t m_height;
    uint16_t m_colorKey;

    // Lock state
    bool m_locked;
    std::vector<uint16_t> m_lockedBuffer;  // RGB565 format for compatibility
};

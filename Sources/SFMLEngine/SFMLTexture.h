// SFMLTexture.h: SFML implementation of hb::shared::render::ITexture interface
//
// Part of SFMLEngine static library
// Wraps sf::Texture and sf::RenderTexture for rendering operations
//////////////////////////////////////////////////////////////////////

#pragma once

#include "ITexture.h"
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/RenderTexture.hpp>

class SFMLTexture : public hb::shared::render::ITexture
{
public:
    SFMLTexture(uint16_t width, uint16_t height);
    virtual ~SFMLTexture();

    // ============== hb::shared::render::ITexture Implementation ==============
    uint16_t get_width() const override;
    uint16_t get_height() const override;

    // ============== SFML-Specific Access ==============

    // get the underlying SFML texture
    sf::Texture& GetTexture() { return m_texture; }
    const sf::Texture& GetTexture() const { return m_texture; }

    // get the SFML render texture (for rendering operations)
    sf::RenderTexture* GetRenderTexture() { return &m_renderTexture; }

private:
    sf::Texture m_texture;
    sf::RenderTexture m_renderTexture;

    uint16_t m_width;
    uint16_t m_height;

};

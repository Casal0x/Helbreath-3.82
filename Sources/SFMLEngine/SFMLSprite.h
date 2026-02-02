// SFMLSprite.h: SFML implementation of ISprite interface
//
// Part of SFMLEngine static library
// Handles sprite rendering using SFML textures with support for
// alpha blending, tinting, shadows, and other effects
//////////////////////////////////////////////////////////////////////

#pragma once

#include "ISprite.h"
#include "PAK.h"
#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <string>
#include <vector>
#include <cstdint>

// Forward declaration
class SFMLRenderer;

class SFMLSprite : public SpriteLib::ISprite
{
public:
    // Construction from file path (opens file, reads sprite data)
    SFMLSprite(SFMLRenderer* pRenderer, const std::string& pakFilePath, int spriteIndex, bool alphaEffect = true);

    // Construction from pre-loaded PAK data (no file I/O - used by SpriteLoader)
    SFMLSprite(SFMLRenderer* pRenderer, const PAKLib::sprite& spriteData, bool alphaEffect = true);

    virtual ~SFMLSprite();

    //------------------------------------------------------------------
    // ISprite Implementation
    //------------------------------------------------------------------

    // Core drawing
    void Draw(int x, int y, int frame, const SpriteLib::DrawParams& params = SpriteLib::DrawParams{}) override;
    void DrawToSurface(void* destSurface, int x, int y, int frame, const SpriteLib::DrawParams& params = SpriteLib::DrawParams{}) override;
    void DrawWidth(int x, int y, int frame, int width, bool vertical = false) override;
    void DrawShifted(int x, int y, int shiftX, int shiftY, int frame, const SpriteLib::DrawParams& params = SpriteLib::DrawParams{}) override;

    // Frame information
    int GetFrameCount() const override;
    SpriteLib::SpriteRect GetFrameRect(int frame) const override;
    void GetBoundingRect(int x, int y, int frame, int& left, int& top, int& right, int& bottom) override;
    void CalculateBounds(int x, int y, int frame) override;
    bool GetLastDrawBounds(int& left, int& top, int& right, int& bottom) const override;
    SpriteLib::BoundRect GetBoundRect() const override;

    // Collision detection
    bool CheckCollision(int spriteX, int spriteY, int frame, int pointX, int pointY) override;

    // Resource management
    void Preload() override;
    void Unload() override;
    bool IsLoaded() const override;
    void Restore() override;
    bool IsInUse() const override;
    uint32_t GetLastAccessTime() const override;

    //------------------------------------------------------------------
    // SFML-Specific Methods
    //------------------------------------------------------------------

    // Get the SFML texture
    const sf::Texture& GetTexture() const { return m_texture; }

    // Alpha degree management
    void SetAlphaDegree(char degree);
    char GetAlphaDegree() const { return m_alphaDegree; }
    bool HasAlphaEffect() const { return m_alphaEffect; }

private:
    //------------------------------------------------------------------
    // Internal Methods
    //------------------------------------------------------------------

    // Initialize from PAK sprite data
    void InitFromSpriteData(const PAKLib::sprite& spriteData);

    // Create SFML texture from 16-bit image data
    bool CreateTexture();

    // Draw implementation
    void DrawInternal(sf::RenderTexture* target, int x, int y, int frame, const SpriteLib::DrawParams& params);

    // Apply alpha degree effect
    void ApplyAlphaDegree();

    //------------------------------------------------------------------
    // Member Variables
    //------------------------------------------------------------------

    // Renderer reference
    SFMLRenderer* m_pRenderer;

    // PAK file info (for lazy loading)
    std::string m_pakFilePath;
    int m_spriteIndex;

    // Frame data from PAK
    std::vector<PAKLib::sprite_rect> m_frames;

    // PNG image data (cleared after texture creation to save memory)
    std::vector<uint8_t> m_imageData;
    uint16_t m_bitmapWidth;
    uint16_t m_bitmapHeight;

    // SFML texture (32-bit RGBA, lazy loaded)
    sf::Texture m_texture;
    sf::Image m_collisionImage;  // Retained for pixel-perfect collision detection
    bool m_textureLoaded;

    // Alpha effect
    bool m_alphaEffect;
    char m_alphaDegree;

    // State
    bool m_inUse;
    SpriteLib::BoundRect m_boundRect;
    uint32_t m_lastAccessTime;
};

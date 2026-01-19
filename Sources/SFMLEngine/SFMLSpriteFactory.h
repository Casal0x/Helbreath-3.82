// SFMLSpriteFactory.h: SFML implementation of ISpriteFactory interface
//
// Part of SFMLEngine static library
// Creates SFMLSprite instances from PAK files
//////////////////////////////////////////////////////////////////////

#pragma once

#include "ISpriteFactory.h"
#include <string>

// Forward declaration
class SFMLRenderer;

class SFMLSpriteFactory : public SpriteLib::ISpriteFactory
{
public:
    SFMLSpriteFactory(SFMLRenderer* pRenderer);
    virtual ~SFMLSpriteFactory();

    //------------------------------------------------------------------
    // ISpriteFactory Implementation
    //------------------------------------------------------------------

    // Sprite creation
    SpriteLib::ISprite* CreateSprite(const std::string& pakName, int spriteIndex, bool alphaEffect = true) override;
    SpriteLib::ISprite* CreateSpriteFromData(const PAKLib::sprite& spriteData, bool alphaEffect = true) override;

    // Sprite destruction
    void DestroySprite(SpriteLib::ISprite* sprite) override;

    // Global alpha degree
    void SetGlobalAlphaDegree(int degree) override;
    int GetGlobalAlphaDegree() const override;

    // PAK file information
    int GetSpriteCount(const std::string& pakName) const override;

    //------------------------------------------------------------------
    // Configuration
    //------------------------------------------------------------------

    // Set the sprite path prefix (default: "sprites")
    void SetSpritePath(const std::string& path) { m_spritePath = path; }
    std::string GetSpritePath() const override { return m_spritePath; }

private:
    // Build full PAK file path from pak name
    std::string BuildPakPath(const std::string& pakName) const;

    SFMLRenderer* m_pRenderer;
    std::string m_spritePath;
    int m_globalAlphaDegree;
};

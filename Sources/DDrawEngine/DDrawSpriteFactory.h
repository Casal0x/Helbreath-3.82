// DDrawSpriteFactory.h: DirectDraw implementation of ISpriteFactory
//
// Creates DDrawSprite instances and manages global sprite settings.
//////////////////////////////////////////////////////////////////////

#pragma once

#include "ISpriteFactory.h"
#include "DDrawSprite.h"
#include <string>
#include <unordered_map>

class DDrawSpriteFactory : public SpriteLib::ISpriteFactory {
public:
    DDrawSpriteFactory(DXC_ddraw* pDDraw, const std::string& spritePath = "sprites");
    virtual ~DDrawSpriteFactory();

    //------------------------------------------------------------------
    // ISpriteFactory Implementation
    //------------------------------------------------------------------

    SpriteLib::ISprite* CreateSprite(const std::string& pakName, int spriteIndex, bool alphaEffect = true) override;
    void DestroySprite(SpriteLib::ISprite* sprite) override;

    void SetGlobalAlphaDegree(int degree) override;
    int GetGlobalAlphaDegree() const override;

    //------------------------------------------------------------------
    // Additional Methods
    //------------------------------------------------------------------

    // Set the base path for sprite PAK files
    void SetSpritePath(const std::string& path) { m_spritePath = path; }
    std::string GetSpritePath() const { return m_spritePath; }

    // Get the DDraw renderer
    DXC_ddraw* GetDDraw() const { return m_pDDraw; }

private:
    // Build full PAK file path from pak name
    std::string BuildPakPath(const std::string& pakName) const;

    DXC_ddraw* m_pDDraw;
    std::string m_spritePath;
    int m_globalAlphaDegree;
};

// DDrawSpriteFactory.cpp: DirectDraw implementation of ISpriteFactory
//
//////////////////////////////////////////////////////////////////////

#include "DDrawSpriteFactory.h"

// External global alpha degree (legacy)
extern char G_cSpriteAlphaDegree;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

DDrawSpriteFactory::DDrawSpriteFactory(DXC_ddraw* pDDraw, const std::string& spritePath)
    : m_pDDraw(pDDraw)
    , m_spritePath(spritePath)
    , m_globalAlphaDegree(1)
{
    G_cSpriteAlphaDegree = 1;
}

DDrawSpriteFactory::~DDrawSpriteFactory()
{
}

//////////////////////////////////////////////////////////////////////
// ISpriteFactory Implementation
//////////////////////////////////////////////////////////////////////

SpriteLib::ISprite* DDrawSpriteFactory::CreateSprite(const std::string& pakName, int spriteIndex, bool alphaEffect)
{
    std::string pakPath = BuildPakPath(pakName);
    return new DDrawSprite(m_pDDraw, pakPath, spriteIndex, alphaEffect);
}

void DDrawSpriteFactory::DestroySprite(SpriteLib::ISprite* sprite)
{
    if (sprite != nullptr) {
        delete sprite;
    }
}

void DDrawSpriteFactory::SetGlobalAlphaDegree(int degree)
{
    m_globalAlphaDegree = degree;
    G_cSpriteAlphaDegree = static_cast<char>(degree);
}

int DDrawSpriteFactory::GetGlobalAlphaDegree() const
{
    return m_globalAlphaDegree;
}

//////////////////////////////////////////////////////////////////////
// Helper Methods
//////////////////////////////////////////////////////////////////////

std::string DDrawSpriteFactory::BuildPakPath(const std::string& pakName) const
{
    // Build path: sprites/pakName.pak
    std::string path = m_spritePath;
    if (!path.empty() && path.back() != '/' && path.back() != '\\') {
        path += "\\";
    }
    path += pakName;
    path += ".pak";
    return path;
}

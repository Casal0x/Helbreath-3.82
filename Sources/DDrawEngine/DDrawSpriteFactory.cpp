// DDrawSpriteFactory.cpp: DirectDraw implementation of ISpriteFactory
//
//////////////////////////////////////////////////////////////////////

#include "DDrawSpriteFactory.h"
#include "PAK.h"
#include <fstream>

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

SpriteLib::ISprite* DDrawSpriteFactory::CreateSpriteFromData(const PAKLib::sprite& spriteData, bool alphaEffect)
{
    return new DDrawSprite(m_pDDraw, spriteData, alphaEffect);
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

int DDrawSpriteFactory::GetSpriteCount(const std::string& pakName) const
{
    std::string pakPath = BuildPakPath(pakName);

    std::ifstream file(pakPath, std::ios::binary);
    if (!file.is_open()) {
        return 0;
    }

    // Skip file header
    file.seekg(sizeof(PAKLib::file_header), std::ios::beg);

    // Read sprite count
    uint32_t spriteCount = 0;
    file.read(reinterpret_cast<char*>(&spriteCount), sizeof(uint32_t));
    if (file.fail() || spriteCount > 1500) {
        return 0;
    }

    return static_cast<int>(spriteCount);
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

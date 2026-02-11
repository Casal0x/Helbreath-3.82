// SFMLSpriteFactory.cpp: SFML implementation of ISpriteFactory interface
//
// Part of SFMLEngine static library
//////////////////////////////////////////////////////////////////////

#include "SFMLSpriteFactory.h"
#include "SFMLSprite.h"
#include "SFMLRenderer.h"

SFMLSpriteFactory::SFMLSpriteFactory(SFMLRenderer* pRenderer)
    : m_pRenderer(pRenderer)
    , m_spritePath("sprites")
    , m_globalAlphaDegree(1)
{
}

SFMLSpriteFactory::~SFMLSpriteFactory()
{
    // Note: Factory does not own created sprites
    // Caller is responsible for destroying sprites via DestroySprite
}

hb::shared::sprite::ISprite* SFMLSpriteFactory::CreateSprite(const std::string& pakName, int spriteIndex, bool alphaEffect)
{
    std::string fullPath = BuildPakPath(pakName);

    SFMLSprite* sprite = new SFMLSprite(m_pRenderer, fullPath, spriteIndex, alphaEffect);

    // Apply global alpha degree
    if (alphaEffect && m_globalAlphaDegree != 1)
    {
        sprite->SetAlphaDegree(static_cast<char>(m_globalAlphaDegree));
    }

    return sprite;
}

hb::shared::sprite::ISprite* SFMLSpriteFactory::CreateSpriteFromData(const PAKLib::sprite& spriteData, bool alphaEffect)
{
    SFMLSprite* sprite = new SFMLSprite(m_pRenderer, spriteData, alphaEffect);

    // Apply global alpha degree
    if (alphaEffect && m_globalAlphaDegree != 1)
    {
        sprite->SetAlphaDegree(static_cast<char>(m_globalAlphaDegree));
    }

    return sprite;
}

void SFMLSpriteFactory::DestroySprite(hb::shared::sprite::ISprite* sprite)
{
    delete sprite;
}

void SFMLSpriteFactory::SetGlobalAlphaDegree(int degree)
{
    m_globalAlphaDegree = degree;

    // Update renderer's sprite alpha degree
    if (m_pRenderer)
    {
        m_pRenderer->SetSpriteAlphaDegree(static_cast<char>(degree));
    }
}

int SFMLSpriteFactory::GetGlobalAlphaDegree() const
{
    return m_globalAlphaDegree;
}

int SFMLSpriteFactory::GetSpriteCount(const std::string& pakName) const
{
    std::string fullPath = BuildPakPath(pakName);

    try
    {
        PAKLib::pak pakFile = PAKLib::loadpak_fast(fullPath);
        return static_cast<int>(pakFile.sprite_count);
    }
    catch (...)
    {
        return 0;
    }
}

std::string SFMLSpriteFactory::BuildPakPath(const std::string& pakName) const
{
    // If pakName already has a path or extension, use as-is
    if (pakName.find('/') != std::string::npos ||
        pakName.find('\\') != std::string::npos ||
        pakName.find('.') != std::string::npos)
    {
        return pakName;
    }

    // Build path: spritePath/pakName.pak
    std::string path = m_spritePath;
    if (!path.empty() && path.back() != '/' && path.back() != '\\')
    {
        path += '/';
    }
    path += pakName;
    path += ".pak";

    return path;
}

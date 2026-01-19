// ISpriteFactory.cpp: Static member implementations for Sprites class
//
//////////////////////////////////////////////////////////////////////

#include "ISpriteFactory.h"
#include <cstdio>

namespace SpriteLib {

// Static member initialization
ISpriteFactory* Sprites::s_pFactory = nullptr;

void Sprites::SetFactory(ISpriteFactory* factory) {
    s_pFactory = factory;
}

ISpriteFactory* Sprites::GetFactory() {
    return s_pFactory;
}

ISprite* Sprites::Create(const std::string& pakName, int spriteIndex, bool alphaEffect) {
    if (s_pFactory) {
        return s_pFactory->CreateSprite(pakName, spriteIndex, alphaEffect);
    }
    printf("[Sprites::Create] ERROR: No factory set! Cannot create sprite %s[%d]\n", pakName.c_str(), spriteIndex);
    return nullptr;
}

void Sprites::Destroy(ISprite* sprite) {
    if (s_pFactory && sprite) {
        s_pFactory->DestroySprite(sprite);
    }
}

void Sprites::SetAlphaDegree(int degree) {
    if (s_pFactory) {
        s_pFactory->SetGlobalAlphaDegree(degree);
    }
}

int Sprites::GetAlphaDegree() {
    if (s_pFactory) {
        return s_pFactory->GetGlobalAlphaDegree();
    }
    return 1;
}

int Sprites::GetSpriteCount(const std::string& pakName) {
    if (s_pFactory) {
        return s_pFactory->GetSpriteCount(pakName);
    }
    return 0;
}

std::string Sprites::GetSpritePath() {
    if (s_pFactory) {
        return s_pFactory->GetSpritePath();
    }
    return "sprites";  // Default fallback
}

} // namespace SpriteLib

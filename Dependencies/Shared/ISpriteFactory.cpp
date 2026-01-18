// ISpriteFactory.cpp: Static member implementations for Sprites class
//
//////////////////////////////////////////////////////////////////////

#include "ISpriteFactory.h"

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

} // namespace SpriteLib

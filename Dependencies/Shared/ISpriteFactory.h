// ISpriteFactory.h: Abstract sprite factory interface for renderer abstraction
//
// Part of the shared interface layer between client and renderers
//////////////////////////////////////////////////////////////////////

#pragma once

#include "ISprite.h"
#include <string>

namespace SpriteLib {

class ISpriteFactory {
public:
    virtual ~ISpriteFactory() = default;

    //------------------------------------------------------------------
    // Sprite Creation/Destruction
    //------------------------------------------------------------------

    // Create a sprite from a PAK file
    // pakName: Name of the PAK file (without path/extension, e.g., "Gmtile")
    // spriteIndex: Index of the sprite within the PAK
    // alphaEffect: Whether this sprite supports alpha degree changes
    virtual ISprite* CreateSprite(const std::string& pakName, int spriteIndex, bool alphaEffect = true) = 0;

    // Destroy a sprite and free its resources
    virtual void DestroySprite(ISprite* sprite) = 0;

    //------------------------------------------------------------------
    // Global Alpha Degree (legacy feature)
    //------------------------------------------------------------------

    // Set global alpha degree affecting all sprites with alphaEffect enabled
    // Degree 1 = normal, Degree 2 = night/dark mode
    virtual void SetGlobalAlphaDegree(int degree) = 0;
    virtual int GetGlobalAlphaDegree() const = 0;
};

//------------------------------------------------------------------
// Global Sprite Factory Access
//------------------------------------------------------------------

class Sprites {
public:
    // Set the active sprite factory (called by renderer during init)
    static void SetFactory(ISpriteFactory* factory);

    // Get the active sprite factory
    static ISpriteFactory* GetFactory();

    // Convenience methods that delegate to the active factory
    static ISprite* Create(const std::string& pakName, int spriteIndex, bool alphaEffect = true);
    static void Destroy(ISprite* sprite);

    // Global alpha degree
    static void SetAlphaDegree(int degree);
    static int GetAlphaDegree();

private:
    static ISpriteFactory* s_pFactory;
};

} // namespace SpriteLib

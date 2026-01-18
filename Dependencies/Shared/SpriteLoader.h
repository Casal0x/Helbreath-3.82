// SpriteLoader.h: Efficient batch sprite loading from PAK files
//
// Opens a PAK file once and allows loading multiple sprites from it
// without repeated file I/O operations.
//////////////////////////////////////////////////////////////////////

#pragma once

#include "ISprite.h"
#include "ISpriteFactory.h"
#include "PAK.h"
#include <string>
#include <functional>
#include <stdexcept>
#include <memory>

namespace SpriteLib {

class SpriteLoader {
public:
    // Open a PAK file and execute a callback with the loader
    // The PAK file is loaded once and kept in memory during the callback
    template<typename Func>
    static void open_pak(const std::string& pakName, Func&& use) {
        SpriteLoader loader;
        loader.m_pakName = pakName;

        ISpriteFactory* factory = Sprites::GetFactory();
        if (!factory) {
            throw std::runtime_error("No sprite factory set");
        }

        // Build full path using factory's path rules
        std::string pakPath = "sprites\\" + pakName + ".pak";

        try {
            loader.m_pak = PAKLib::loadpak_fast(pakPath);
            loader.m_isPakOpen = true;
            use(loader);
        }
        catch (...) {
            loader.m_isPakOpen = false;
            throw;
        }
        loader.m_isPakOpen = false;
    }

    // Open a PAK file with access to the raw PAK data
    template<typename Func>
    static void open_pak_with_data(const std::string& pakName, Func&& use) {
        SpriteLoader loader;
        loader.m_pakName = pakName;

        std::string pakPath = "sprites\\" + pakName + ".pak";

        try {
            loader.m_pak = PAKLib::loadpak_fast(pakPath);
            loader.m_isPakOpen = true;
            use(loader, loader.m_pak);
        }
        catch (...) {
            loader.m_isPakOpen = false;
            throw;
        }
        loader.m_isPakOpen = false;
    }

    // Get a sprite from the loaded PAK file
    // Returns nullptr if sprite creation fails
    ISprite* get_sprite(size_t index, bool alphaEffect = true) {
        if (!m_isPakOpen) {
            throw std::runtime_error("No PAK file is currently open");
        }
        if (index >= m_pak.sprites.size()) {
            throw std::out_of_range("Sprite index out of range: " + std::to_string(index) +
                                    " >= " + std::to_string(m_pak.sprites.size()));
        }

        ISpriteFactory* factory = Sprites::GetFactory();
        if (!factory) {
            return nullptr;
        }

        // Use factory's CreateFromData method if available
        return factory->CreateSpriteFromData(m_pak.sprites[index], alphaEffect);
    }

    // Get the number of sprites in the PAK
    size_t get_sprite_count() const {
        if (!m_isPakOpen) {
            return 0;
        }
        return m_pak.sprites.size();
    }

    // Get PAK name
    const std::string& get_pak_name() const { return m_pakName; }

    // Check if PAK is open
    bool is_open() const { return m_isPakOpen; }

private:
    SpriteLoader() = default;

    bool m_isPakOpen = false;
    std::string m_pakName;
    PAKLib::pak m_pak;
};

} // namespace SpriteLib

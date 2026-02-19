// Screen_Loading.cpp: Loading Screen Implementation
//
// Handles progressive loading of all game resources including sprites,
// tiles, effects, and sounds. Loading is split across multiple frames
// to allow progress updates.
//
//////////////////////////////////////////////////////////////////////

#include "Screen_Loading.h"
#include "Game.h"
#include "GameModeManager.h"
#include "GameFonts.h"
#include "SpriteID.h"
#include "GlobalDef.h"
#include "AudioManager.h"
#include "WeatherManager.h"
#include "SpriteLoader.h"
#include "ItemSpriteMetadata.h"
#include "Log.h"
using namespace hb::client::sprite_id;

Screen_Loading::Screen_Loading(CGame* game)
    : IGameScreen(game)
{
}

void Screen_Loading::on_initialize()
{
    GameModeManager::set_current_mode(GameMode::Loading);
    m_iLoadingStage = 0;

    // Pre-load the loading screen sprite so it can render immediately
    m_game->m_sprite[InterfaceNdLoading] = hb::shared::sprite::Sprites::create("new-dialog", 0, false);
}

void Screen_Loading::on_uninitialize()
{
    // Nothing to clean up - resources are owned by CGame
}

void Screen_Loading::on_update()
{
    // Process one loading stage per frame
    switch (m_iLoadingStage)
    {
    case 0:   LoadStage_Interface();    break;
    case 4:   LoadStage_Tiles1();       break;
    case 8:   LoadStage_Tiles2();       break;
    case 12:  LoadStage_Tiles3();       break;
    case 16:  LoadStage_Equipment1();   break;
    case 20:  LoadStage_Angels();       break;
    case 24:  LoadStage_Monsters1();    break;
    case 28:  LoadStage_Monsters2();    break;
    case 32:  LoadStage_Monsters3();    break;
    case 36:  LoadStage_Monsters4();    break;
    case 40:  LoadStage_Monsters5();    break;
    case 44:  LoadStage_Monsters6();    break;
    case 48:  LoadStage_CosmeticsMale();   break;
    case 52:  LoadStage_CosmeticsFemale(); break;
    case 56: case 60: case 64: case 68: case 72: case 76:
    case 80: case 84: case 88: case 92: case 96:
              LoadStage_EquipmentBatch();  break;
    case 100: LoadStage_Effects();      break;
    }
}

void Screen_Loading::on_render()
{
    // draw loading background
    draw_new_dialog_box(InterfaceNdLoading, 0, 0, 0, true);
    draw_version();

    // draw progress bar - scale loading stage (0-100) to actual sprite frame width
    int frame_width = m_game->m_sprite[InterfaceNdLoading]->GetFrameRect(1).width;
    int bar_width = (m_iLoadingStage * frame_width) / 100;
    m_game->m_sprite[InterfaceNdLoading]->DrawWidth(626, 552, 1, bar_width);
}

//=============================================================================
// Stage 0: Interface sprites, dialog boxes, maps
//=============================================================================
void Screen_Loading::LoadStage_Interface()
{
    // load interface sprites
    hb::shared::sprite::SpriteLoader::open_pak("interface", [&](hb::shared::sprite::SpriteLoader& loader) {
        m_game->m_sprite[MouseCursor] = loader.get_sprite(0, false);
        m_game->m_sprite[InterfaceSprFonts] = loader.get_sprite(1, false);
    });

    hb::shared::sprite::SpriteLoader::open_pak("newmaps", [&](hb::shared::sprite::SpriteLoader& loader) {
        m_game->m_sprite[InterfaceNewMaps1] = loader.get_sprite(0, false);
        m_game->m_sprite[InterfaceNewMaps2] = loader.get_sprite(1, false);
        m_game->m_sprite[InterfaceNewMaps3] = loader.get_sprite(2, false);
        m_game->m_sprite[InterfaceNewMaps4] = loader.get_sprite(3, false);
        m_game->m_sprite[InterfaceNewMaps5] = loader.get_sprite(4, false);
    });

    m_game->m_sprite[InterfaceNdLogin] = hb::shared::sprite::Sprites::create("logindialog", 0, false);

    hb::shared::sprite::SpriteLoader::open_pak("new-dialog", [&](hb::shared::sprite::SpriteLoader& loader) {
        m_game->m_sprite[InterfaceNdMainMenu] = loader.get_sprite(1, false);
        m_game->m_sprite[InterfaceNdQuit] = loader.get_sprite(2, false);
        m_game->m_sprite[InterfaceNdNewAccount] = loader.get_sprite(2, false);
    });

    hb::shared::sprite::SpriteLoader::open_pak("gamedialog", [&](hb::shared::sprite::SpriteLoader& loader) {
        m_game->m_sprite[InterfaceNdGame1] = loader.get_sprite(0, false);
        m_game->m_sprite[InterfaceNdGame2] = loader.get_sprite(1, false);
        m_game->m_sprite[InterfaceNdGame3] = loader.get_sprite(2, false);
        m_game->m_sprite[InterfaceNdGame4] = loader.get_sprite(3, false);
        m_game->m_sprite[InterfaceNdCrusade] = loader.get_sprite(4, false);
        m_game->m_sprite[InterfaceNdIconPanel] = loader.get_sprite(6, false);
        m_game->m_sprite[InterfaceNdInventory] = loader.get_sprite(7, false);
        m_game->m_sprite[InterfaceNdSelectChar] = loader.get_sprite(8, false);
        m_game->m_sprite[InterfaceNdNewChar] = loader.get_sprite(9, false);
        m_game->m_sprite[InterfaceNdNewExchange] = loader.get_sprite(10, false);
    });

    m_game->m_sprite[InterfaceNdPartyStatus] = hb::shared::sprite::Sprites::create("partysprite", 0, false);

    hb::shared::sprite::SpriteLoader::open_pak("dialogtext", [&](hb::shared::sprite::SpriteLoader& loader) {
        m_game->m_sprite[InterfaceNdText] = loader.get_sprite(0, false);
        m_game->m_sprite[InterfaceNdButton] = loader.get_sprite(1, false);
    });

    make_sprite("telescope", InterfaceGuideMap, 32, false);
    make_sprite("telescope2", InterfaceGuideMap + 35, 4, false);
    make_sprite("monster", InterfaceMonster, 1, false);

    // load interface2 sprites in one batch
    hb::shared::sprite::SpriteLoader::open_pak("interface2", [&](hb::shared::sprite::SpriteLoader& loader) {
        m_game->m_sprite[InterfaceAddInterface] = loader.get_sprite(0, false);
        m_game->m_sprite[InterfaceSprFonts2] = loader.get_sprite(1, false);
        m_game->m_sprite[InterfaceF1HelpWindows] = loader.get_sprite(2, false);
        m_game->m_sprite[InterfaceCrafting] = loader.get_sprite(3, false);
    });

    // load sprfonts sprites in one batch
    hb::shared::sprite::SpriteLoader::open_pak("sprfonts", [&](hb::shared::sprite::SpriteLoader& loader) {
        m_game->m_sprite[InterfaceFont1] = loader.get_sprite(0, false);
        m_game->m_sprite[InterfaceFont2] = loader.get_sprite(1, false);
    });

    // Create and register bitmap fonts with TextLib
    // Font 1: Characters '!' (33) to 'z' (122)
    if (m_game->m_sprite[InterfaceFont1])
    {
        hb::shared::text::load_bitmap_font(GameFont::Bitmap1, m_game->m_sprite[InterfaceFont1].get(),
            '!', 'z', 0, GameFont::GetFontSpacing(GameFont::Bitmap1));
    }

    // Font 2: Characters ' ' (32) to '~' (126), uses dynamic spacing from sprite frames
    if (m_game->m_sprite[InterfaceFont2])
    {
        hb::shared::text::load_bitmap_font_dynamic(GameFont::Bitmap2, m_game->m_sprite[InterfaceFont2].get(), ' ', '~', 0);
    }

    // Number font: Digits '0' to '9', frame offset 6 in ADDINTERFACE sprite
    if (m_game->m_sprite[InterfaceAddInterface])
    {
        hb::shared::text::load_bitmap_font(GameFont::Numbers, m_game->m_sprite[InterfaceAddInterface].get(),
            '0', '9', 6, GameFont::GetFontSpacing(GameFont::Numbers));
    }

    // SPRFONTS2: Characters ' ' (32) to '~' (126), with 3 different sizes (types 0, 1, 2)
    // Each type has 95 frames offset
    if (m_game->m_sprite[InterfaceSprFonts2])
    {
        hb::shared::text::load_bitmap_font_dynamic(GameFont::SprFont3_0, m_game->m_sprite[InterfaceSprFonts2].get(), ' ', '~', 0);
        hb::shared::text::load_bitmap_font_dynamic(GameFont::SprFont3_1, m_game->m_sprite[InterfaceSprFonts2].get(), ' ', '~', 95);
        hb::shared::text::load_bitmap_font_dynamic(GameFont::SprFont3_2, m_game->m_sprite[InterfaceSprFonts2].get(), ' ', '~', 190);
    }

    m_iLoadingStage = 4;
}

//=============================================================================
// Stage 4: Map tiles, structures, trees
//=============================================================================
void Screen_Loading::LoadStage_Tiles1()
{
    make_tile_spr("maptiles1", 0, 32, true);
    m_game->m_tile_spr[1 + 50] = hb::shared::sprite::Sprites::create("structures1", 1, true);
    m_game->m_tile_spr[5 + 50] = hb::shared::sprite::Sprites::create("structures1", 5, true);
    make_tile_spr("sinside1", 70, 27, false);
    make_tile_spr("trees1", 100, 46, true);
    make_tile_spr("treeshadows", 150, 46, true);
    make_tile_spr("objects1", 200, 10, true);
    make_tile_spr("objects2", 211, 5, true);
    make_tile_spr("objects3", 216, 4, true);
    make_tile_spr("objects4", 220, 2, true);

    m_iLoadingStage = 8;
}

//=============================================================================
// Stage 8: More tiles and objects
//=============================================================================
void Screen_Loading::LoadStage_Tiles2()
{
    make_tile_spr("tile223-225", 223, 3, true);
    make_tile_spr("tile226-229", 226, 4, true);
    make_tile_spr("objects5", 230, 9, true);
    make_tile_spr("objects6", 238, 4, true);
    make_tile_spr("objects7", 242, 7, true);
    make_tile_spr("maptiles2", 300, 15, true);
    make_tile_spr("maptiles4", 320, 10, true);
    make_tile_spr("maptiles5", 330, 19, true);
    make_tile_spr("maptiles6", 349, 4, true);
    make_tile_spr("maptiles353-361", 353, 9, true);
    make_tile_spr("tile363-366", 363, 4, true);
    make_tile_spr("tile367-367", 367, 1, true);
    make_tile_spr("tile370-381", 370, 12, true);
    make_tile_spr("tile382-387", 382, 6, true);
    make_tile_spr("tile388-402", 388, 15, true);

    m_iLoadingStage = 12;
}

//=============================================================================
// Stage 12: More tiles, item sprites
//=============================================================================
void Screen_Loading::LoadStage_Tiles3()
{
    make_tile_spr("tile403-405", 403, 3, true);
    make_tile_spr("tile406-421", 406, 16, true);
    make_tile_spr("tile422-429", 422, 8, true);
    make_tile_spr("tile430-443", 430, 14, true);
    make_tile_spr("tile444-444", 444, 1, true);
    make_tile_spr("tile445-461", 445, 17, true);
    make_tile_spr("tile462-473", 462, 12, true);
    make_tile_spr("tile474-478", 474, 5, true);
    make_tile_spr("tile479-488", 479, 10, true);
    make_tile_spr("tile489-522", 489, 34, true);
    make_tile_spr("tile523-530", 523, 8, true);
    make_tile_spr("tile531-540", 531, 10, true);
    make_tile_spr("tile541-545", 541, 5, true);

    make_sprite("item-dynamic", ItemDynamicPivotPoint, 3, false);

    // Item atlas sprites (unified atlas for new display_id system)
    hb::shared::sprite::SpriteLoader::open_pak("item_atlas", [&](hb::shared::sprite::SpriteLoader& loader) {
        for (size_t i = 0; i < 3 && i < loader.get_sprite_count(); i++) {
            m_game->m_item_sprites[i] = loader.get_sprite(i, false);
        }
        hb::logger::log("Item atlas loaded: {} sprites", loader.get_sprite_count());
    });

    // Item sprite metadata (display_id → frame index mappings)
    item_sprite_manager::get().load("contents/ItemSpriteMetadata.json");

    m_iLoadingStage = 16;
}

//=============================================================================
// Stage 16: Male/Female equipment base, player bodies
//=============================================================================
void Screen_Loading::LoadStage_Equipment1()
{
    // Male equipment
    hb::shared::sprite::SpriteLoader::open_pak("item-equipm", [&](hb::shared::sprite::SpriteLoader& loader) {
        m_game->m_sprite[ItemEquipPivotPoint + 0] = loader.get_sprite(0, false);
        m_game->m_sprite[ItemEquipPivotPoint + 1] = loader.get_sprite(1, false);
        m_game->m_sprite[ItemEquipPivotPoint + 2] = loader.get_sprite(2, false);
        m_game->m_sprite[ItemEquipPivotPoint + 3] = loader.get_sprite(3, false);
        m_game->m_sprite[ItemEquipPivotPoint + 4] = loader.get_sprite(4, false);
        m_game->m_sprite[ItemEquipPivotPoint + 5] = loader.get_sprite(5, false);
        m_game->m_sprite[ItemEquipPivotPoint + 7] = loader.get_sprite(6, false);
        m_game->m_sprite[ItemEquipPivotPoint + 8] = loader.get_sprite(7, false);
        m_game->m_sprite[ItemEquipPivotPoint + 9] = loader.get_sprite(8, false);
        m_game->m_sprite[ItemEquipPivotPoint + 15] = loader.get_sprite(11, false);
        m_game->m_sprite[ItemEquipPivotPoint + 17] = loader.get_sprite(12, false);
        m_game->m_sprite[ItemEquipPivotPoint + 18] = loader.get_sprite(9, false);
        m_game->m_sprite[ItemEquipPivotPoint + 19] = loader.get_sprite(10, false);
        m_game->m_sprite[ItemEquipPivotPoint + 20] = loader.get_sprite(13, false);
        m_game->m_sprite[ItemEquipPivotPoint + 21] = loader.get_sprite(14, false);
    });

    // Female equipment
    hb::shared::sprite::SpriteLoader::open_pak("item-equipw", [&](hb::shared::sprite::SpriteLoader& loader) {
        m_game->m_sprite[ItemEquipPivotPoint + 40] = loader.get_sprite(0, false);
        m_game->m_sprite[ItemEquipPivotPoint + 41] = loader.get_sprite(1, false);
        m_game->m_sprite[ItemEquipPivotPoint + 42] = loader.get_sprite(2, false);
        m_game->m_sprite[ItemEquipPivotPoint + 43] = loader.get_sprite(3, false);
        m_game->m_sprite[ItemEquipPivotPoint + 45] = loader.get_sprite(4, false);
        m_game->m_sprite[ItemEquipPivotPoint + 50] = loader.get_sprite(5, false);
        m_game->m_sprite[ItemEquipPivotPoint + 51] = loader.get_sprite(6, false);
        m_game->m_sprite[ItemEquipPivotPoint + 52] = loader.get_sprite(7, false);
        m_game->m_sprite[ItemEquipPivotPoint + 53] = loader.get_sprite(8, false);
        m_game->m_sprite[ItemEquipPivotPoint + 55] = loader.get_sprite(11, false);
        m_game->m_sprite[ItemEquipPivotPoint + 57] = loader.get_sprite(12, false);
        m_game->m_sprite[ItemEquipPivotPoint + 58] = loader.get_sprite(9, false);
        m_game->m_sprite[ItemEquipPivotPoint + 59] = loader.get_sprite(10, false);
        m_game->m_sprite[ItemEquipPivotPoint + 60] = loader.get_sprite(13, false);
        m_game->m_sprite[ItemEquipPivotPoint + 61] = loader.get_sprite(14, false);
    });

    // Player body sprites
    make_sprite("bm", 500 + 15 * 8 * 0, 96, true);  // Black Man
    make_sprite("wm", 500 + 15 * 8 * 1, 96, true);  // White Man
    make_sprite("ym", 500 + 15 * 8 * 2, 96, true);  // Yellow Man

    m_iLoadingStage = 20;
}

//=============================================================================
// Stage 20: Tutelary angels, female player bodies
//=============================================================================
void Screen_Loading::LoadStage_Angels()
{
    make_sprite("tutelarangel1", TutelaryAngelsPivotPoint + 50 * 0, 48, false);
    make_sprite("tutelarangel2", TutelaryAngelsPivotPoint + 50 * 1, 48, false);
    make_sprite("tutelarangel3", TutelaryAngelsPivotPoint + 50 * 2, 48, false);
    make_sprite("tutelarangel4", TutelaryAngelsPivotPoint + 50 * 3, 48, false);

    make_sprite("bw", 500 + 15 * 8 * 3, 96, true);  // Black Woman
    make_sprite("ww", 500 + 15 * 8 * 4, 96, true);  // White Woman
    make_sprite("yw", 500 + 15 * 8 * 5, 96, true);  // Yellow Woman

    m_iLoadingStage = 24;
}

//=============================================================================
// Stage 24: Monsters (Slime to William)
//=============================================================================
void Screen_Loading::LoadStage_Monsters1()
{
    make_sprite("slm", Mob + 7 * 8 * 0, 40, true);
    make_sprite("ske", Mob + 7 * 8 * 1, 40, true);
    make_sprite("gol", Mob + 7 * 8 * 2, 40, true);
    make_sprite("cyc", Mob + 7 * 8 * 3, 40, true);
    make_sprite("orc", Mob + 7 * 8 * 4, 40, true);
    make_sprite("shopkpr", Mob + 7 * 8 * 5, 8, true);
    make_sprite("ant", Mob + 7 * 8 * 6, 40, true);
    make_sprite("scp", Mob + 7 * 8 * 7, 40, true);
    make_sprite("zom", Mob + 7 * 8 * 8, 40, true);
    make_sprite("gandlf", Mob + 7 * 8 * 9, 8, true);
    make_sprite("howard", Mob + 7 * 8 * 10, 8, true);
    make_sprite("guard", Mob + 7 * 8 * 11, 40, true);
    make_sprite("amp", Mob + 7 * 8 * 12, 40, true);
    make_sprite("cla", Mob + 7 * 8 * 13, 40, true);
    make_sprite("tom", Mob + 7 * 8 * 14, 8, true);
    make_sprite("william", Mob + 7 * 8 * 15, 8, true);

    m_iLoadingStage = 28;
}

//=============================================================================
// Stage 28: Monsters (Kennedy to Energy Ball)
//=============================================================================
void Screen_Loading::LoadStage_Monsters2()
{
    make_sprite("kennedy", Mob + 7 * 8 * 16, 8, true);
    make_sprite("helb", Mob + 7 * 8 * 17, 40, true);
    make_sprite("troll", Mob + 7 * 8 * 18, 40, true);
    make_sprite("orge", Mob + 7 * 8 * 19, 40, true);
    make_sprite("liche", Mob + 7 * 8 * 20, 40, true);
    make_sprite("demon", Mob + 7 * 8 * 21, 40, true);
    make_sprite("unicorn", Mob + 7 * 8 * 22, 40, true);
    make_sprite("werewolf", Mob + 7 * 8 * 23, 40, true);
    make_sprite("dummy", Mob + 7 * 8 * 24, 40, true);

    // Energy Ball - all 40 slots use the same sprite
    hb::shared::sprite::SpriteLoader::open_pak("effect5", [&](hb::shared::sprite::SpriteLoader& loader) {
        for (int i = 0; i < 40; i++)
            m_game->m_sprite[Mob + i + 7 * 8 * 25] = loader.get_sprite(0, true);
    });

    m_iLoadingStage = 32;
}

//=============================================================================
// Stage 32: Guard towers, structures
//=============================================================================
void Screen_Loading::LoadStage_Monsters3()
{
    make_sprite("gt-arrow", Mob + 7 * 8 * 26, 40, true);
    make_sprite("gt-cannon", Mob + 7 * 8 * 27, 40, true);
    make_sprite("manacollector", Mob + 7 * 8 * 28, 40, true);
    make_sprite("detector", Mob + 7 * 8 * 29, 40, true);
    make_sprite("esg", Mob + 7 * 8 * 30, 40, true);
    make_sprite("gmg", Mob + 7 * 8 * 31, 40, true);
    make_sprite("manastone", Mob + 7 * 8 * 32, 40, true);
    make_sprite("lwb", Mob + 7 * 8 * 33, 40, true);
    make_sprite("ghk", Mob + 7 * 8 * 34, 40, true);
    make_sprite("ghkabs", Mob + 7 * 8 * 35, 40, true);
    make_sprite("tk", Mob + 7 * 8 * 36, 40, true);
    make_sprite("bg", Mob + 7 * 8 * 37, 40, true);

    m_iLoadingStage = 36;
}

//=============================================================================
// Stage 36: More monsters
//=============================================================================
void Screen_Loading::LoadStage_Monsters4()
{
    make_sprite("stalker", Mob + 7 * 8 * 38, 40, true);
    make_sprite("hellclaw", Mob + 7 * 8 * 39, 40, true);
    make_sprite("tigerworm", Mob + 7 * 8 * 40, 40, true);
    make_sprite("catapult", Mob + 7 * 8 * 41, 40, true);
    make_sprite("gagoyle", Mob + 7 * 8 * 42, 40, true);
    make_sprite("beholder", Mob + 7 * 8 * 43, 40, true);
    make_sprite("darkelf", Mob + 7 * 8 * 44, 40, true);
    make_sprite("bunny", Mob + 7 * 8 * 45, 40, true);
    make_sprite("cat", Mob + 7 * 8 * 46, 40, true);
    make_sprite("giantfrog", Mob + 7 * 8 * 47, 40, true);
    make_sprite("mtgiant", Mob + 7 * 8 * 48, 40, true);

    m_iLoadingStage = 40;
}

//=============================================================================
// Stage 40: More monsters
//=============================================================================
void Screen_Loading::LoadStage_Monsters5()
{
    make_sprite("ettin", Mob + 7 * 8 * 49, 40, true);
    make_sprite("canplant", Mob + 7 * 8 * 50, 40, true);
    make_sprite("rudolph", Mob + 7 * 8 * 51, 40, true);
    make_sprite("direboar", Mob + 7 * 8 * 52, 40, true);
    make_sprite("frost", Mob + 7 * 8 * 53, 40, true);
    make_sprite("crop", Mob + 7 * 8 * 54, 40, true);
    make_sprite("icegolem", Mob + 7 * 8 * 55, 40, true);
    make_sprite("wyvern", Mob + 7 * 8 * 56, 24, true);
    make_sprite("mcgaffin", Mob + 7 * 8 * 57, 16, true);
    make_sprite("perry", Mob + 7 * 8 * 58, 16, true);
    make_sprite("devlin", Mob + 7 * 8 * 59, 16, true);
    make_sprite("barlog", Mob + 7 * 8 * 60, 40, true);
    make_sprite("centaurus", Mob + 7 * 8 * 61, 40, true);
    make_sprite("clawturtle", Mob + 7 * 8 * 62, 40, true);
    make_sprite("firewyvern", Mob + 7 * 8 * 63, 24, true);
    make_sprite("giantcrayfish", Mob + 7 * 8 * 64, 40, true);
    make_sprite("giantlizard", Mob + 7 * 8 * 65, 40, true);

    m_iLoadingStage = 44;
}

//=============================================================================
// Stage 44: New NPCs and monsters
//=============================================================================
void Screen_Loading::LoadStage_Monsters6()
{
    make_sprite("giantplant", Mob + 7 * 8 * 66, 40, true);
    make_sprite("mastermageorc", Mob + 7 * 8 * 67, 40, true);
    make_sprite("minotaurs", Mob + 7 * 8 * 68, 40, true);
    make_sprite("nizie", Mob + 7 * 8 * 69, 40, true);
    make_sprite("tentocle", Mob + 7 * 8 * 70, 40, true);
    make_sprite("yspro", Mob + 7 * 8 * 71, 32, true);
    make_sprite("sorceress", Mob + 7 * 8 * 72, 40, true);
    make_sprite("tpknight", Mob + 7 * 8 * 73, 40, true);
    make_sprite("elfmaster", Mob + 7 * 8 * 74, 40, true);
    make_sprite("darkknight", Mob + 7 * 8 * 75, 40, true);
    make_sprite("hbtank", Mob + 7 * 8 * 76, 32, true);
    make_sprite("cbturret", Mob + 7 * 8 * 77, 32, true);
    make_sprite("babarian", Mob + 7 * 8 * 78, 40, true);
    make_sprite("acannon", Mob + 7 * 8 * 79, 32, true);

    m_iLoadingStage = 48;
}

//=============================================================================
// Stage 48: Gail, Gate, Male cosmetics (underwear, hair)
//=============================================================================
void Screen_Loading::LoadStage_CosmeticsMale()
{
    make_sprite("gail", Mob + 7 * 8 * 80, 8, true);
    make_sprite("gate", Mob + 7 * 8 * 81, 24, true);

    // Male underwear
    hb::shared::sprite::SpriteLoader::open_pak("mpt", [&](hb::shared::sprite::SpriteLoader& loader) {
        for (int g = 0; g < 8; g++) {
            for (int i = 0; i < 12; i++) {
                m_game->m_sprite[UndiesM + i + 15 * g] = loader.get_sprite(i + 12 * g, true);
            }
        }
    });

    // Male hair
    hb::shared::sprite::SpriteLoader::open_pak("mhr", [&](hb::shared::sprite::SpriteLoader& loader) {
        for (int g = 0; g < 8; g++) {
            for (int i = 0; i < 12; i++) {
                m_game->m_sprite[HairM + i + 15 * g] = loader.get_sprite(i + 12 * g, true);
            }
        }
    });

    m_iLoadingStage = 52;
}

//=============================================================================
// Stage 52: Female cosmetics (underwear, hair)
//=============================================================================
void Screen_Loading::LoadStage_CosmeticsFemale()
{
    // Female underwear
    hb::shared::sprite::SpriteLoader::open_pak("wpt", [&](hb::shared::sprite::SpriteLoader& loader) {
        for (int g = 0; g < 8; g++) {
            for (int i = 0; i < 12; i++) {
                m_game->m_sprite[UndiesW + i + 15 * g] = loader.get_sprite(i + 12 * g, true);
            }
        }
    });

    // Female hair
    hb::shared::sprite::SpriteLoader::open_pak("whr", [&](hb::shared::sprite::SpriteLoader& loader) {
        for (int g = 0; g < 8; g++) {
            for (int i = 0; i < 12; i++) {
                m_game->m_sprite[HairW + i + 15 * g] = loader.get_sprite(i + 12 * g, true);
            }
        }
    });

    m_iLoadingStage = 56;
}

//=============================================================================
// Stages 56-96: Per-item equipment sprites from metadata (batched)
//=============================================================================
void Screen_Loading::LoadStage_EquipmentBatch()
{
    constexpr int equip_stages = 11; // stages 56, 60, 64, 68, 72, 76, 80, 84, 88, 92, 96

    // Count total loadable entries
    int total = 0;
    item_sprite_manager::get().for_each_equippable([&](const item_sprite_entry&) {
        total++;
    });

    int per_batch = (total + equip_stages - 1) / equip_stages;
    int batch_end = m_equip_load_cursor + per_batch;

    int current = 0;
    item_sprite_manager::get().for_each_equippable([&](const item_sprite_entry& entry) {
        if (current < m_equip_load_cursor || current >= batch_end) { current++; return; }
        current++;

        if (entry.pak_file.empty()) return;

        // Strip .pak extension and prepend items/ subdirectory
        std::string pak_name = entry.pak_file;
        if (pak_name.size() > 4 && pak_name.compare(pak_name.size() - 4, 4, ".pak") == 0)
            pak_name.resize(pak_name.size() - 4);
        std::string pak_path = "items/" + pak_name;

        try {
            hb::shared::sprite::SpriteLoader::open_pak(pak_path.c_str(), [&](hb::shared::sprite::SpriteLoader& loader) {
                size_t sprite_count = loader.get_sprite_count();
                // Male sprites: pak indices 0-11
                for (int pose = 0; pose < equip_sprite::sprites_per_item; pose++) {
                    int slot = equip_sprite::index(false, entry.id, pose);
                    if (slot >= 0 && static_cast<size_t>(pose) < sprite_count)
                        m_game->m_equip_sprites[slot] = loader.get_sprite(pose, true);
                }
                // Female sprites: pak indices 12-23
                for (int pose = 0; pose < equip_sprite::sprites_per_item; pose++) {
                    int pak_idx = equip_sprite::sprites_per_item + pose;
                    int slot = equip_sprite::index(true, entry.id, pose);
                    if (slot >= 0 && static_cast<size_t>(pak_idx) < sprite_count)
                        m_game->m_equip_sprites[slot] = loader.get_sprite(pak_idx, true);
                }
            });
        } catch (const std::exception& ex) {
            hb::logger::warn("Failed to load equipment pak '{}': {}", pak_path, ex.what());
        }
    });

    m_equip_load_cursor = batch_end;

    if (m_iLoadingStage == 96) {
        hb::logger::log("Equipment sprites: loaded {} equippable items into m_equip_sprites", total);
    }

    m_iLoadingStage += 4;
}

//=============================================================================
// Stage 100: Effects, sounds, finish loading
//=============================================================================
void Screen_Loading::LoadStage_Effects()
{
    make_effect_spr("effect", 0, 10, false);
    make_effect_spr("effect2", 10, 3, false);
    make_effect_spr("effect3", 13, 6, false);
    make_effect_spr("effect4", 19, 5, false);

    // Effect5 batch load
    hb::shared::sprite::SpriteLoader::open_pak("effect5", [&](hb::shared::sprite::SpriteLoader& loader) {
        for (int i = 0; i <= 6; i++) {
            m_game->m_effect_sprites[i + 24] = loader.get_sprite(i + 1, false);
        }
    });

    make_effect_spr("crueffect1", 31, 9, false);
    make_effect_spr("effect6", 40, 5, false);
    make_effect_spr("effect7", 45, 12, false);
    make_effect_spr("effect8", 57, 9, false);
    make_effect_spr("effect9", 66, 21, false);
    make_effect_spr("effect10", 87, 2, false);
    make_effect_spr("effect11", 89, 14, false);
    make_effect_spr("effect11s", 104, 1, false);
    make_effect_spr("yseffect2", 140, 8, false);
    make_effect_spr("effect12", 148, 4, false);
    make_effect_spr("yseffect3", 152, 16, false);
    make_effect_spr("yseffect4", 133, 7, false);

    // initialize effect_manager with loaded sprites
    m_game->m_effect_manager->set_effect_sprites(m_game->m_effect_sprites);
    weather_manager::get().set_dependencies(*m_game->m_Renderer, m_game->m_effect_sprites, m_game->m_Camera);
    weather_manager::get().set_map_data(m_game->m_map_data.get());

    // load all sound effects
    audio_manager::get().load_sounds();

    // Loading complete - transition to main menu
    m_game->change_game_mode(GameMode::MainMenu);
}

//=============================================================================
// Sprite loading helpers
//=============================================================================
void Screen_Loading::make_sprite(const char* FileName, short start, short count, bool alpha_effect)
{
    try {
        hb::shared::sprite::SpriteLoader::open_pak(FileName, [&](hb::shared::sprite::SpriteLoader& loader) {
            size_t totalInPak = loader.get_sprite_count();
            size_t toLoad = static_cast<size_t>(count);
            if (toLoad > totalInPak) toLoad = totalInPak;

            for (size_t i = 0; i < toLoad; i++) {
                m_game->m_sprite[i + start] = loader.get_sprite(i, alpha_effect);
            }
        });
    } catch (const std::exception& e) {
        printf("[make_sprite] FAILED %s: %s\n", FileName, e.what());
    } catch (...) {
        printf("[make_sprite] FAILED %s: unknown exception\n", FileName);
    }
}

void Screen_Loading::make_tile_spr(const char* FileName, short start, short count, bool alpha_effect)
{
    try {
        hb::shared::sprite::SpriteLoader::open_pak(FileName, [&](hb::shared::sprite::SpriteLoader& loader) {
            size_t totalInPak = loader.get_sprite_count();
            size_t toLoad = static_cast<size_t>(count);
            if (toLoad > totalInPak) toLoad = totalInPak;

            for (size_t i = 0; i < toLoad; i++) {
                m_game->m_tile_spr[i + start] = loader.get_sprite(i, alpha_effect);
            }
        });
    } catch (const std::exception& e) {
        printf("[make_tile_spr] FAILED %s: %s\n", FileName, e.what());
    }
}

void Screen_Loading::make_effect_spr(const char* FileName, short start, short count, bool alpha_effect)
{
    try {
        hb::shared::sprite::SpriteLoader::open_pak(FileName, [&](hb::shared::sprite::SpriteLoader& loader) {
            size_t totalInPak = loader.get_sprite_count();
            size_t toLoad = static_cast<size_t>(count);
            if (toLoad > totalInPak) toLoad = totalInPak;

            for (size_t i = 0; i < toLoad; i++) {
                m_game->m_effect_sprites[i + start] = loader.get_sprite(i, alpha_effect);
            }
        });
    } catch (const std::exception& e) {
        printf("[make_effect_spr] FAILED %s: %s\n", FileName, e.what());
    }
}

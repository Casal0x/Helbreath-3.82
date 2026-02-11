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
using namespace hb::client::sprite_id;

Screen_Loading::Screen_Loading(CGame* pGame)
    : IGameScreen(pGame)
{
}

void Screen_Loading::on_initialize()
{
    GameModeManager::SetCurrentMode(GameMode::Loading);
    m_iLoadingStage = 0;

    // Pre-load the loading screen sprite so it can render immediately
    m_pGame->m_pSprite[InterfaceNdLoading] = hb::shared::sprite::Sprites::Create("New-Dialog", 0, false);
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
    case 48:  LoadStage_MaleUndies();   break;
    case 52:  LoadStage_MaleArmor();    break;
    case 56:  LoadStage_MaleLegs();     break;
    case 60:  LoadStage_MaleSwords();   break;
    case 64:  LoadStage_MaleWeapons();  break;
    case 68:  LoadStage_MaleBows();     break;
    case 72:  LoadStage_FemaleBase();   break;
    case 76:  LoadStage_FemaleArmor();  break;
    case 80:  LoadStage_FemaleLegs();   break;
    case 84:  LoadStage_FemaleSwords(); break;
    case 88:  LoadStage_FemaleWeapons(); break;
    case 92:  LoadStage_FemaleMantles(); break;
    case 96:  LoadStage_FemaleBows();   break;
    case 100: LoadStage_Effects();      break;
    }
}

void Screen_Loading::on_render()
{
    // Draw loading background
    DrawNewDialogBox(InterfaceNdLoading, 0, 0, 0, true);
    DrawVersion();

    // Draw progress bar - scale loading stage (0-100) to actual sprite frame width
    int iFrameWidth = m_pGame->m_pSprite[InterfaceNdLoading]->GetFrameRect(1).width;
    int iBarWidth = (m_iLoadingStage * iFrameWidth) / 100;
    m_pGame->m_pSprite[InterfaceNdLoading]->DrawWidth(626, 552, 1, iBarWidth);
}

//=============================================================================
// Stage 0: Interface sprites, dialog boxes, maps
//=============================================================================
void Screen_Loading::LoadStage_Interface()
{
    // Load interface sprites
    hb::shared::sprite::SpriteLoader::open_pak("interface", [&](hb::shared::sprite::SpriteLoader& loader) {
        m_pGame->m_pSprite[MouseCursor] = loader.get_sprite(0, false);
        m_pGame->m_pSprite[InterfaceSprFonts] = loader.get_sprite(1, false);
    });

    hb::shared::sprite::SpriteLoader::open_pak("newmaps", [&](hb::shared::sprite::SpriteLoader& loader) {
        m_pGame->m_pSprite[InterfaceNewMaps1] = loader.get_sprite(0, false);
        m_pGame->m_pSprite[InterfaceNewMaps2] = loader.get_sprite(1, false);
        m_pGame->m_pSprite[InterfaceNewMaps3] = loader.get_sprite(2, false);
        m_pGame->m_pSprite[InterfaceNewMaps4] = loader.get_sprite(3, false);
        m_pGame->m_pSprite[InterfaceNewMaps5] = loader.get_sprite(4, false);
    });

    m_pGame->m_pSprite[InterfaceNdLogin] = hb::shared::sprite::Sprites::Create("LoginDialog", 0, false);

    hb::shared::sprite::SpriteLoader::open_pak("New-Dialog", [&](hb::shared::sprite::SpriteLoader& loader) {
        m_pGame->m_pSprite[InterfaceNdMainMenu] = loader.get_sprite(1, false);
        m_pGame->m_pSprite[InterfaceNdQuit] = loader.get_sprite(2, false);
        m_pGame->m_pSprite[InterfaceNdNewAccount] = loader.get_sprite(2, false);
    });

    hb::shared::sprite::SpriteLoader::open_pak("GameDialog", [&](hb::shared::sprite::SpriteLoader& loader) {
        m_pGame->m_pSprite[InterfaceNdGame1] = loader.get_sprite(0, false);
        m_pGame->m_pSprite[InterfaceNdGame2] = loader.get_sprite(1, false);
        m_pGame->m_pSprite[InterfaceNdGame3] = loader.get_sprite(2, false);
        m_pGame->m_pSprite[InterfaceNdGame4] = loader.get_sprite(3, false);
        m_pGame->m_pSprite[InterfaceNdCrusade] = loader.get_sprite(4, false);
        m_pGame->m_pSprite[InterfaceNdIconPanel] = loader.get_sprite(6, false);
        m_pGame->m_pSprite[InterfaceNdInventory] = loader.get_sprite(7, false);
        m_pGame->m_pSprite[InterfaceNdSelectChar] = loader.get_sprite(8, false);
        m_pGame->m_pSprite[InterfaceNdNewChar] = loader.get_sprite(9, false);
        m_pGame->m_pSprite[InterfaceNdNewExchange] = loader.get_sprite(10, false);
    });

    m_pGame->m_pSprite[InterfaceNdPartyStatus] = hb::shared::sprite::Sprites::Create("PartySprite", 0, false);

    hb::shared::sprite::SpriteLoader::open_pak("DialogText", [&](hb::shared::sprite::SpriteLoader& loader) {
        m_pGame->m_pSprite[InterfaceNdText] = loader.get_sprite(0, false);
        m_pGame->m_pSprite[InterfaceNdButton] = loader.get_sprite(1, false);
    });

    MakeSprite("Telescope", InterfaceGuideMap, 32, false);
    MakeSprite("Telescope2", InterfaceGuideMap + 35, 4, false);
    MakeSprite("monster", InterfaceMonster, 1, false);

    // Load interface2 sprites in one batch
    hb::shared::sprite::SpriteLoader::open_pak("interface2", [&](hb::shared::sprite::SpriteLoader& loader) {
        m_pGame->m_pSprite[InterfaceAddInterface] = loader.get_sprite(0, false);
        m_pGame->m_pSprite[InterfaceSprFonts2] = loader.get_sprite(1, false);
        m_pGame->m_pSprite[InterfaceF1HelpWindows] = loader.get_sprite(2, false);
        m_pGame->m_pSprite[InterfaceCrafting] = loader.get_sprite(3, false);
    });

    // Load sprfonts sprites in one batch
    hb::shared::sprite::SpriteLoader::open_pak("sprfonts", [&](hb::shared::sprite::SpriteLoader& loader) {
        m_pGame->m_pSprite[InterfaceFont1] = loader.get_sprite(0, false);
        m_pGame->m_pSprite[InterfaceFont2] = loader.get_sprite(1, false);
    });

    // Create and register bitmap fonts with TextLib
    // Font 1: Characters '!' (33) to 'z' (122)
    if (m_pGame->m_pSprite[InterfaceFont1])
    {
        hb::shared::text::LoadBitmapFont(GameFont::Bitmap1, m_pGame->m_pSprite[InterfaceFont1].get(),
            '!', 'z', 0, GameFont::GetFontSpacing(GameFont::Bitmap1));
    }

    // Font 2: Characters ' ' (32) to '~' (126), uses dynamic spacing from sprite frames
    if (m_pGame->m_pSprite[InterfaceFont2])
    {
        hb::shared::text::LoadBitmapFontDynamic(GameFont::Bitmap2, m_pGame->m_pSprite[InterfaceFont2].get(), ' ', '~', 0);
    }

    // Number font: Digits '0' to '9', frame offset 6 in ADDINTERFACE sprite
    if (m_pGame->m_pSprite[InterfaceAddInterface])
    {
        hb::shared::text::LoadBitmapFont(GameFont::Numbers, m_pGame->m_pSprite[InterfaceAddInterface].get(),
            '0', '9', 6, GameFont::GetFontSpacing(GameFont::Numbers));
    }

    // SPRFONTS2: Characters ' ' (32) to '~' (126), with 3 different sizes (types 0, 1, 2)
    // Each type has 95 frames offset
    if (m_pGame->m_pSprite[InterfaceSprFonts2])
    {
        hb::shared::text::LoadBitmapFontDynamic(GameFont::SprFont3_0, m_pGame->m_pSprite[InterfaceSprFonts2].get(), ' ', '~', 0);
        hb::shared::text::LoadBitmapFontDynamic(GameFont::SprFont3_1, m_pGame->m_pSprite[InterfaceSprFonts2].get(), ' ', '~', 95);
        hb::shared::text::LoadBitmapFontDynamic(GameFont::SprFont3_2, m_pGame->m_pSprite[InterfaceSprFonts2].get(), ' ', '~', 190);
    }

    m_iLoadingStage = 4;
}

//=============================================================================
// Stage 4: Map tiles, structures, trees
//=============================================================================
void Screen_Loading::LoadStage_Tiles1()
{
    MakeTileSpr("maptiles1", 0, 32, true);
    m_pGame->m_pTileSpr[1 + 50] = hb::shared::sprite::Sprites::Create("structures1", 1, true);
    m_pGame->m_pTileSpr[5 + 50] = hb::shared::sprite::Sprites::Create("structures1", 5, true);
    MakeTileSpr("Sinside1", 70, 27, false);
    MakeTileSpr("Trees1", 100, 46, true);
    MakeTileSpr("TreeShadows", 150, 46, true);
    MakeTileSpr("objects1", 200, 10, true);
    MakeTileSpr("objects2", 211, 5, true);
    MakeTileSpr("objects3", 216, 4, true);
    MakeTileSpr("objects4", 220, 2, true);

    m_iLoadingStage = 8;
}

//=============================================================================
// Stage 8: More tiles and objects
//=============================================================================
void Screen_Loading::LoadStage_Tiles2()
{
    MakeTileSpr("Tile223-225", 223, 3, true);
    MakeTileSpr("Tile226-229", 226, 4, true);
    MakeTileSpr("objects5", 230, 9, true);
    MakeTileSpr("objects6", 238, 4, true);
    MakeTileSpr("objects7", 242, 7, true);
    MakeTileSpr("maptiles2", 300, 15, true);
    MakeTileSpr("maptiles4", 320, 10, true);
    MakeTileSpr("maptiles5", 330, 19, true);
    MakeTileSpr("maptiles6", 349, 4, true);
    MakeTileSpr("maptiles353-361", 353, 9, true);
    MakeTileSpr("Tile363-366", 363, 4, true);
    MakeTileSpr("Tile367-367", 367, 1, true);
    MakeTileSpr("Tile370-381", 370, 12, true);
    MakeTileSpr("Tile382-387", 382, 6, true);
    MakeTileSpr("Tile388-402", 388, 15, true);

    m_iLoadingStage = 12;
}

//=============================================================================
// Stage 12: More tiles, item sprites
//=============================================================================
void Screen_Loading::LoadStage_Tiles3()
{
    MakeTileSpr("Tile403-405", 403, 3, true);
    MakeTileSpr("Tile406-421", 406, 16, true);
    MakeTileSpr("Tile422-429", 422, 8, true);
    MakeTileSpr("Tile430-443", 430, 14, true);
    MakeTileSpr("Tile444-444", 444, 1, true);
    MakeTileSpr("Tile445-461", 445, 17, true);
    MakeTileSpr("Tile462-473", 462, 12, true);
    MakeTileSpr("Tile474-478", 474, 5, true);
    MakeTileSpr("Tile479-488", 479, 10, true);
    MakeTileSpr("Tile489-522", 489, 34, true);
    MakeTileSpr("Tile523-530", 523, 8, true);
    MakeTileSpr("Tile531-540", 531, 10, true);
    MakeTileSpr("Tile541-545", 541, 5, true);

    // Item pack sprites
    hb::shared::sprite::SpriteLoader::open_pak("item-pack", [&](hb::shared::sprite::SpriteLoader& loader) {
        for (size_t i = 0; i < 27 && i < loader.get_sprite_count(); i++) {
            m_pGame->m_pSprite[ItemPackPivotPoint + 1 + i] = loader.get_sprite(i, false);
        }
        m_pGame->m_pSprite[ItemPackPivotPoint + 20] = loader.get_sprite(17, false);
        m_pGame->m_pSprite[ItemPackPivotPoint + 21] = loader.get_sprite(18, false);
        m_pGame->m_pSprite[ItemPackPivotPoint + 22] = loader.get_sprite(19, false);
    });

    // Item ground sprites
    hb::shared::sprite::SpriteLoader::open_pak("item-ground", [&](hb::shared::sprite::SpriteLoader& loader) {
        for (size_t i = 0; i < 19 && i < loader.get_sprite_count(); i++) {
            m_pGame->m_pSprite[ItemGroundPivotPoint + 1 + i] = loader.get_sprite(i, false);
        }
        m_pGame->m_pSprite[ItemGroundPivotPoint + 20] = loader.get_sprite(17, false);
        m_pGame->m_pSprite[ItemGroundPivotPoint + 21] = loader.get_sprite(18, false);
        m_pGame->m_pSprite[ItemGroundPivotPoint + 22] = loader.get_sprite(19, false);
    });

    MakeSprite("item-dynamic", ItemDynamicPivotPoint, 3, false);

    m_iLoadingStage = 16;
}

//=============================================================================
// Stage 16: Male/Female equipment base, player bodies
//=============================================================================
void Screen_Loading::LoadStage_Equipment1()
{
    // Male equipment
    hb::shared::sprite::SpriteLoader::open_pak("item-equipM", [&](hb::shared::sprite::SpriteLoader& loader) {
        m_pGame->m_pSprite[ItemEquipPivotPoint + 0] = loader.get_sprite(0, false);
        m_pGame->m_pSprite[ItemEquipPivotPoint + 1] = loader.get_sprite(1, false);
        m_pGame->m_pSprite[ItemEquipPivotPoint + 2] = loader.get_sprite(2, false);
        m_pGame->m_pSprite[ItemEquipPivotPoint + 3] = loader.get_sprite(3, false);
        m_pGame->m_pSprite[ItemEquipPivotPoint + 4] = loader.get_sprite(4, false);
        m_pGame->m_pSprite[ItemEquipPivotPoint + 5] = loader.get_sprite(5, false);
        m_pGame->m_pSprite[ItemEquipPivotPoint + 7] = loader.get_sprite(6, false);
        m_pGame->m_pSprite[ItemEquipPivotPoint + 8] = loader.get_sprite(7, false);
        m_pGame->m_pSprite[ItemEquipPivotPoint + 9] = loader.get_sprite(8, false);
        m_pGame->m_pSprite[ItemEquipPivotPoint + 15] = loader.get_sprite(11, false);
        m_pGame->m_pSprite[ItemEquipPivotPoint + 17] = loader.get_sprite(12, false);
        m_pGame->m_pSprite[ItemEquipPivotPoint + 18] = loader.get_sprite(9, false);
        m_pGame->m_pSprite[ItemEquipPivotPoint + 19] = loader.get_sprite(10, false);
        m_pGame->m_pSprite[ItemEquipPivotPoint + 20] = loader.get_sprite(13, false);
        m_pGame->m_pSprite[ItemEquipPivotPoint + 21] = loader.get_sprite(14, false);
    });

    // Female equipment
    hb::shared::sprite::SpriteLoader::open_pak("item-equipW", [&](hb::shared::sprite::SpriteLoader& loader) {
        m_pGame->m_pSprite[ItemEquipPivotPoint + 40] = loader.get_sprite(0, false);
        m_pGame->m_pSprite[ItemEquipPivotPoint + 41] = loader.get_sprite(1, false);
        m_pGame->m_pSprite[ItemEquipPivotPoint + 42] = loader.get_sprite(2, false);
        m_pGame->m_pSprite[ItemEquipPivotPoint + 43] = loader.get_sprite(3, false);
        m_pGame->m_pSprite[ItemEquipPivotPoint + 45] = loader.get_sprite(4, false);
        m_pGame->m_pSprite[ItemEquipPivotPoint + 50] = loader.get_sprite(5, false);
        m_pGame->m_pSprite[ItemEquipPivotPoint + 51] = loader.get_sprite(6, false);
        m_pGame->m_pSprite[ItemEquipPivotPoint + 52] = loader.get_sprite(7, false);
        m_pGame->m_pSprite[ItemEquipPivotPoint + 53] = loader.get_sprite(8, false);
        m_pGame->m_pSprite[ItemEquipPivotPoint + 55] = loader.get_sprite(11, false);
        m_pGame->m_pSprite[ItemEquipPivotPoint + 57] = loader.get_sprite(12, false);
        m_pGame->m_pSprite[ItemEquipPivotPoint + 58] = loader.get_sprite(9, false);
        m_pGame->m_pSprite[ItemEquipPivotPoint + 59] = loader.get_sprite(10, false);
        m_pGame->m_pSprite[ItemEquipPivotPoint + 60] = loader.get_sprite(13, false);
        m_pGame->m_pSprite[ItemEquipPivotPoint + 61] = loader.get_sprite(14, false);
    });

    // Necks and angels for both genders
    hb::shared::sprite::SpriteLoader::open_pak("item-pack", [&](hb::shared::sprite::SpriteLoader& loader) {
        m_pGame->m_pSprite[ItemEquipPivotPoint + 16] = loader.get_sprite(15, false);
        m_pGame->m_pSprite[ItemEquipPivotPoint + 22] = loader.get_sprite(19, false);
        m_pGame->m_pSprite[ItemEquipPivotPoint + 56] = loader.get_sprite(15, false);
        m_pGame->m_pSprite[ItemEquipPivotPoint + 62] = loader.get_sprite(19, false);
    });

    // Player body sprites
    MakeSprite("Bm", 500 + 15 * 8 * 0, 96, true);  // Black Man
    MakeSprite("Wm", 500 + 15 * 8 * 1, 96, true);  // White Man
    MakeSprite("Ym", 500 + 15 * 8 * 2, 96, true);  // Yellow Man

    m_iLoadingStage = 20;
}

//=============================================================================
// Stage 20: Tutelary angels, female player bodies
//=============================================================================
void Screen_Loading::LoadStage_Angels()
{
    MakeSprite("TutelarAngel1", TutelaryAngelsPivotPoint + 50 * 0, 48, false);
    MakeSprite("TutelarAngel2", TutelaryAngelsPivotPoint + 50 * 1, 48, false);
    MakeSprite("TutelarAngel3", TutelaryAngelsPivotPoint + 50 * 2, 48, false);
    MakeSprite("TutelarAngel4", TutelaryAngelsPivotPoint + 50 * 3, 48, false);

    MakeSprite("Bw", 500 + 15 * 8 * 3, 96, true);  // Black Woman
    MakeSprite("Ww", 500 + 15 * 8 * 4, 96, true);  // White Woman
    MakeSprite("Yw", 500 + 15 * 8 * 5, 96, true);  // Yellow Woman

    m_iLoadingStage = 24;
}

//=============================================================================
// Stage 24: Monsters (Slime to William)
//=============================================================================
void Screen_Loading::LoadStage_Monsters1()
{
    MakeSprite("slm", Mob + 7 * 8 * 0, 40, true);
    MakeSprite("ske", Mob + 7 * 8 * 1, 40, true);
    MakeSprite("Gol", Mob + 7 * 8 * 2, 40, true);
    MakeSprite("Cyc", Mob + 7 * 8 * 3, 40, true);
    MakeSprite("Orc", Mob + 7 * 8 * 4, 40, true);
    MakeSprite("Shopkpr", Mob + 7 * 8 * 5, 8, true);
    MakeSprite("Ant", Mob + 7 * 8 * 6, 40, true);
    MakeSprite("Scp", Mob + 7 * 8 * 7, 40, true);
    MakeSprite("Zom", Mob + 7 * 8 * 8, 40, true);
    MakeSprite("Gandlf", Mob + 7 * 8 * 9, 8, true);
    MakeSprite("Howard", Mob + 7 * 8 * 10, 8, true);
    MakeSprite("Guard", Mob + 7 * 8 * 11, 40, true);
    MakeSprite("Amp", Mob + 7 * 8 * 12, 40, true);
    MakeSprite("Cla", Mob + 7 * 8 * 13, 40, true);
    MakeSprite("tom", Mob + 7 * 8 * 14, 8, true);
    MakeSprite("William", Mob + 7 * 8 * 15, 8, true);

    m_iLoadingStage = 28;
}

//=============================================================================
// Stage 28: Monsters (Kennedy to Energy Ball)
//=============================================================================
void Screen_Loading::LoadStage_Monsters2()
{
    MakeSprite("Kennedy", Mob + 7 * 8 * 16, 8, true);
    MakeSprite("Helb", Mob + 7 * 8 * 17, 40, true);
    MakeSprite("Troll", Mob + 7 * 8 * 18, 40, true);
    MakeSprite("Orge", Mob + 7 * 8 * 19, 40, true);
    MakeSprite("Liche", Mob + 7 * 8 * 20, 40, true);
    MakeSprite("Demon", Mob + 7 * 8 * 21, 40, true);
    MakeSprite("Unicorn", Mob + 7 * 8 * 22, 40, true);
    MakeSprite("WereWolf", Mob + 7 * 8 * 23, 40, true);
    MakeSprite("Dummy", Mob + 7 * 8 * 24, 40, true);

    // Energy Ball - all 40 slots use the same sprite
    hb::shared::sprite::SpriteLoader::open_pak("Effect5", [&](hb::shared::sprite::SpriteLoader& loader) {
        for (int i = 0; i < 40; i++)
            m_pGame->m_pSprite[Mob + i + 7 * 8 * 25] = loader.get_sprite(0, true);
    });

    m_iLoadingStage = 32;
}

//=============================================================================
// Stage 32: Guard towers, structures
//=============================================================================
void Screen_Loading::LoadStage_Monsters3()
{
    MakeSprite("GT-Arrow", Mob + 7 * 8 * 26, 40, true);
    MakeSprite("GT-Cannon", Mob + 7 * 8 * 27, 40, true);
    MakeSprite("ManaCollector", Mob + 7 * 8 * 28, 40, true);
    MakeSprite("Detector", Mob + 7 * 8 * 29, 40, true);
    MakeSprite("ESG", Mob + 7 * 8 * 30, 40, true);
    MakeSprite("GMG", Mob + 7 * 8 * 31, 40, true);
    MakeSprite("ManaStone", Mob + 7 * 8 * 32, 40, true);
    MakeSprite("LWB", Mob + 7 * 8 * 33, 40, true);
    MakeSprite("GHK", Mob + 7 * 8 * 34, 40, true);
    MakeSprite("GHKABS", Mob + 7 * 8 * 35, 40, true);
    MakeSprite("TK", Mob + 7 * 8 * 36, 40, true);
    MakeSprite("BG", Mob + 7 * 8 * 37, 40, true);

    m_iLoadingStage = 36;
}

//=============================================================================
// Stage 36: More monsters
//=============================================================================
void Screen_Loading::LoadStage_Monsters4()
{
    MakeSprite("Stalker", Mob + 7 * 8 * 38, 40, true);
    MakeSprite("Hellclaw", Mob + 7 * 8 * 39, 40, true);
    MakeSprite("Tigerworm", Mob + 7 * 8 * 40, 40, true);
    MakeSprite("Catapult", Mob + 7 * 8 * 41, 40, true);
    MakeSprite("Gagoyle", Mob + 7 * 8 * 42, 40, true);
    MakeSprite("Beholder", Mob + 7 * 8 * 43, 40, true);
    MakeSprite("DarkElf", Mob + 7 * 8 * 44, 40, true);
    MakeSprite("Bunny", Mob + 7 * 8 * 45, 40, true);
    MakeSprite("Cat", Mob + 7 * 8 * 46, 40, true);
    MakeSprite("GiantFrog", Mob + 7 * 8 * 47, 40, true);
    MakeSprite("MTGiant", Mob + 7 * 8 * 48, 40, true);

    m_iLoadingStage = 40;
}

//=============================================================================
// Stage 40: More monsters
//=============================================================================
void Screen_Loading::LoadStage_Monsters5()
{
    MakeSprite("Ettin", Mob + 7 * 8 * 49, 40, true);
    MakeSprite("CanPlant", Mob + 7 * 8 * 50, 40, true);
    MakeSprite("Rudolph", Mob + 7 * 8 * 51, 40, true);
    MakeSprite("DireBoar", Mob + 7 * 8 * 52, 40, true);
    MakeSprite("frost", Mob + 7 * 8 * 53, 40, true);
    MakeSprite("Crop", Mob + 7 * 8 * 54, 40, true);
    MakeSprite("IceGolem", Mob + 7 * 8 * 55, 40, true);
    MakeSprite("Wyvern", Mob + 7 * 8 * 56, 24, true);
    MakeSprite("McGaffin", Mob + 7 * 8 * 57, 16, true);
    MakeSprite("Perry", Mob + 7 * 8 * 58, 16, true);
    MakeSprite("Devlin", Mob + 7 * 8 * 59, 16, true);
    MakeSprite("Barlog", Mob + 7 * 8 * 60, 40, true);
    MakeSprite("Centaurus", Mob + 7 * 8 * 61, 40, true);
    MakeSprite("ClawTurtle", Mob + 7 * 8 * 62, 40, true);
    MakeSprite("FireWyvern", Mob + 7 * 8 * 63, 24, true);
    MakeSprite("GiantCrayfish", Mob + 7 * 8 * 64, 40, true);
    MakeSprite("GiantLizard", Mob + 7 * 8 * 65, 40, true);

    m_iLoadingStage = 44;
}

//=============================================================================
// Stage 44: New NPCs and monsters
//=============================================================================
void Screen_Loading::LoadStage_Monsters6()
{
    MakeSprite("GiantPlant", Mob + 7 * 8 * 66, 40, true);
    MakeSprite("MasterMageOrc", Mob + 7 * 8 * 67, 40, true);
    MakeSprite("Minotaurs", Mob + 7 * 8 * 68, 40, true);
    MakeSprite("Nizie", Mob + 7 * 8 * 69, 40, true);
    MakeSprite("Tentocle", Mob + 7 * 8 * 70, 40, true);
    MakeSprite("yspro", Mob + 7 * 8 * 71, 32, true);
    MakeSprite("Sorceress", Mob + 7 * 8 * 72, 40, true);
    MakeSprite("TPKnight", Mob + 7 * 8 * 73, 40, true);
    MakeSprite("ElfMaster", Mob + 7 * 8 * 74, 40, true);
    MakeSprite("DarkKnight", Mob + 7 * 8 * 75, 40, true);
    MakeSprite("HBTank", Mob + 7 * 8 * 76, 32, true);
    MakeSprite("CBTurret", Mob + 7 * 8 * 77, 32, true);
    MakeSprite("Babarian", Mob + 7 * 8 * 78, 40, true);
    MakeSprite("ACannon", Mob + 7 * 8 * 79, 32, true);

    m_iLoadingStage = 48;
}

//=============================================================================
// Stage 48: Male underwear, Gail, Heldenian Gate
//=============================================================================
void Screen_Loading::LoadStage_MaleUndies()
{
    MakeSprite("Gail", Mob + 7 * 8 * 80, 8, true);
    MakeSprite("Gate", Mob + 7 * 8 * 81, 24, true);

    // Male underwear
    hb::shared::sprite::SpriteLoader::open_pak("Mpt", [&](hb::shared::sprite::SpriteLoader& loader) {
        for (int g = 0; g < 8; g++) {
            for (int i = 0; i < 12; i++) {
                m_pGame->m_pSprite[UndiesM + i + 15 * g] = loader.get_sprite(i + 12 * g, true);
            }
        }
    });

    m_iLoadingStage = 52;
}

//=============================================================================
// Stage 52: Male hair, armor, shirts
//=============================================================================
void Screen_Loading::LoadStage_MaleArmor()
{
    // Male hair
    hb::shared::sprite::SpriteLoader::open_pak("Mhr", [&](hb::shared::sprite::SpriteLoader& loader) {
        for (int g = 0; g < 8; g++) {
            for (int i = 0; i < 12; i++) {
                m_pGame->m_pSprite[HairM + i + 15 * g] = loader.get_sprite(i + 12 * g, true);
            }
        }
    });

    // Male body armor
    MakeSprite("MLArmor", BodyArmorM + 15 * 1, 12, true);
    MakeSprite("MCMail", BodyArmorM + 15 * 2, 12, true);
    MakeSprite("MSMail", BodyArmorM + 15 * 3, 12, true);
    MakeSprite("MPMail", BodyArmorM + 15 * 4, 12, true);
    MakeSprite("Mtunic", BodyArmorM + 15 * 5, 12, true);
    MakeSprite("MRobe1", BodyArmorM + 15 * 6, 12, true);
    MakeSprite("MSanta", BodyArmorM + 15 * 7, 12, true);
    MakeSprite("MHRobe1", BodyArmorM + 15 * 10, 12, true);
    MakeSprite("MHRobe2", BodyArmorM + 15 * 11, 12, true);
    MakeSprite("MHPMail1", BodyArmorM + 15 * 8, 12, true);
    MakeSprite("MHPMail2", BodyArmorM + 15 * 9, 12, true);

    // Male shirts
    MakeSprite("MShirt", BerkM + 15 * 1, 12, true);
    MakeSprite("MHauberk", BerkM + 15 * 2, 12, true);
    MakeSprite("MHHauberk1", BerkM + 15 * 3, 12, true);
    MakeSprite("MHHauberk2", BerkM + 15 * 4, 12, true);

    m_iLoadingStage = 56;
}

//=============================================================================
// Stage 56: Male pants, shoes, swords
//=============================================================================
void Screen_Loading::LoadStage_MaleLegs()
{
    // Male leggings
    MakeSprite("MTrouser", LeggM + 15 * 1, 12, true);
    MakeSprite("MHTrouser", LeggM + 15 * 2, 12, true);
    MakeSprite("MCHoses", LeggM + 15 * 3, 12, true);
    MakeSprite("MLeggings", LeggM + 15 * 4, 12, true);
    MakeSprite("MHLeggings1", LeggM + 15 * 5, 12, true);
    MakeSprite("MHLeggings2", LeggM + 15 * 6, 12, true);

    // Male boots
    MakeSprite("MShoes", BootM + 15 * 1, 12, true);
    MakeSprite("MLBoots", BootM + 15 * 2, 12, true);

    // Male swords (batch load)
    hb::shared::sprite::SpriteLoader::open_pak("Msw", [&](hb::shared::sprite::SpriteLoader& loader) {
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[WeaponM + i + 64 * 1] = loader.get_sprite(i + 56 * 0, true);
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[WeaponM + i + 64 * 2] = loader.get_sprite(i + 56 * 1, true);
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[WeaponM + i + 64 * 3] = loader.get_sprite(i + 56 * 2, true);
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[WeaponM + i + 64 * 4] = loader.get_sprite(i + 56 * 3, true);
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[WeaponM + i + 64 * 6] = loader.get_sprite(i + 56 * 5, true);
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[WeaponM + i + 64 * 7] = loader.get_sprite(i + 56 * 6, true);
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[WeaponM + i + 64 * 8] = loader.get_sprite(i + 56 * 7, true);
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[WeaponM + i + 64 * 9] = loader.get_sprite(i + 56 * 8, true);
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[WeaponM + i + 64 * 10] = loader.get_sprite(i + 56 * 9, true);
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[WeaponM + i + 64 * 11] = loader.get_sprite(i + 56 * 10, true);
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[WeaponM + i + 64 * 12] = loader.get_sprite(i + 56 * 11, true);
    });

    m_iLoadingStage = 60;
}

//=============================================================================
// Stage 60: Male swords continued
//=============================================================================
void Screen_Loading::LoadStage_MaleSwords()
{
    MakeSprite("Mswx", WeaponM + 64 * 5, 56, true);
    MakeSprite("Msw2", WeaponM + 64 * 13, 56, true);
    MakeSprite("Msw3", WeaponM + 64 * 14, 56, true);
    MakeSprite("MStormBringer", WeaponM + 64 * 15, 56, true);
    MakeSprite("MDarkExec", WeaponM + 64 * 16, 56, true);
    MakeSprite("MKlonessBlade", WeaponM + 64 * 17, 56, true);
    MakeSprite("MKlonessAstock", WeaponM + 64 * 18, 56, true);
    MakeSprite("MDebastator", WeaponM + 64 * 19, 56, true);
    MakeSprite("MAxe1", WeaponM + 64 * 20, 56, true);
    MakeSprite("MAxe2", WeaponM + 64 * 21, 56, true);
    MakeSprite("MAxe3", WeaponM + 64 * 22, 56, true);
    MakeSprite("MAxe4", WeaponM + 64 * 23, 56, true);
    MakeSprite("MAxe5", WeaponM + 64 * 24, 56, true);
    MakeSprite("MPickAxe1", WeaponM + 64 * 25, 56, true);
    MakeSprite("MAxe6", WeaponM + 64 * 26, 56, true);
    MakeSprite("Mhoe", WeaponM + 64 * 27, 56, true);
    MakeSprite("MKlonessAxe", WeaponM + 64 * 28, 56, true);
    MakeSprite("MLightBlade", WeaponM + 64 * 29, 56, true);

    m_iLoadingStage = 64;
}

//=============================================================================
// Stage 64: Male hammers, staves, bows setup
//=============================================================================
void Screen_Loading::LoadStage_MaleWeapons()
{
    MakeSprite("MHammer", WeaponM + 64 * 30, 56, true);
    MakeSprite("MBHammer", WeaponM + 64 * 31, 56, true);
    MakeSprite("MBabHammer", WeaponM + 64 * 32, 56, true);
    MakeSprite("MBShadowSword", WeaponM + 64 * 33, 56, true);
    MakeSprite("MBerserkWand", WeaponM + 64 * 34, 56, true);
    MakeSprite("Mstaff1", WeaponM + 64 * 35, 56, true);
    MakeSprite("Mstaff2", WeaponM + 64 * 36, 56, true);
    MakeSprite("MStaff3", WeaponM + 64 * 37, 56, true);
    MakeSprite("MReMagicWand", WeaponM + 64 * 38, 56, true);
    MakeSprite("MKlonessWand", WeaponM + 64 * 39, 56, true);
    MakeSprite("MDirectBow", WeaponM + 64 * 42, 56, true);
    MakeSprite("MFireBow", WeaponM + 64 * 43, 56, true);

    m_iLoadingStage = 68;
}

//=============================================================================
// Stage 68: Male bows, shields
//=============================================================================
void Screen_Loading::LoadStage_MaleBows()
{
    // Male bows
    hb::shared::sprite::SpriteLoader::open_pak("Mbo", [&](hb::shared::sprite::SpriteLoader& loader) {
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[WeaponM + i + 64 * 40] = loader.get_sprite(i + 56 * 0, true);
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[WeaponM + i + 64 * 41] = loader.get_sprite(i + 56 * 1, true);
    });

    // Male shields
    hb::shared::sprite::SpriteLoader::open_pak("Msh", [&](hb::shared::sprite::SpriteLoader& loader) {
        for (int g = 0; g < 9; g++) {
            for (int i = 0; i < 7; i++) {
                m_pGame->m_pSprite[ShieldM + i + 8 * (g + 1)] = loader.get_sprite(i + 7 * g, true);
            }
        }
    });

    m_iLoadingStage = 72;
}

//=============================================================================
// Stage 72: Male mantles, helms
//=============================================================================
void Screen_Loading::LoadStage_FemaleBase()
{
    // Male mantles
    MakeSprite("Mmantle01", MantleM + 15 * 1, 12, true);
    MakeSprite("Mmantle02", MantleM + 15 * 2, 12, true);
    MakeSprite("Mmantle03", MantleM + 15 * 3, 12, true);
    MakeSprite("Mmantle04", MantleM + 15 * 4, 12, true);
    MakeSprite("Mmantle05", MantleM + 15 * 5, 12, true);
    MakeSprite("Mmantle06", MantleM + 15 * 6, 12, true);

    // Male helms
    MakeSprite("MHelm1", HeadM + 15 * 1, 12, true);
    MakeSprite("MHelm2", HeadM + 15 * 2, 12, true);
    MakeSprite("MHelm3", HeadM + 15 * 3, 12, true);
    MakeSprite("MHelm4", HeadM + 15 * 4, 12, true);
    MakeSprite("MHCap1", HeadM + 15 * 11, 12, true);
    MakeSprite("MHCap2", HeadM + 15 * 12, 12, true);
    MakeSprite("MHHelm1", HeadM + 15 * 9, 12, true);
    MakeSprite("MHHelm2", HeadM + 15 * 10, 12, true);
    MakeSprite("NMHelm1", HeadM + 15 * 5, 12, true);
    MakeSprite("NMHelm2", HeadM + 15 * 6, 12, true);
    MakeSprite("NMHelm3", HeadM + 15 * 7, 12, true);
    MakeSprite("NMHelm4", HeadM + 15 * 8, 12, true);

    m_iLoadingStage = 76;
}

//=============================================================================
// Stage 76: Female underwear and hair
//=============================================================================
void Screen_Loading::LoadStage_FemaleArmor()
{
    // Female underwear
    hb::shared::sprite::SpriteLoader::open_pak("Wpt", [&](hb::shared::sprite::SpriteLoader& loader) {
        for (int g = 0; g < 8; g++) {
            for (int i = 0; i < 12; i++) {
                m_pGame->m_pSprite[UndiesW + i + 15 * g] = loader.get_sprite(i + 12 * g, true);
            }
        }
    });

    // Female hair
    hb::shared::sprite::SpriteLoader::open_pak("Whr", [&](hb::shared::sprite::SpriteLoader& loader) {
        for (int g = 0; g < 8; g++) {
            for (int i = 0; i < 12; i++) {
                m_pGame->m_pSprite[HairW + i + 15 * g] = loader.get_sprite(i + 12 * g, true);
            }
        }
    });

    m_iLoadingStage = 80;
}

//=============================================================================
// Stage 80: Female armor, shirts, legs, shoes
//=============================================================================
void Screen_Loading::LoadStage_FemaleLegs()
{
    // Female body armor
    MakeSprite("WBodice1", BodyArmorW + 15 * 1, 12, true);
    MakeSprite("WBodice2", BodyArmorW + 15 * 2, 12, true);
    MakeSprite("WLArmor", BodyArmorW + 15 * 3, 12, true);
    MakeSprite("WCMail", BodyArmorW + 15 * 4, 12, true);
    MakeSprite("WSMail", BodyArmorW + 15 * 5, 12, true);
    MakeSprite("WPMail", BodyArmorW + 15 * 6, 12, true);
    MakeSprite("WRobe1", BodyArmorW + 15 * 7, 12, true);
    MakeSprite("WSanta", BodyArmorW + 15 * 8, 12, true);
    MakeSprite("WHRobe1", BodyArmorW + 15 * 11, 12, true);
    MakeSprite("WHRobe2", BodyArmorW + 15 * 12, 12, true);
    MakeSprite("WHPMail1", BodyArmorW + 15 * 9, 12, true);
    MakeSprite("WHPMail2", BodyArmorW + 15 * 10, 12, true);

    // Female shirts
    MakeSprite("WChemiss", BerkW + 15 * 1, 12, true);
    MakeSprite("WShirt", BerkW + 15 * 2, 12, true);
    MakeSprite("WHauberk", BerkW + 15 * 3, 12, true);
    MakeSprite("WHHauberk1", BerkW + 15 * 4, 12, true);
    MakeSprite("WHHauberk2", BerkW + 15 * 5, 12, true);

    // Female leggings
    MakeSprite("WSkirt", LeggW + 15 * 1, 12, true);
    MakeSprite("WTrouser", LeggW + 15 * 2, 12, true);
    MakeSprite("WHTrouser", LeggW + 15 * 3, 12, true);
    MakeSprite("WHLeggings1", LeggW + 15 * 6, 12, true);
    MakeSprite("WHLeggings2", LeggW + 15 * 7, 12, true);
    MakeSprite("WCHoses", LeggW + 15 * 4, 12, true);
    MakeSprite("WLeggings", LeggW + 15 * 5, 12, true);

    // Female boots
    MakeSprite("WShoes", BootW + 15 * 1, 12, true);
    MakeSprite("WLBoots", BootW + 15 * 2, 12, true);

    m_iLoadingStage = 84;
}

//=============================================================================
// Stage 84: Female swords
//=============================================================================
void Screen_Loading::LoadStage_FemaleSwords()
{
    // Female swords (batch load)
    hb::shared::sprite::SpriteLoader::open_pak("Wsw", [&](hb::shared::sprite::SpriteLoader& loader) {
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[WeaponW + i + 64 * 1] = loader.get_sprite(i + 56 * 0, true);
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[WeaponW + i + 64 * 2] = loader.get_sprite(i + 56 * 1, true);
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[WeaponW + i + 64 * 3] = loader.get_sprite(i + 56 * 2, true);
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[WeaponW + i + 64 * 4] = loader.get_sprite(i + 56 * 3, true);
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[WeaponW + i + 64 * 6] = loader.get_sprite(i + 56 * 5, true);
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[WeaponW + i + 64 * 7] = loader.get_sprite(i + 56 * 6, true);
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[WeaponW + i + 64 * 8] = loader.get_sprite(i + 56 * 7, true);
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[WeaponW + i + 64 * 9] = loader.get_sprite(i + 56 * 8, true);
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[WeaponW + i + 64 * 10] = loader.get_sprite(i + 56 * 9, true);
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[WeaponW + i + 64 * 11] = loader.get_sprite(i + 56 * 10, true);
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[WeaponW + i + 64 * 12] = loader.get_sprite(i + 56 * 11, true);
    });

    MakeSprite("Wswx", WeaponW + 64 * 5, 56, true);
    MakeSprite("Wsw2", WeaponW + 64 * 13, 56, true);
    MakeSprite("Wsw3", WeaponW + 64 * 14, 56, true);
    MakeSprite("WStormBringer", WeaponW + 64 * 15, 56, true);
    MakeSprite("WDarkExec", WeaponW + 64 * 16, 56, true);
    MakeSprite("WKlonessBlade", WeaponW + 64 * 17, 56, true);
    MakeSprite("WKlonessAstock", WeaponW + 64 * 18, 56, true);
    MakeSprite("WDebastator", WeaponW + 64 * 19, 56, true);

    m_iLoadingStage = 88;
}

//=============================================================================
// Stage 88: Female axes, staves
//=============================================================================
void Screen_Loading::LoadStage_FemaleWeapons()
{
    MakeSprite("WAxe1", WeaponW + 64 * 20, 56, true);
    MakeSprite("WAxe2", WeaponW + 64 * 21, 56, true);
    MakeSprite("WAxe3", WeaponW + 64 * 22, 56, true);
    MakeSprite("WAxe4", WeaponW + 64 * 23, 56, true);
    MakeSprite("WAxe5", WeaponW + 64 * 24, 56, true);
    MakeSprite("WpickAxe1", WeaponW + 64 * 25, 56, true);
    MakeSprite("WAxe6", WeaponW + 64 * 26, 56, true);
    MakeSprite("Whoe", WeaponW + 64 * 27, 56, true);
    MakeSprite("WKlonessAxe", WeaponW + 64 * 28, 56, true);
    MakeSprite("WLightBlade", WeaponW + 64 * 29, 56, true);
    MakeSprite("WHammer", WeaponW + 64 * 30, 56, true);
    MakeSprite("WBHammer", WeaponW + 64 * 31, 56, true);
    MakeSprite("WBabHammer", WeaponW + 64 * 32, 56, true);
    MakeSprite("WBShadowSword", WeaponW + 64 * 33, 56, true);
    MakeSprite("WBerserkWand", WeaponW + 64 * 34, 56, true);
    MakeSprite("Wstaff1", WeaponW + 64 * 35, 56, true);
    MakeSprite("Wstaff2", WeaponW + 64 * 36, 56, true);
    MakeSprite("WStaff3", WeaponW + 64 * 37, 56, true);
    MakeSprite("WKlonessWand", WeaponW + 64 * 39, 56, true);
    MakeSprite("WReMagicWand", WeaponW + 64 * 38, 56, true);
    MakeSprite("WDirectBow", WeaponW + 64 * 42, 56, true);
    MakeSprite("WFireBow", WeaponW + 64 * 43, 56, true);

    m_iLoadingStage = 92;
}

//=============================================================================
// Stage 92: Female mantles, helms
//=============================================================================
void Screen_Loading::LoadStage_FemaleMantles()
{
    // Female mantles
    MakeSprite("Wmantle01", MantleW + 15 * 1, 12, true);
    MakeSprite("Wmantle02", MantleW + 15 * 2, 12, true);
    MakeSprite("Wmantle03", MantleW + 15 * 3, 12, true);
    MakeSprite("Wmantle04", MantleW + 15 * 4, 12, true);
    MakeSprite("Wmantle05", MantleW + 15 * 5, 12, true);
    MakeSprite("Wmantle06", MantleW + 15 * 6, 12, true);

    // Female helms
    MakeSprite("WHelm1", HeadW + 15 * 1, 12, true);
    MakeSprite("WHelm4", HeadW + 15 * 4, 12, true);
    MakeSprite("WHHelm1", HeadW + 15 * 9, 12, true);
    MakeSprite("WHHelm2", HeadW + 15 * 10, 12, true);
    MakeSprite("WHCap1", HeadW + 15 * 11, 12, true);
    MakeSprite("WHCap2", HeadW + 15 * 12, 12, true);
    MakeSprite("NWHelm1", HeadW + 15 * 5, 12, true);
    MakeSprite("NWHelm2", HeadW + 15 * 6, 12, true);
    MakeSprite("NWHelm3", HeadW + 15 * 7, 12, true);
    MakeSprite("NWHelm4", HeadW + 15 * 8, 12, true);

    m_iLoadingStage = 96;
}

//=============================================================================
// Stage 96: Female bows, shields
//=============================================================================
void Screen_Loading::LoadStage_FemaleBows()
{
    // Female bows
    hb::shared::sprite::SpriteLoader::open_pak("Wbo", [&](hb::shared::sprite::SpriteLoader& loader) {
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[WeaponW + i + 64 * 40] = loader.get_sprite(i + 56 * 0, true);
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[WeaponW + i + 64 * 41] = loader.get_sprite(i + 56 * 1, true);
    });

    // Female shields
    hb::shared::sprite::SpriteLoader::open_pak("Wsh", [&](hb::shared::sprite::SpriteLoader& loader) {
        for (int g = 0; g < 9; g++) {
            for (int i = 0; i < 7; i++) {
                m_pGame->m_pSprite[ShieldW + i + 8 * (g + 1)] = loader.get_sprite(i + 7 * g, true);
            }
        }
    });

    m_iLoadingStage = 100;
}

//=============================================================================
// Stage 100: Effects, sounds, finish loading
//=============================================================================
void Screen_Loading::LoadStage_Effects()
{
    MakeEffectSpr("effect", 0, 10, false);
    MakeEffectSpr("effect2", 10, 3, false);
    MakeEffectSpr("effect3", 13, 6, false);
    MakeEffectSpr("effect4", 19, 5, false);

    // Effect5 batch load
    hb::shared::sprite::SpriteLoader::open_pak("effect5", [&](hb::shared::sprite::SpriteLoader& loader) {
        for (int i = 0; i <= 6; i++) {
            m_pGame->m_pEffectSpr[i + 24] = loader.get_sprite(i + 1, false);
        }
    });

    MakeEffectSpr("CruEffect1", 31, 9, false);
    MakeEffectSpr("effect6", 40, 5, false);
    MakeEffectSpr("effect7", 45, 12, false);
    MakeEffectSpr("effect8", 57, 9, false);
    MakeEffectSpr("effect9", 66, 21, false);
    MakeEffectSpr("effect10", 87, 2, false);
    MakeEffectSpr("effect11", 89, 14, false);
    MakeEffectSpr("effect11s", 104, 1, false);
    MakeEffectSpr("yseffect2", 140, 8, false);
    MakeEffectSpr("effect12", 148, 4, false);
    MakeEffectSpr("yseffect3", 152, 16, false);
    MakeEffectSpr("yseffect4", 133, 7, false);

    // Initialize EffectManager with loaded sprites
    m_pGame->m_pEffectManager->SetEffectSprites(m_pGame->m_pEffectSpr);
    WeatherManager::Get().SetDependencies(*m_pGame->m_Renderer, m_pGame->m_pEffectSpr, m_pGame->m_Camera);
    WeatherManager::Get().SetMapData(m_pGame->m_pMapData.get());

    // Load all sound effects
    AudioManager::Get().LoadSounds();

    // Loading complete - transition to main menu
    m_pGame->ChangeGameMode(GameMode::MainMenu);
}

//=============================================================================
// Sprite loading helpers
//=============================================================================
void Screen_Loading::MakeSprite(const char* FileName, short sStart, short sCount, bool bAlphaEffect)
{
    try {
        hb::shared::sprite::SpriteLoader::open_pak(FileName, [&](hb::shared::sprite::SpriteLoader& loader) {
            size_t totalInPak = loader.get_sprite_count();
            size_t toLoad = static_cast<size_t>(sCount);
            if (toLoad > totalInPak) toLoad = totalInPak;

            for (size_t i = 0; i < toLoad; i++) {
                m_pGame->m_pSprite[i + sStart] = loader.get_sprite(i, bAlphaEffect);
            }
        });
    } catch (const std::exception& e) {
        printf("[MakeSprite] FAILED %s: %s\n", FileName, e.what());
    } catch (...) {
        printf("[MakeSprite] FAILED %s: unknown exception\n", FileName);
    }
}

void Screen_Loading::MakeTileSpr(const char* FileName, short sStart, short sCount, bool bAlphaEffect)
{
    try {
        hb::shared::sprite::SpriteLoader::open_pak(FileName, [&](hb::shared::sprite::SpriteLoader& loader) {
            size_t totalInPak = loader.get_sprite_count();
            size_t toLoad = static_cast<size_t>(sCount);
            if (toLoad > totalInPak) toLoad = totalInPak;

            for (size_t i = 0; i < toLoad; i++) {
                m_pGame->m_pTileSpr[i + sStart] = loader.get_sprite(i, bAlphaEffect);
            }
        });
    } catch (const std::exception& e) {
        printf("[MakeTileSpr] FAILED %s: %s\n", FileName, e.what());
    }
}

void Screen_Loading::MakeEffectSpr(const char* FileName, short sStart, short sCount, bool bAlphaEffect)
{
    try {
        hb::shared::sprite::SpriteLoader::open_pak(FileName, [&](hb::shared::sprite::SpriteLoader& loader) {
            size_t totalInPak = loader.get_sprite_count();
            size_t toLoad = static_cast<size_t>(sCount);
            if (toLoad > totalInPak) toLoad = totalInPak;

            for (size_t i = 0; i < toLoad; i++) {
                m_pGame->m_pEffectSpr[i + sStart] = loader.get_sprite(i, bAlphaEffect);
            }
        });
    } catch (const std::exception& e) {
        printf("[MakeEffectSpr] FAILED %s: %s\n", FileName, e.what());
    }
}

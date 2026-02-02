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
#include "SpriteLoader.h"

Screen_Loading::Screen_Loading(CGame* pGame)
    : IGameScreen(pGame)
{
}

void Screen_Loading::on_initialize()
{
    GameModeManager::SetCurrentMode(GameMode::Loading);
    m_iLoadingStage = 0;

    // Pre-load the loading screen sprite so it can render immediately
    m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_LOADING] = SpriteLib::Sprites::Create("New-Dialog", 0, false);
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
    DrawNewDialogBox(DEF_SPRID_INTERFACE_ND_LOADING, 0 + MENUX(), 0 + MENUY(), 0, true);
    DrawVersion();

    // Draw progress bar - width corresponds to loading stage (0-100)
    int iBarWidth = m_iLoadingStage;
    m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_LOADING]->DrawWidth(472 + MENUX(), 442 + MENUY(), 1, iBarWidth);
}

//=============================================================================
// Stage 0: Interface sprites, dialog boxes, maps
//=============================================================================
void Screen_Loading::LoadStage_Interface()
{
    // Load interface sprites
    SpriteLib::SpriteLoader::open_pak("interface", [&](SpriteLib::SpriteLoader& loader) {
        m_pGame->m_pSprite[DEF_SPRID_MOUSECURSOR] = loader.get_sprite(0, false);
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_SPRFONTS] = loader.get_sprite(1, false);
    });

    SpriteLib::SpriteLoader::open_pak("newmaps", [&](SpriteLib::SpriteLoader& loader) {
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_NEWMAPS1] = loader.get_sprite(0, false);
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_NEWMAPS2] = loader.get_sprite(1, false);
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_NEWMAPS3] = loader.get_sprite(2, false);
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_NEWMAPS4] = loader.get_sprite(3, false);
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_NEWMAPS5] = loader.get_sprite(4, false);
    });

    m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_LOGIN] = SpriteLib::Sprites::Create("LoginDialog", 0, false);

    SpriteLib::SpriteLoader::open_pak("New-Dialog", [&](SpriteLib::SpriteLoader& loader) {
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_MAINMENU] = loader.get_sprite(1, false);
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_QUIT] = loader.get_sprite(2, false);
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_NEWACCOUNT] = loader.get_sprite(2, false);
    });

    SpriteLib::SpriteLoader::open_pak("GameDialog", [&](SpriteLib::SpriteLoader& loader) {
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_GAME1] = loader.get_sprite(0, false);
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_GAME2] = loader.get_sprite(1, false);
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_GAME3] = loader.get_sprite(2, false);
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_GAME4] = loader.get_sprite(3, false);
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_CRUSADE] = loader.get_sprite(4, false);
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_ICONPANNEL] = loader.get_sprite(6, false);
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_INVENTORY] = loader.get_sprite(7, false);
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_SELECTCHAR] = loader.get_sprite(8, false);
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_NEWCHAR] = loader.get_sprite(9, false);
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_NEWEXCHANGE] = loader.get_sprite(10, false);
    });

    m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_PARTYSTATUS] = SpriteLib::Sprites::Create("PartySprite", 0, false);

    SpriteLib::SpriteLoader::open_pak("DialogText", [&](SpriteLib::SpriteLoader& loader) {
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_TEXT] = loader.get_sprite(0, false);
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ND_BUTTON] = loader.get_sprite(1, false);
    });

    MakeSprite("Telescope", DEF_SPRID_INTERFACE_GUIDEMAP, 32, false);
    MakeSprite("Telescope2", DEF_SPRID_INTERFACE_GUIDEMAP + 35, 4, false);
    MakeSprite("monster", DEF_SPRID_INTERFACE_MONSTER, 1, false);

    // Load interface2 sprites in one batch
    SpriteLib::SpriteLoader::open_pak("interface2", [&](SpriteLib::SpriteLoader& loader) {
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ADDINTERFACE] = loader.get_sprite(0, false);
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_SPRFONTS2] = loader.get_sprite(1, false);
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_F1HELPWINDOWS] = loader.get_sprite(2, false);
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_CRAFTING] = loader.get_sprite(3, false);
    });

    // Load sprfonts sprites in one batch
    SpriteLib::SpriteLoader::open_pak("sprfonts", [&](SpriteLib::SpriteLoader& loader) {
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_FONT1] = loader.get_sprite(0, false);
        m_pGame->m_pSprite[DEF_SPRID_INTERFACE_FONT2] = loader.get_sprite(1, false);
    });

    // Create and register bitmap fonts with TextLib
    // Font 1: Characters '!' (33) to 'z' (122)
    if (m_pGame->m_pSprite[DEF_SPRID_INTERFACE_FONT1])
    {
        TextLib::LoadBitmapFont(GameFont::Bitmap1, m_pGame->m_pSprite[DEF_SPRID_INTERFACE_FONT1].get(),
            '!', 'z', 0, GameFont::GetFontSpacing(GameFont::Bitmap1));
    }

    // Font 2: Characters ' ' (32) to '~' (126), uses dynamic spacing from sprite frames
    if (m_pGame->m_pSprite[DEF_SPRID_INTERFACE_FONT2])
    {
        TextLib::LoadBitmapFontDynamic(GameFont::Bitmap2, m_pGame->m_pSprite[DEF_SPRID_INTERFACE_FONT2].get(), ' ', '~', 0);
    }

    // Number font: Digits '0' to '9', frame offset 6 in ADDINTERFACE sprite
    if (m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ADDINTERFACE])
    {
        TextLib::LoadBitmapFont(GameFont::Numbers, m_pGame->m_pSprite[DEF_SPRID_INTERFACE_ADDINTERFACE].get(),
            '0', '9', 6, GameFont::GetFontSpacing(GameFont::Numbers));
    }

    // SPRFONTS2: Characters ' ' (32) to '~' (126), with 3 different sizes (types 0, 1, 2)
    // Each type has 95 frames offset
    if (m_pGame->m_pSprite[DEF_SPRID_INTERFACE_SPRFONTS2])
    {
        TextLib::LoadBitmapFontDynamic(GameFont::SprFont3_0, m_pGame->m_pSprite[DEF_SPRID_INTERFACE_SPRFONTS2].get(), ' ', '~', 0);
        TextLib::LoadBitmapFontDynamic(GameFont::SprFont3_1, m_pGame->m_pSprite[DEF_SPRID_INTERFACE_SPRFONTS2].get(), ' ', '~', 95);
        TextLib::LoadBitmapFontDynamic(GameFont::SprFont3_2, m_pGame->m_pSprite[DEF_SPRID_INTERFACE_SPRFONTS2].get(), ' ', '~', 190);
    }

    m_iLoadingStage = 4;
}

//=============================================================================
// Stage 4: Map tiles, structures, trees
//=============================================================================
void Screen_Loading::LoadStage_Tiles1()
{
    MakeTileSpr("maptiles1", 0, 32, true);
    m_pGame->m_pTileSpr[1 + 50] = SpriteLib::Sprites::Create("structures1", 1, true);
    m_pGame->m_pTileSpr[5 + 50] = SpriteLib::Sprites::Create("structures1", 5, true);
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
    SpriteLib::SpriteLoader::open_pak("item-pack", [&](SpriteLib::SpriteLoader& loader) {
        for (size_t i = 0; i < 27 && i < loader.get_sprite_count(); i++) {
            m_pGame->m_pSprite[DEF_SPRID_ITEMPACK_PIVOTPOINT + 1 + i] = loader.get_sprite(i, false);
        }
        m_pGame->m_pSprite[DEF_SPRID_ITEMPACK_PIVOTPOINT + 20] = loader.get_sprite(17, false);
        m_pGame->m_pSprite[DEF_SPRID_ITEMPACK_PIVOTPOINT + 21] = loader.get_sprite(18, false);
        m_pGame->m_pSprite[DEF_SPRID_ITEMPACK_PIVOTPOINT + 22] = loader.get_sprite(19, false);
    });

    // Item ground sprites
    SpriteLib::SpriteLoader::open_pak("item-ground", [&](SpriteLib::SpriteLoader& loader) {
        for (size_t i = 0; i < 19 && i < loader.get_sprite_count(); i++) {
            m_pGame->m_pSprite[DEF_SPRID_ITEMGROUND_PIVOTPOINT + 1 + i] = loader.get_sprite(i, false);
        }
        m_pGame->m_pSprite[DEF_SPRID_ITEMGROUND_PIVOTPOINT + 20] = loader.get_sprite(17, false);
        m_pGame->m_pSprite[DEF_SPRID_ITEMGROUND_PIVOTPOINT + 21] = loader.get_sprite(18, false);
        m_pGame->m_pSprite[DEF_SPRID_ITEMGROUND_PIVOTPOINT + 22] = loader.get_sprite(19, false);
    });

    MakeSprite("item-dynamic", DEF_SPRID_ITEMDYNAMIC_PIVOTPOINT, 3, false);

    m_iLoadingStage = 16;
}

//=============================================================================
// Stage 16: Male/Female equipment base, player bodies
//=============================================================================
void Screen_Loading::LoadStage_Equipment1()
{
    // Male equipment
    SpriteLib::SpriteLoader::open_pak("item-equipM", [&](SpriteLib::SpriteLoader& loader) {
        m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 0] = loader.get_sprite(0, false);
        m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 1] = loader.get_sprite(1, false);
        m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 2] = loader.get_sprite(2, false);
        m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 3] = loader.get_sprite(3, false);
        m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 4] = loader.get_sprite(4, false);
        m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 5] = loader.get_sprite(5, false);
        m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 7] = loader.get_sprite(6, false);
        m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 8] = loader.get_sprite(7, false);
        m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 9] = loader.get_sprite(8, false);
        m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 15] = loader.get_sprite(11, false);
        m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 17] = loader.get_sprite(12, false);
        m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 18] = loader.get_sprite(9, false);
        m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 19] = loader.get_sprite(10, false);
        m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 20] = loader.get_sprite(13, false);
        m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 21] = loader.get_sprite(14, false);
    });

    // Female equipment
    SpriteLib::SpriteLoader::open_pak("item-equipW", [&](SpriteLib::SpriteLoader& loader) {
        m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 40] = loader.get_sprite(0, false);
        m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 41] = loader.get_sprite(1, false);
        m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 42] = loader.get_sprite(2, false);
        m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 43] = loader.get_sprite(3, false);
        m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 45] = loader.get_sprite(4, false);
        m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 50] = loader.get_sprite(5, false);
        m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 51] = loader.get_sprite(6, false);
        m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 52] = loader.get_sprite(7, false);
        m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 53] = loader.get_sprite(8, false);
        m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 55] = loader.get_sprite(11, false);
        m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 57] = loader.get_sprite(12, false);
        m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 58] = loader.get_sprite(9, false);
        m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 59] = loader.get_sprite(10, false);
        m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 60] = loader.get_sprite(13, false);
        m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 61] = loader.get_sprite(14, false);
    });

    // Necks and angels for both genders
    SpriteLib::SpriteLoader::open_pak("item-pack", [&](SpriteLib::SpriteLoader& loader) {
        m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 16] = loader.get_sprite(15, false);
        m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 22] = loader.get_sprite(19, false);
        m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 56] = loader.get_sprite(15, false);
        m_pGame->m_pSprite[DEF_SPRID_ITEMEQUIP_PIVOTPOINT + 62] = loader.get_sprite(19, false);
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
    MakeSprite("TutelarAngel1", DEF_SPRID_TUTELARYANGELS_PIVOTPOINT + 50 * 0, 48, false);
    MakeSprite("TutelarAngel2", DEF_SPRID_TUTELARYANGELS_PIVOTPOINT + 50 * 1, 48, false);
    MakeSprite("TutelarAngel3", DEF_SPRID_TUTELARYANGELS_PIVOTPOINT + 50 * 2, 48, false);
    MakeSprite("TutelarAngel4", DEF_SPRID_TUTELARYANGELS_PIVOTPOINT + 50 * 3, 48, false);

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
    MakeSprite("slm", DEF_SPRID_MOB + 7 * 8 * 0, 40, true);
    MakeSprite("ske", DEF_SPRID_MOB + 7 * 8 * 1, 40, true);
    MakeSprite("Gol", DEF_SPRID_MOB + 7 * 8 * 2, 40, true);
    MakeSprite("Cyc", DEF_SPRID_MOB + 7 * 8 * 3, 40, true);
    MakeSprite("Orc", DEF_SPRID_MOB + 7 * 8 * 4, 40, true);
    MakeSprite("Shopkpr", DEF_SPRID_MOB + 7 * 8 * 5, 8, true);
    MakeSprite("Ant", DEF_SPRID_MOB + 7 * 8 * 6, 40, true);
    MakeSprite("Scp", DEF_SPRID_MOB + 7 * 8 * 7, 40, true);
    MakeSprite("Zom", DEF_SPRID_MOB + 7 * 8 * 8, 40, true);
    MakeSprite("Gandlf", DEF_SPRID_MOB + 7 * 8 * 9, 8, true);
    MakeSprite("Howard", DEF_SPRID_MOB + 7 * 8 * 10, 8, true);
    MakeSprite("Guard", DEF_SPRID_MOB + 7 * 8 * 11, 40, true);
    MakeSprite("Amp", DEF_SPRID_MOB + 7 * 8 * 12, 40, true);
    MakeSprite("Cla", DEF_SPRID_MOB + 7 * 8 * 13, 40, true);
    MakeSprite("tom", DEF_SPRID_MOB + 7 * 8 * 14, 8, true);
    MakeSprite("William", DEF_SPRID_MOB + 7 * 8 * 15, 8, true);

    m_iLoadingStage = 28;
}

//=============================================================================
// Stage 28: Monsters (Kennedy to Energy Ball)
//=============================================================================
void Screen_Loading::LoadStage_Monsters2()
{
    MakeSprite("Kennedy", DEF_SPRID_MOB + 7 * 8 * 16, 8, true);
    MakeSprite("Helb", DEF_SPRID_MOB + 7 * 8 * 17, 40, true);
    MakeSprite("Troll", DEF_SPRID_MOB + 7 * 8 * 18, 40, true);
    MakeSprite("Orge", DEF_SPRID_MOB + 7 * 8 * 19, 40, true);
    MakeSprite("Liche", DEF_SPRID_MOB + 7 * 8 * 20, 40, true);
    MakeSprite("Demon", DEF_SPRID_MOB + 7 * 8 * 21, 40, true);
    MakeSprite("Unicorn", DEF_SPRID_MOB + 7 * 8 * 22, 40, true);
    MakeSprite("WereWolf", DEF_SPRID_MOB + 7 * 8 * 23, 40, true);
    MakeSprite("Dummy", DEF_SPRID_MOB + 7 * 8 * 24, 40, true);

    // Energy Ball - all 40 slots use the same sprite
    SpriteLib::SpriteLoader::open_pak("Effect5", [&](SpriteLib::SpriteLoader& loader) {
        for (int i = 0; i < 40; i++)
            m_pGame->m_pSprite[DEF_SPRID_MOB + i + 7 * 8 * 25] = loader.get_sprite(0, true);
    });

    m_iLoadingStage = 32;
}

//=============================================================================
// Stage 32: Guard towers, structures
//=============================================================================
void Screen_Loading::LoadStage_Monsters3()
{
    MakeSprite("GT-Arrow", DEF_SPRID_MOB + 7 * 8 * 26, 40, true);
    MakeSprite("GT-Cannon", DEF_SPRID_MOB + 7 * 8 * 27, 40, true);
    MakeSprite("ManaCollector", DEF_SPRID_MOB + 7 * 8 * 28, 40, true);
    MakeSprite("Detector", DEF_SPRID_MOB + 7 * 8 * 29, 40, true);
    MakeSprite("ESG", DEF_SPRID_MOB + 7 * 8 * 30, 40, true);
    MakeSprite("GMG", DEF_SPRID_MOB + 7 * 8 * 31, 40, true);
    MakeSprite("ManaStone", DEF_SPRID_MOB + 7 * 8 * 32, 40, true);
    MakeSprite("LWB", DEF_SPRID_MOB + 7 * 8 * 33, 40, true);
    MakeSprite("GHK", DEF_SPRID_MOB + 7 * 8 * 34, 40, true);
    MakeSprite("GHKABS", DEF_SPRID_MOB + 7 * 8 * 35, 40, true);
    MakeSprite("TK", DEF_SPRID_MOB + 7 * 8 * 36, 40, true);
    MakeSprite("BG", DEF_SPRID_MOB + 7 * 8 * 37, 40, true);

    m_iLoadingStage = 36;
}

//=============================================================================
// Stage 36: More monsters
//=============================================================================
void Screen_Loading::LoadStage_Monsters4()
{
    MakeSprite("Stalker", DEF_SPRID_MOB + 7 * 8 * 38, 40, true);
    MakeSprite("Hellclaw", DEF_SPRID_MOB + 7 * 8 * 39, 40, true);
    MakeSprite("Tigerworm", DEF_SPRID_MOB + 7 * 8 * 40, 40, true);
    MakeSprite("Catapult", DEF_SPRID_MOB + 7 * 8 * 41, 40, true);
    MakeSprite("Gagoyle", DEF_SPRID_MOB + 7 * 8 * 42, 40, true);
    MakeSprite("Beholder", DEF_SPRID_MOB + 7 * 8 * 43, 40, true);
    MakeSprite("DarkElf", DEF_SPRID_MOB + 7 * 8 * 44, 40, true);
    MakeSprite("Bunny", DEF_SPRID_MOB + 7 * 8 * 45, 40, true);
    MakeSprite("Cat", DEF_SPRID_MOB + 7 * 8 * 46, 40, true);
    MakeSprite("GiantFrog", DEF_SPRID_MOB + 7 * 8 * 47, 40, true);
    MakeSprite("MTGiant", DEF_SPRID_MOB + 7 * 8 * 48, 40, true);

    m_iLoadingStage = 40;
}

//=============================================================================
// Stage 40: More monsters
//=============================================================================
void Screen_Loading::LoadStage_Monsters5()
{
    MakeSprite("Ettin", DEF_SPRID_MOB + 7 * 8 * 49, 40, true);
    MakeSprite("CanPlant", DEF_SPRID_MOB + 7 * 8 * 50, 40, true);
    MakeSprite("Rudolph", DEF_SPRID_MOB + 7 * 8 * 51, 40, true);
    MakeSprite("DireBoar", DEF_SPRID_MOB + 7 * 8 * 52, 40, true);
    MakeSprite("frost", DEF_SPRID_MOB + 7 * 8 * 53, 40, true);
    MakeSprite("Crop", DEF_SPRID_MOB + 7 * 8 * 54, 40, true);
    MakeSprite("IceGolem", DEF_SPRID_MOB + 7 * 8 * 55, 40, true);
    MakeSprite("Wyvern", DEF_SPRID_MOB + 7 * 8 * 56, 24, true);
    MakeSprite("McGaffin", DEF_SPRID_MOB + 7 * 8 * 57, 16, true);
    MakeSprite("Perry", DEF_SPRID_MOB + 7 * 8 * 58, 16, true);
    MakeSprite("Devlin", DEF_SPRID_MOB + 7 * 8 * 59, 16, true);
    MakeSprite("Barlog", DEF_SPRID_MOB + 7 * 8 * 60, 40, true);
    MakeSprite("Centaurus", DEF_SPRID_MOB + 7 * 8 * 61, 40, true);
    MakeSprite("ClawTurtle", DEF_SPRID_MOB + 7 * 8 * 62, 40, true);
    MakeSprite("FireWyvern", DEF_SPRID_MOB + 7 * 8 * 63, 24, true);
    MakeSprite("GiantCrayfish", DEF_SPRID_MOB + 7 * 8 * 64, 40, true);
    MakeSprite("GiantLizard", DEF_SPRID_MOB + 7 * 8 * 65, 40, true);

    m_iLoadingStage = 44;
}

//=============================================================================
// Stage 44: New NPCs and monsters
//=============================================================================
void Screen_Loading::LoadStage_Monsters6()
{
    MakeSprite("GiantPlant", DEF_SPRID_MOB + 7 * 8 * 66, 40, true);
    MakeSprite("MasterMageOrc", DEF_SPRID_MOB + 7 * 8 * 67, 40, true);
    MakeSprite("Minotaurs", DEF_SPRID_MOB + 7 * 8 * 68, 40, true);
    MakeSprite("Nizie", DEF_SPRID_MOB + 7 * 8 * 69, 40, true);
    MakeSprite("Tentocle", DEF_SPRID_MOB + 7 * 8 * 70, 40, true);
    MakeSprite("yspro", DEF_SPRID_MOB + 7 * 8 * 71, 32, true);
    MakeSprite("Sorceress", DEF_SPRID_MOB + 7 * 8 * 72, 40, true);
    MakeSprite("TPKnight", DEF_SPRID_MOB + 7 * 8 * 73, 40, true);
    MakeSprite("ElfMaster", DEF_SPRID_MOB + 7 * 8 * 74, 40, true);
    MakeSprite("DarkKnight", DEF_SPRID_MOB + 7 * 8 * 75, 40, true);
    MakeSprite("HBTank", DEF_SPRID_MOB + 7 * 8 * 76, 32, true);
    MakeSprite("CBTurret", DEF_SPRID_MOB + 7 * 8 * 77, 32, true);
    MakeSprite("Babarian", DEF_SPRID_MOB + 7 * 8 * 78, 40, true);
    MakeSprite("ACannon", DEF_SPRID_MOB + 7 * 8 * 79, 32, true);

    m_iLoadingStage = 48;
}

//=============================================================================
// Stage 48: Male underwear, Gail, Heldenian Gate
//=============================================================================
void Screen_Loading::LoadStage_MaleUndies()
{
    MakeSprite("Gail", DEF_SPRID_MOB + 7 * 8 * 80, 8, true);
    MakeSprite("Gate", DEF_SPRID_MOB + 7 * 8 * 81, 24, true);

    // Male underwear
    SpriteLib::SpriteLoader::open_pak("Mpt", [&](SpriteLib::SpriteLoader& loader) {
        for (int g = 0; g < 8; g++) {
            for (int i = 0; i < 12; i++) {
                m_pGame->m_pSprite[DEF_SPRID_UNDIES_M + i + 15 * g] = loader.get_sprite(i + 12 * g, true);
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
    SpriteLib::SpriteLoader::open_pak("Mhr", [&](SpriteLib::SpriteLoader& loader) {
        for (int g = 0; g < 8; g++) {
            for (int i = 0; i < 12; i++) {
                m_pGame->m_pSprite[DEF_SPRID_HAIR_M + i + 15 * g] = loader.get_sprite(i + 12 * g, true);
            }
        }
    });

    // Male body armor
    MakeSprite("MLArmor", DEF_SPRID_BODYARMOR_M + 15 * 1, 12, true);
    MakeSprite("MCMail", DEF_SPRID_BODYARMOR_M + 15 * 2, 12, true);
    MakeSprite("MSMail", DEF_SPRID_BODYARMOR_M + 15 * 3, 12, true);
    MakeSprite("MPMail", DEF_SPRID_BODYARMOR_M + 15 * 4, 12, true);
    MakeSprite("Mtunic", DEF_SPRID_BODYARMOR_M + 15 * 5, 12, true);
    MakeSprite("MRobe1", DEF_SPRID_BODYARMOR_M + 15 * 6, 12, true);
    MakeSprite("MSanta", DEF_SPRID_BODYARMOR_M + 15 * 7, 12, true);
    MakeSprite("MHRobe1", DEF_SPRID_BODYARMOR_M + 15 * 10, 12, true);
    MakeSprite("MHRobe2", DEF_SPRID_BODYARMOR_M + 15 * 11, 12, true);
    MakeSprite("MHPMail1", DEF_SPRID_BODYARMOR_M + 15 * 8, 12, true);
    MakeSprite("MHPMail2", DEF_SPRID_BODYARMOR_M + 15 * 9, 12, true);

    // Male shirts
    MakeSprite("MShirt", DEF_SPRID_BERK_M + 15 * 1, 12, true);
    MakeSprite("MHauberk", DEF_SPRID_BERK_M + 15 * 2, 12, true);
    MakeSprite("MHHauberk1", DEF_SPRID_BERK_M + 15 * 3, 12, true);
    MakeSprite("MHHauberk2", DEF_SPRID_BERK_M + 15 * 4, 12, true);

    m_iLoadingStage = 56;
}

//=============================================================================
// Stage 56: Male pants, shoes, swords
//=============================================================================
void Screen_Loading::LoadStage_MaleLegs()
{
    // Male leggings
    MakeSprite("MTrouser", DEF_SPRID_LEGG_M + 15 * 1, 12, true);
    MakeSprite("MHTrouser", DEF_SPRID_LEGG_M + 15 * 2, 12, true);
    MakeSprite("MCHoses", DEF_SPRID_LEGG_M + 15 * 3, 12, true);
    MakeSprite("MLeggings", DEF_SPRID_LEGG_M + 15 * 4, 12, true);
    MakeSprite("MHLeggings1", DEF_SPRID_LEGG_M + 15 * 5, 12, true);
    MakeSprite("MHLeggings2", DEF_SPRID_LEGG_M + 15 * 6, 12, true);

    // Male boots
    MakeSprite("MShoes", DEF_SPRID_BOOT_M + 15 * 1, 12, true);
    MakeSprite("MLBoots", DEF_SPRID_BOOT_M + 15 * 2, 12, true);

    // Male swords (batch load)
    SpriteLib::SpriteLoader::open_pak("Msw", [&](SpriteLib::SpriteLoader& loader) {
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[DEF_SPRID_WEAPON_M + i + 64 * 1] = loader.get_sprite(i + 56 * 0, true);
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[DEF_SPRID_WEAPON_M + i + 64 * 2] = loader.get_sprite(i + 56 * 1, true);
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[DEF_SPRID_WEAPON_M + i + 64 * 3] = loader.get_sprite(i + 56 * 2, true);
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[DEF_SPRID_WEAPON_M + i + 64 * 4] = loader.get_sprite(i + 56 * 3, true);
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[DEF_SPRID_WEAPON_M + i + 64 * 6] = loader.get_sprite(i + 56 * 5, true);
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[DEF_SPRID_WEAPON_M + i + 64 * 7] = loader.get_sprite(i + 56 * 6, true);
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[DEF_SPRID_WEAPON_M + i + 64 * 8] = loader.get_sprite(i + 56 * 7, true);
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[DEF_SPRID_WEAPON_M + i + 64 * 9] = loader.get_sprite(i + 56 * 8, true);
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[DEF_SPRID_WEAPON_M + i + 64 * 10] = loader.get_sprite(i + 56 * 9, true);
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[DEF_SPRID_WEAPON_M + i + 64 * 11] = loader.get_sprite(i + 56 * 10, true);
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[DEF_SPRID_WEAPON_M + i + 64 * 12] = loader.get_sprite(i + 56 * 11, true);
    });

    m_iLoadingStage = 60;
}

//=============================================================================
// Stage 60: Male swords continued
//=============================================================================
void Screen_Loading::LoadStage_MaleSwords()
{
    MakeSprite("Mswx", DEF_SPRID_WEAPON_M + 64 * 5, 56, true);
    MakeSprite("Msw2", DEF_SPRID_WEAPON_M + 64 * 13, 56, true);
    MakeSprite("Msw3", DEF_SPRID_WEAPON_M + 64 * 14, 56, true);
    MakeSprite("MStormBringer", DEF_SPRID_WEAPON_M + 64 * 15, 56, true);
    MakeSprite("MDarkExec", DEF_SPRID_WEAPON_M + 64 * 16, 56, true);
    MakeSprite("MKlonessBlade", DEF_SPRID_WEAPON_M + 64 * 17, 56, true);
    MakeSprite("MKlonessAstock", DEF_SPRID_WEAPON_M + 64 * 18, 56, true);
    MakeSprite("MDebastator", DEF_SPRID_WEAPON_M + 64 * 19, 56, true);
    MakeSprite("MAxe1", DEF_SPRID_WEAPON_M + 64 * 20, 56, true);
    MakeSprite("MAxe2", DEF_SPRID_WEAPON_M + 64 * 21, 56, true);
    MakeSprite("MAxe3", DEF_SPRID_WEAPON_M + 64 * 22, 56, true);
    MakeSprite("MAxe4", DEF_SPRID_WEAPON_M + 64 * 23, 56, true);
    MakeSprite("MAxe5", DEF_SPRID_WEAPON_M + 64 * 24, 56, true);
    MakeSprite("MPickAxe1", DEF_SPRID_WEAPON_M + 64 * 25, 56, true);
    MakeSprite("MAxe6", DEF_SPRID_WEAPON_M + 64 * 26, 56, true);
    MakeSprite("Mhoe", DEF_SPRID_WEAPON_M + 64 * 27, 56, true);
    MakeSprite("MKlonessAxe", DEF_SPRID_WEAPON_M + 64 * 28, 56, true);
    MakeSprite("MLightBlade", DEF_SPRID_WEAPON_M + 64 * 29, 56, true);

    m_iLoadingStage = 64;
}

//=============================================================================
// Stage 64: Male hammers, staves, bows setup
//=============================================================================
void Screen_Loading::LoadStage_MaleWeapons()
{
    MakeSprite("MHammer", DEF_SPRID_WEAPON_M + 64 * 30, 56, true);
    MakeSprite("MBHammer", DEF_SPRID_WEAPON_M + 64 * 31, 56, true);
    MakeSprite("MBabHammer", DEF_SPRID_WEAPON_M + 64 * 32, 56, true);
    MakeSprite("MBShadowSword", DEF_SPRID_WEAPON_M + 64 * 33, 56, true);
    MakeSprite("MBerserkWand", DEF_SPRID_WEAPON_M + 64 * 34, 56, true);
    MakeSprite("Mstaff1", DEF_SPRID_WEAPON_M + 64 * 35, 56, true);
    MakeSprite("Mstaff2", DEF_SPRID_WEAPON_M + 64 * 36, 56, true);
    MakeSprite("MStaff3", DEF_SPRID_WEAPON_M + 64 * 37, 56, true);
    MakeSprite("MReMagicWand", DEF_SPRID_WEAPON_M + 64 * 38, 56, true);
    MakeSprite("MKlonessWand", DEF_SPRID_WEAPON_M + 64 * 39, 56, true);
    MakeSprite("MDirectBow", DEF_SPRID_WEAPON_M + 64 * 42, 56, true);
    MakeSprite("MFireBow", DEF_SPRID_WEAPON_M + 64 * 43, 56, true);

    m_iLoadingStage = 68;
}

//=============================================================================
// Stage 68: Male bows, shields
//=============================================================================
void Screen_Loading::LoadStage_MaleBows()
{
    // Male bows
    SpriteLib::SpriteLoader::open_pak("Mbo", [&](SpriteLib::SpriteLoader& loader) {
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[DEF_SPRID_WEAPON_M + i + 64 * 40] = loader.get_sprite(i + 56 * 0, true);
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[DEF_SPRID_WEAPON_M + i + 64 * 41] = loader.get_sprite(i + 56 * 1, true);
    });

    // Male shields
    SpriteLib::SpriteLoader::open_pak("Msh", [&](SpriteLib::SpriteLoader& loader) {
        for (int g = 0; g < 9; g++) {
            for (int i = 0; i < 7; i++) {
                m_pGame->m_pSprite[DEF_SPRID_SHIELD_M + i + 8 * (g + 1)] = loader.get_sprite(i + 7 * g, true);
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
    MakeSprite("Mmantle01", DEF_SPRID_MANTLE_M + 15 * 1, 12, true);
    MakeSprite("Mmantle02", DEF_SPRID_MANTLE_M + 15 * 2, 12, true);
    MakeSprite("Mmantle03", DEF_SPRID_MANTLE_M + 15 * 3, 12, true);
    MakeSprite("Mmantle04", DEF_SPRID_MANTLE_M + 15 * 4, 12, true);
    MakeSprite("Mmantle05", DEF_SPRID_MANTLE_M + 15 * 5, 12, true);
    MakeSprite("Mmantle06", DEF_SPRID_MANTLE_M + 15 * 6, 12, true);

    // Male helms
    MakeSprite("MHelm1", DEF_SPRID_HEAD_M + 15 * 1, 12, true);
    MakeSprite("MHelm2", DEF_SPRID_HEAD_M + 15 * 2, 12, true);
    MakeSprite("MHelm3", DEF_SPRID_HEAD_M + 15 * 3, 12, true);
    MakeSprite("MHelm4", DEF_SPRID_HEAD_M + 15 * 4, 12, true);
    MakeSprite("MHCap1", DEF_SPRID_HEAD_M + 15 * 11, 12, true);
    MakeSprite("MHCap2", DEF_SPRID_HEAD_M + 15 * 12, 12, true);
    MakeSprite("MHHelm1", DEF_SPRID_HEAD_M + 15 * 9, 12, true);
    MakeSprite("MHHelm2", DEF_SPRID_HEAD_M + 15 * 10, 12, true);
    MakeSprite("NMHelm1", DEF_SPRID_HEAD_M + 15 * 5, 12, true);
    MakeSprite("NMHelm2", DEF_SPRID_HEAD_M + 15 * 6, 12, true);
    MakeSprite("NMHelm3", DEF_SPRID_HEAD_M + 15 * 7, 12, true);
    MakeSprite("NMHelm4", DEF_SPRID_HEAD_M + 15 * 8, 12, true);

    m_iLoadingStage = 76;
}

//=============================================================================
// Stage 76: Female underwear and hair
//=============================================================================
void Screen_Loading::LoadStage_FemaleArmor()
{
    // Female underwear
    SpriteLib::SpriteLoader::open_pak("Wpt", [&](SpriteLib::SpriteLoader& loader) {
        for (int g = 0; g < 8; g++) {
            for (int i = 0; i < 12; i++) {
                m_pGame->m_pSprite[DEF_SPRID_UNDIES_W + i + 15 * g] = loader.get_sprite(i + 12 * g, true);
            }
        }
    });

    // Female hair
    SpriteLib::SpriteLoader::open_pak("Whr", [&](SpriteLib::SpriteLoader& loader) {
        for (int g = 0; g < 8; g++) {
            for (int i = 0; i < 12; i++) {
                m_pGame->m_pSprite[DEF_SPRID_HAIR_W + i + 15 * g] = loader.get_sprite(i + 12 * g, true);
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
    MakeSprite("WBodice1", DEF_SPRID_BODYARMOR_W + 15 * 1, 12, true);
    MakeSprite("WBodice2", DEF_SPRID_BODYARMOR_W + 15 * 2, 12, true);
    MakeSprite("WLArmor", DEF_SPRID_BODYARMOR_W + 15 * 3, 12, true);
    MakeSprite("WCMail", DEF_SPRID_BODYARMOR_W + 15 * 4, 12, true);
    MakeSprite("WSMail", DEF_SPRID_BODYARMOR_W + 15 * 5, 12, true);
    MakeSprite("WPMail", DEF_SPRID_BODYARMOR_W + 15 * 6, 12, true);
    MakeSprite("WRobe1", DEF_SPRID_BODYARMOR_W + 15 * 7, 12, true);
    MakeSprite("WSanta", DEF_SPRID_BODYARMOR_W + 15 * 8, 12, true);
    MakeSprite("WHRobe1", DEF_SPRID_BODYARMOR_W + 15 * 11, 12, true);
    MakeSprite("WHRobe2", DEF_SPRID_BODYARMOR_W + 15 * 12, 12, true);
    MakeSprite("WHPMail1", DEF_SPRID_BODYARMOR_W + 15 * 9, 12, true);
    MakeSprite("WHPMail2", DEF_SPRID_BODYARMOR_W + 15 * 10, 12, true);

    // Female shirts
    MakeSprite("WChemiss", DEF_SPRID_BERK_W + 15 * 1, 12, true);
    MakeSprite("WShirt", DEF_SPRID_BERK_W + 15 * 2, 12, true);
    MakeSprite("WHauberk", DEF_SPRID_BERK_W + 15 * 3, 12, true);
    MakeSprite("WHHauberk1", DEF_SPRID_BERK_W + 15 * 4, 12, true);
    MakeSprite("WHHauberk2", DEF_SPRID_BERK_W + 15 * 5, 12, true);

    // Female leggings
    MakeSprite("WSkirt", DEF_SPRID_LEGG_W + 15 * 1, 12, true);
    MakeSprite("WTrouser", DEF_SPRID_LEGG_W + 15 * 2, 12, true);
    MakeSprite("WHTrouser", DEF_SPRID_LEGG_W + 15 * 3, 12, true);
    MakeSprite("WHLeggings1", DEF_SPRID_LEGG_W + 15 * 6, 12, true);
    MakeSprite("WHLeggings2", DEF_SPRID_LEGG_W + 15 * 7, 12, true);
    MakeSprite("WCHoses", DEF_SPRID_LEGG_W + 15 * 4, 12, true);
    MakeSprite("WLeggings", DEF_SPRID_LEGG_W + 15 * 5, 12, true);

    // Female boots
    MakeSprite("WShoes", DEF_SPRID_BOOT_W + 15 * 1, 12, true);
    MakeSprite("WLBoots", DEF_SPRID_BOOT_W + 15 * 2, 12, true);

    m_iLoadingStage = 84;
}

//=============================================================================
// Stage 84: Female swords
//=============================================================================
void Screen_Loading::LoadStage_FemaleSwords()
{
    // Female swords (batch load)
    SpriteLib::SpriteLoader::open_pak("Wsw", [&](SpriteLib::SpriteLoader& loader) {
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[DEF_SPRID_WEAPON_W + i + 64 * 1] = loader.get_sprite(i + 56 * 0, true);
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[DEF_SPRID_WEAPON_W + i + 64 * 2] = loader.get_sprite(i + 56 * 1, true);
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[DEF_SPRID_WEAPON_W + i + 64 * 3] = loader.get_sprite(i + 56 * 2, true);
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[DEF_SPRID_WEAPON_W + i + 64 * 4] = loader.get_sprite(i + 56 * 3, true);
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[DEF_SPRID_WEAPON_W + i + 64 * 6] = loader.get_sprite(i + 56 * 5, true);
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[DEF_SPRID_WEAPON_W + i + 64 * 7] = loader.get_sprite(i + 56 * 6, true);
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[DEF_SPRID_WEAPON_W + i + 64 * 8] = loader.get_sprite(i + 56 * 7, true);
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[DEF_SPRID_WEAPON_W + i + 64 * 9] = loader.get_sprite(i + 56 * 8, true);
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[DEF_SPRID_WEAPON_W + i + 64 * 10] = loader.get_sprite(i + 56 * 9, true);
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[DEF_SPRID_WEAPON_W + i + 64 * 11] = loader.get_sprite(i + 56 * 10, true);
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[DEF_SPRID_WEAPON_W + i + 64 * 12] = loader.get_sprite(i + 56 * 11, true);
    });

    MakeSprite("Wswx", DEF_SPRID_WEAPON_W + 64 * 5, 56, true);
    MakeSprite("Wsw2", DEF_SPRID_WEAPON_W + 64 * 13, 56, true);
    MakeSprite("Wsw3", DEF_SPRID_WEAPON_W + 64 * 14, 56, true);
    MakeSprite("WStormBringer", DEF_SPRID_WEAPON_W + 64 * 15, 56, true);
    MakeSprite("WDarkExec", DEF_SPRID_WEAPON_W + 64 * 16, 56, true);
    MakeSprite("WKlonessBlade", DEF_SPRID_WEAPON_W + 64 * 17, 56, true);
    MakeSprite("WKlonessAstock", DEF_SPRID_WEAPON_W + 64 * 18, 56, true);
    MakeSprite("WDebastator", DEF_SPRID_WEAPON_W + 64 * 19, 56, true);

    m_iLoadingStage = 88;
}

//=============================================================================
// Stage 88: Female axes, staves
//=============================================================================
void Screen_Loading::LoadStage_FemaleWeapons()
{
    MakeSprite("WAxe1", DEF_SPRID_WEAPON_W + 64 * 20, 56, true);
    MakeSprite("WAxe2", DEF_SPRID_WEAPON_W + 64 * 21, 56, true);
    MakeSprite("WAxe3", DEF_SPRID_WEAPON_W + 64 * 22, 56, true);
    MakeSprite("WAxe4", DEF_SPRID_WEAPON_W + 64 * 23, 56, true);
    MakeSprite("WAxe5", DEF_SPRID_WEAPON_W + 64 * 24, 56, true);
    MakeSprite("WpickAxe1", DEF_SPRID_WEAPON_W + 64 * 25, 56, true);
    MakeSprite("WAxe6", DEF_SPRID_WEAPON_W + 64 * 26, 56, true);
    MakeSprite("Whoe", DEF_SPRID_WEAPON_W + 64 * 27, 56, true);
    MakeSprite("WKlonessAxe", DEF_SPRID_WEAPON_W + 64 * 28, 56, true);
    MakeSprite("WLightBlade", DEF_SPRID_WEAPON_W + 64 * 29, 56, true);
    MakeSprite("WHammer", DEF_SPRID_WEAPON_W + 64 * 30, 56, true);
    MakeSprite("WBHammer", DEF_SPRID_WEAPON_W + 64 * 31, 56, true);
    MakeSprite("WBabHammer", DEF_SPRID_WEAPON_W + 64 * 32, 56, true);
    MakeSprite("WBShadowSword", DEF_SPRID_WEAPON_W + 64 * 33, 56, true);
    MakeSprite("WBerserkWand", DEF_SPRID_WEAPON_W + 64 * 34, 56, true);
    MakeSprite("Wstaff1", DEF_SPRID_WEAPON_W + 64 * 35, 56, true);
    MakeSprite("Wstaff2", DEF_SPRID_WEAPON_W + 64 * 36, 56, true);
    MakeSprite("WStaff3", DEF_SPRID_WEAPON_W + 64 * 37, 56, true);
    MakeSprite("WKlonessWand", DEF_SPRID_WEAPON_W + 64 * 39, 56, true);
    MakeSprite("WReMagicWand", DEF_SPRID_WEAPON_W + 64 * 38, 56, true);
    MakeSprite("WDirectBow", DEF_SPRID_WEAPON_W + 64 * 42, 56, true);
    MakeSprite("WFireBow", DEF_SPRID_WEAPON_W + 64 * 43, 56, true);

    m_iLoadingStage = 92;
}

//=============================================================================
// Stage 92: Female mantles, helms
//=============================================================================
void Screen_Loading::LoadStage_FemaleMantles()
{
    // Female mantles
    MakeSprite("Wmantle01", DEF_SPRID_MANTLE_W + 15 * 1, 12, true);
    MakeSprite("Wmantle02", DEF_SPRID_MANTLE_W + 15 * 2, 12, true);
    MakeSprite("Wmantle03", DEF_SPRID_MANTLE_W + 15 * 3, 12, true);
    MakeSprite("Wmantle04", DEF_SPRID_MANTLE_W + 15 * 4, 12, true);
    MakeSprite("Wmantle05", DEF_SPRID_MANTLE_W + 15 * 5, 12, true);
    MakeSprite("Wmantle06", DEF_SPRID_MANTLE_W + 15 * 6, 12, true);

    // Female helms
    MakeSprite("WHelm1", DEF_SPRID_HEAD_W + 15 * 1, 12, true);
    MakeSprite("WHelm4", DEF_SPRID_HEAD_W + 15 * 4, 12, true);
    MakeSprite("WHHelm1", DEF_SPRID_HEAD_W + 15 * 9, 12, true);
    MakeSprite("WHHelm2", DEF_SPRID_HEAD_W + 15 * 10, 12, true);
    MakeSprite("WHCap1", DEF_SPRID_HEAD_W + 15 * 11, 12, true);
    MakeSprite("WHCap2", DEF_SPRID_HEAD_W + 15 * 12, 12, true);
    MakeSprite("NWHelm1", DEF_SPRID_HEAD_W + 15 * 5, 12, true);
    MakeSprite("NWHelm2", DEF_SPRID_HEAD_W + 15 * 6, 12, true);
    MakeSprite("NWHelm3", DEF_SPRID_HEAD_W + 15 * 7, 12, true);
    MakeSprite("NWHelm4", DEF_SPRID_HEAD_W + 15 * 8, 12, true);

    m_iLoadingStage = 96;
}

//=============================================================================
// Stage 96: Female bows, shields
//=============================================================================
void Screen_Loading::LoadStage_FemaleBows()
{
    // Female bows
    SpriteLib::SpriteLoader::open_pak("Wbo", [&](SpriteLib::SpriteLoader& loader) {
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[DEF_SPRID_WEAPON_W + i + 64 * 40] = loader.get_sprite(i + 56 * 0, true);
        for (int i = 0; i < 56; i++) m_pGame->m_pSprite[DEF_SPRID_WEAPON_W + i + 64 * 41] = loader.get_sprite(i + 56 * 1, true);
    });

    // Female shields
    SpriteLib::SpriteLoader::open_pak("Wsh", [&](SpriteLib::SpriteLoader& loader) {
        for (int g = 0; g < 9; g++) {
            for (int i = 0; i < 7; i++) {
                m_pGame->m_pSprite[DEF_SPRID_SHIELD_W + i + 8 * (g + 1)] = loader.get_sprite(i + 7 * g, true);
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
    SpriteLib::SpriteLoader::open_pak("effect5", [&](SpriteLib::SpriteLoader& loader) {
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
        SpriteLib::SpriteLoader::open_pak(FileName, [&](SpriteLib::SpriteLoader& loader) {
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
        SpriteLib::SpriteLoader::open_pak(FileName, [&](SpriteLib::SpriteLoader& loader) {
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
        SpriteLib::SpriteLoader::open_pak(FileName, [&](SpriteLib::SpriteLoader& loader) {
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

// Screen_Loading.h: Loading Screen Interface
//
// Resource loading screen that progressively loads game assets.
// Handles all sprite, tile, effect, and sound loading in stages.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include "IGameScreen.h"

class Screen_Loading : public IGameScreen
{
public:
    SCREEN_TYPE(Screen_Loading)

    explicit Screen_Loading(CGame* pGame);
    ~Screen_Loading() override = default;

    void on_initialize() override;
    void on_uninitialize() override;
    void on_update() override;
    void on_render() override;

    // Get loading progress (0-100)
    int GetProgress() const { return m_iLoadingStage; }

private:
    // Loading stages - each stage loads a batch of resources
    void LoadStage_Interface();      // Stage 0: UI sprites, dialog boxes
    void LoadStage_Tiles1();         // Stage 4: Map tiles, structures, trees
    void LoadStage_Tiles2();         // Stage 8: More tiles, objects
    void LoadStage_Tiles3();         // Stage 12: More tiles, items
    void LoadStage_Equipment1();     // Stage 16: Male/Female equipment base
    void LoadStage_Angels();         // Stage 20: Tutelary angels, player bodies
    void LoadStage_Monsters1();      // Stage 24: Monsters (slime to William)
    void LoadStage_Monsters2();      // Stage 28: Monsters (Kennedy to EnergyBall)
    void LoadStage_Monsters3();      // Stage 32: Guard towers, structures
    void LoadStage_Monsters4();      // Stage 36: More monsters
    void LoadStage_Monsters5();      // Stage 40: More monsters
    void LoadStage_Monsters6();      // Stage 44: NPCs, new monsters
    void LoadStage_MaleUndies();     // Stage 48: Male underwear, Gail, Gate
    void LoadStage_MaleArmor();      // Stage 52: Male hair, armor, shirts
    void LoadStage_MaleLegs();       // Stage 56: Male pants, shoes, swords
    void LoadStage_MaleSwords();     // Stage 60: Male weapons continued
    void LoadStage_MaleWeapons();    // Stage 64: Male axes, hammers, staves
    void LoadStage_MaleBows();       // Stage 68: Male bows, shields, mantles
    void LoadStage_FemaleBase();     // Stage 72: Female underwear, hair
    void LoadStage_FemaleArmor();    // Stage 76: Female armor, shirts
    void LoadStage_FemaleLegs();     // Stage 80: Female pants, shoes, swords
    void LoadStage_FemaleSwords();   // Stage 84: Female weapons continued
    void LoadStage_FemaleWeapons();  // Stage 88: Female axes, staves
    void LoadStage_FemaleMantles();  // Stage 92: Female mantles, helms
    void LoadStage_FemaleBows();     // Stage 96: Female bows, shields
    void LoadStage_Effects();        // Stage 100: Effects, sounds, finish

    // Sprite loading helpers
    void MakeSprite(const char* FileName, short sStart, short sCount, bool bAlphaEffect);
    void MakeTileSpr(const char* FileName, short sStart, short sCount, bool bAlphaEffect);
    void MakeEffectSpr(const char* FileName, short sStart, short sCount, bool bAlphaEffect);

    int m_iLoadingStage = 0;
};

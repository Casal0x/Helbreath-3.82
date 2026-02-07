// ItemEnums.h: Unified type-safe enums for the Item system
//
// Replaces divergent DEF_ macros from Client/Item.h and Server/Item.h
// Naming decisions:
//   - Slot 5: Leggings (matches item data: PlateLeggings, ChainHose)
//   - Slot 13: FullBody (describes slot purpose, not behavior)
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include <cstdint>

namespace hb {
namespace item {

//------------------------------------------------------------------------
// Equipment Position
//------------------------------------------------------------------------
enum class EquipPos : int8_t
{
    None      = 0,
    Head      = 1,
    Body      = 2,
    Arms      = 3,
    Pants     = 4,
    Leggings  = 5,   // Leg armor (ChainHose, PlateLeggings)
    Neck      = 6,
    LeftHand  = 7,
    RightHand = 8,
    TwoHand   = 9,
    RightFinger = 10,
    LeftFinger  = 11,
    Back      = 12,
    FullBody  = 13,  // Full-body armor (robes) - releases other armor slots when equipped

    Max       = 15
};

constexpr int DEF_MAXITEMEQUIPPOS = 15;

//------------------------------------------------------------------------
// Item Type
//------------------------------------------------------------------------
enum class ItemType : int8_t
{
    NotUsed                = -1,  // Previously used, but currently unused item
    None                   = 0,
    Equip                  = 1,
    Apply                  = 2,
    UseDeplete             = 3,
    Install                = 4,
    Consume                = 5,
    Arrow                  = 6,
    Eat                    = 7,
    UseSkill               = 8,
    UsePerm                = 9,
    UseSkillEnableDialogBox = 10,
    UseDepleteDest         = 11,
    Material               = 12
};

//------------------------------------------------------------------------
// Item Effect Type
//------------------------------------------------------------------------
enum class ItemEffectType : int16_t
{
    None               = 0,
    Attack             = 1,   // Attack value: value1 D value2 + value3
    Defense            = 2,   // Defense capability
    AttackArrow        = 3,   // Arrow attack - adds to base weapon damage
    HP                 = 4,   // HP restoration effect
    MP                 = 5,   // MP restoration effect
    SP                 = 6,   // SP restoration effect
    HPStock            = 7,   // HP recovery over time (no immediate visual)
    Get                = 8,   // Acquire something (tools, containers)
    StudySkill         = 9,   // Skill learning item
    ShowLocation       = 10,  // Shows location on map
    Magic              = 11,  // Item with magic effect when used
    ChangeAttr         = 12,  // Changes player attributes (hair, skin, etc.)
    AttackManaSave     = 13,  // Attack with mana saving effect
    AddEffect          = 14,  // Additional effect
    MagicDamageSave    = 15,  // Magic damage absorption
    OccupyFlag         = 16,  // Capture flag
    Dye                = 17,  // Dye item
    StudyMagic         = 18,  // Magic learning item
    AttackMaxHPDown    = 19,  // Attack that reduces max HP and HP recovery
    AttackDefense      = 20,  // Attack with defense reduction effect
    MaterialAttr       = 21,  // Material attribute for crafting
    FirmStamina        = 22,  // Stamina enhancement
    Lottery            = 23,  // Lottery ticket
    AttackSpecAbility  = 24,  // Attack with special ability
    DefenseSpecAbility = 25,  // Defense with special ability
    AlterItemDrop      = 26,  // Affects item drop of other items
    ConstructionKit    = 27,  // Construction kit
    Warm               = 28,  // Unfreeze potion effect
    // 29 is unused
    Farming            = 30,  // Farming item
    Slates             = 31,  // Ancient Tablets
    ArmorDye           = 32,  // Armor dye
    CritKomm           = 33,  // Crit Candy
    WeaponDye          = 34   // Weapon dye
};

//------------------------------------------------------------------------
// AddEffect Sub-Types (used with ItemEffectType::AddEffect)
// m_sItemEffectValue1 contains the sub-type, m_sItemEffectValue2 contains the value
//------------------------------------------------------------------------
enum class AddEffectType : int16_t
{
    MagicResist     = 1,   // Additional magic resistance
    ManaSave        = 2,   // Mana save percentage
    PoisonResist    = 3,   // Poison resistance
    CriticalHit     = 4,   // Critical hit chance
    Poisoning       = 5,   // Poison attack
    RepairItem      = 6,   // Repair item
    FireAbsorb      = 7,   // Fire damage absorption
    IceAbsorb       = 8,   // Ice damage absorption
    LightAbsorb     = 9,   // Light damage absorption
    ExpBonus        = 10,  // Experience bonus
    GoldBonus       = 11,  // Gold drop bonus
    // 12 unused
    Summon          = 13,  // Summon creature
    CancelBuff      = 14,  // Cancel buffs
    // 15-17 unused
    SpecialAbility  = 18,  // Special ability
    ParalysisImmune = 19   // Paralysis immunity
};

constexpr int16_t ToInt(AddEffectType type) { return static_cast<int16_t>(type); }

//------------------------------------------------------------------------
// Touch Effect Type (item-specific effects on first touch/creation)
//------------------------------------------------------------------------
enum class TouchEffectType : int16_t
{
    None        = 0,
    UniqueOwner = 1,  // Item bound to owner
    ID          = 2,  // Item has unique ID
    Date        = 3   // Item has expiration date
};

//------------------------------------------------------------------------
// Helper Functions
//------------------------------------------------------------------------

// Check if equipment position is a weapon slot
constexpr bool IsWeaponSlot(EquipPos pos)
{
    return pos == EquipPos::LeftHand ||
           pos == EquipPos::RightHand ||
           pos == EquipPos::TwoHand;
}

// Check if equipment position is an armor slot
constexpr bool IsArmorSlot(EquipPos pos)
{
    return pos == EquipPos::Head ||
           pos == EquipPos::Body ||
           pos == EquipPos::Arms ||
           pos == EquipPos::Pants ||
           pos == EquipPos::Leggings ||
           pos == EquipPos::Back ||
           pos == EquipPos::FullBody;
}

// Check if equipment position is an accessory slot
constexpr bool IsAccessorySlot(EquipPos pos)
{
    return pos == EquipPos::Neck ||
           pos == EquipPos::RightFinger ||
           pos == EquipPos::LeftFinger;
}

// Check if item type indicates stackable items (includes soft-linked types like potions)
constexpr bool IsStackableType(ItemType type)
{
    return type == ItemType::Arrow ||
           type == ItemType::Consume ||
           type == ItemType::Eat ||
           type == ItemType::UseDeplete ||
           type == ItemType::UseDepleteDest ||
           type == ItemType::Material;
}

// Check if item type is a true stack (single inventory entry with count > 1).
// Soft-linked types (Consume, Eat, etc.) are individual items that the client
// groups by item ID and displays with a count.
constexpr bool IsTrueStackType(ItemType type)
{
    return type == ItemType::Arrow ||
           type == ItemType::Material;
}

// Check if item effect type is an attack type
constexpr bool IsAttackEffectType(ItemEffectType type)
{
    return type == ItemEffectType::Attack ||
           type == ItemEffectType::AttackArrow ||
           type == ItemEffectType::AttackManaSave ||
           type == ItemEffectType::AttackMaxHPDown ||
           type == ItemEffectType::AttackDefense ||
           type == ItemEffectType::AttackSpecAbility;
}

// Check if item effect type is consumable (potion-like)
constexpr bool IsConsumableEffectType(ItemEffectType type)
{
    return type == ItemEffectType::HP ||
           type == ItemEffectType::MP ||
           type == ItemEffectType::SP ||
           type == ItemEffectType::HPStock;
}

//------------------------------------------------------------------------
// Enum conversion helpers (for serialization/deserialization)
//------------------------------------------------------------------------

constexpr int8_t ToInt(EquipPos pos) { return static_cast<int8_t>(pos); }
constexpr int8_t ToInt(ItemType type) { return static_cast<int8_t>(type); }
constexpr int16_t ToInt(ItemEffectType type) { return static_cast<int16_t>(type); }
constexpr int16_t ToInt(TouchEffectType type) { return static_cast<int16_t>(type); }

constexpr EquipPos ToEquipPos(int8_t val) { return static_cast<EquipPos>(val); }
constexpr ItemType ToItemType(int8_t val) { return static_cast<ItemType>(val); }
constexpr ItemEffectType ToItemEffectType(int16_t val) { return static_cast<ItemEffectType>(val); }
constexpr TouchEffectType ToTouchEffectType(int16_t val) { return static_cast<TouchEffectType>(val); }

//------------------------------------------------------------------------
// Common Item IDs
// These are well-known item IDs that are frequently referenced in code
//------------------------------------------------------------------------
namespace ItemId
{
    constexpr short Excaliber = 20;
    constexpr short Arrow = 77;
    constexpr short PoisonArrow = 78;
    constexpr short GuildAdmissionTicket = 88;
    constexpr short GuildSecessionTicket = 89;
    constexpr short Gold = 90;
    constexpr short MagicWandMShield = 259;
    constexpr short MagicWandMS30LLF = 291;
    constexpr short MagicNecklaceRM10 = 300;
    constexpr short MagicNecklaceDMp1 = 305;
    constexpr short MagicNecklaceMS10 = 308;
    constexpr short MagicNecklaceDFp10 = 311;
    constexpr short EmeraldRing = 335;
    constexpr short SapphireRing = 336;
    constexpr short RubyRing = 337;
    constexpr short AresdenHeroCape = 400;
    constexpr short ElvineHeroCape = 401;
    constexpr short AresdenHeroHelmM = 403;
    constexpr short ElvineHeroLeggingsW = 426;
    constexpr short AresdenHeroCapePlus1 = 427;
    constexpr short ElvineHeroCapePlus1 = 428;
    constexpr short BloodSword = 490;
    constexpr short BloodAxe = 491;
    constexpr short BloodRapier = 492;
    constexpr short XelimaBlade = 610;
    constexpr short XelimaAxe = 611;
    constexpr short XelimaRapier = 612;
    constexpr short DemonSlayer = 616;
    constexpr short DarkElfBow = 618;
    constexpr short MerienShield = 620;
    constexpr short MerienPlateMailM = 621;
    constexpr short MerienPlateMailW = 622;
    constexpr short RingoftheXelima = 630;
    constexpr short RingoftheAbaddon = 631;
    constexpr short RingofOgrepower = 632;
    constexpr short RingofDemonpower = 633;
    constexpr short RingofWizard = 634;
    constexpr short RingofMage = 635;
    constexpr short RingofGrandMage = 636;
    constexpr short NecklaceOfBeholder = 646;
    constexpr short NecklaceOfStoneGol = 647;
    constexpr short NecklaceOfLiche = 648;
    constexpr short StoneOfXelima = 656;
    constexpr short StoneOfMerien = 657;
    constexpr short SwordofMedusa = 724;
    constexpr short SwordofIceElemental = 725;
    constexpr short RingofArcmage = 734;
    constexpr short RingofDragonpower = 735;
    constexpr short ZemstoneofSacrifice = 753;
    constexpr short StormBringer = 845;
    constexpr short KlonessBlade = 849;
    constexpr short KlonessAxe = 850;
    constexpr short KlonessEsterk = 851;
    constexpr short NecklaceOfMerien = 858;
    constexpr short NecklaceOfKloness = 859;
    constexpr short NecklaceOfXelima = 860;
    constexpr short BerserkWandMS20 = 861;
    constexpr short BerserkWandMS10 = 862;
    constexpr short KlonessWandMS20 = 863;
    constexpr short KlonessWandMS10 = 864;
    constexpr short ResurWandMS20 = 865;
    constexpr short ResurWandMS10 = 866;
    constexpr short AcientTablet = 867;
    constexpr short AcientTabletLU = 868;
    constexpr short AcientTabletLD = 869;
    constexpr short AcientTabletRU = 870;
    constexpr short AcientTabletRD = 871;
    constexpr short DarkExecutor = 879;
    constexpr short TheDevastator = 880;
    constexpr short LightingBlade = 881;
    constexpr short MagicNecklaceDFp15 = 1086;
    constexpr short MagicNecklaceRM30 = 1101;
    constexpr short AngelicPandentSTR = 1108;
    constexpr short AngelicPandentDEX = 1109;
    constexpr short AngelicPandentINT = 1110;
    constexpr short AngelicPandentMAG = 1111;
}

inline bool IsSpecialItem(short sIDnum)
{
    switch (sIDnum) {
    case ItemId::Excaliber:
    case ItemId::MagicWandMShield:
    case ItemId::MagicWandMS30LLF:
    case ItemId::MagicNecklaceRM10:
    case ItemId::MagicNecklaceDMp1:
    case ItemId::MagicNecklaceMS10:
    case ItemId::MagicNecklaceDFp10:
    case ItemId::EmeraldRing:
    case ItemId::SapphireRing:
    case ItemId::RubyRing:
    case ItemId::AresdenHeroCape:
    case ItemId::ElvineHeroCape:
    case ItemId::AresdenHeroCapePlus1:
    case ItemId::ElvineHeroCapePlus1:
    case ItemId::BloodSword:
    case ItemId::BloodAxe:
    case ItemId::BloodRapier:
    case ItemId::XelimaBlade:
    case ItemId::XelimaAxe:
    case ItemId::XelimaRapier:
    case ItemId::DemonSlayer:
    case ItemId::DarkElfBow:
    case ItemId::MerienShield:
    case ItemId::MerienPlateMailM:
    case ItemId::MerienPlateMailW:
    case ItemId::RingoftheXelima:
    case ItemId::RingoftheAbaddon:
    case ItemId::RingofOgrepower:
    case ItemId::RingofDemonpower:
    case ItemId::RingofWizard:
    case ItemId::RingofMage:
    case ItemId::RingofGrandMage:
    case ItemId::NecklaceOfBeholder:
    case ItemId::NecklaceOfStoneGol:
    case ItemId::NecklaceOfLiche:
    case ItemId::StoneOfXelima:
    case ItemId::StoneOfMerien:
    case ItemId::SwordofMedusa:
    case ItemId::SwordofIceElemental:
    case ItemId::RingofArcmage:
    case ItemId::RingofDragonpower:
    case ItemId::ZemstoneofSacrifice:
    case ItemId::StormBringer:
    case ItemId::KlonessBlade:
    case ItemId::KlonessAxe:
    case ItemId::KlonessEsterk:
    case ItemId::NecklaceOfMerien:
    case ItemId::NecklaceOfKloness:
    case ItemId::NecklaceOfXelima:
    case ItemId::BerserkWandMS20:
    case ItemId::BerserkWandMS10:
    case ItemId::KlonessWandMS20:
    case ItemId::KlonessWandMS10:
    case ItemId::ResurWandMS20:
    case ItemId::ResurWandMS10:
    case ItemId::AcientTablet:
    case ItemId::AcientTabletLU:
    case ItemId::AcientTabletLD:
    case ItemId::AcientTabletRU:
    case ItemId::AcientTabletRD:
    case ItemId::DarkExecutor:
    case ItemId::TheDevastator:
    case ItemId::LightingBlade:
    case ItemId::MagicNecklaceDFp15:
    case ItemId::MagicNecklaceRM30:
    case ItemId::AngelicPandentSTR:
    case ItemId::AngelicPandentDEX:
    case ItemId::AngelicPandentINT:
    case ItemId::AngelicPandentMAG:
        return true;
    default:
        // Also check ranges for items between known IDs
        if (sIDnum >= ItemId::AresdenHeroHelmM && sIDnum <= ItemId::ElvineHeroLeggingsW) return true;  // Hero items 403-426
        if (sIDnum >= ItemId::MagicNecklaceDFp15 && sIDnum <= ItemId::MagicNecklaceRM30) return true;  // Magic necklaces 1086-1101
        return false;
    }
}

} // namespace item
} // namespace hb

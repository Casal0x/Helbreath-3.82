// EntityRenderState.h: Temporary state for the entity currently being rendered
//
// This class encapsulates the _tmp_* global variables that were used to hold
// the current entity's data during the render loop. It provides a clean
// interface for accessing entity properties during drawing operations.
//////////////////////////////////////////////////////////////////////

#pragma once

#include <cstdint>
#include <array>
#include <cstring>

// Maximum length for entity names
constexpr int ENTITY_NAME_LENGTH = 12;

// Entity relationship to the player (Friend or Enemy)
enum class EEntityRelation : int8_t
{
    Hostile_PK = -2,    // Player Killer - always hostile
    Hostile = -1,       // Enemy faction (in crusade) or player is PK
    Neutral = 0,        // Not a citizen, or indeterminate
    Friendly = 1        // Same faction as player
};

// Status flag bits (extracted from m_iStatus)
namespace EntityStatusFlags
{
    constexpr int PK       = 0x80000000;  // Player Killer
    constexpr int Citizen  = 0x40000000;  // Is a citizen of a town
    constexpr int Aresden  = 0x20000000;  // Belongs to Aresden faction
    constexpr int Hunter   = 0x10000000;  // Is a hunter

    // Lower bits contain other status effects
    constexpr int Poisoned   = 0x00000080;
    constexpr int Frozen     = 0x00000020;
    constexpr int Confused   = 0x00000010;
}

class CEntityRenderState
{
public:
    CEntityRenderState() { Reset(); }

    void Reset()
    {
        m_wObjectID = 0;
        m_sOwnerType = 0;
        m_sAppr1 = 0;
        m_sAppr2 = 0;
        m_sAppr3 = 0;
        m_sAppr4 = 0;
        m_iApprColor = 0;
        m_iStatus = 0;
        m_iAction = 0;
        m_iDir = 0;
        m_iFrame = 0;
        m_cName.fill('\0');
        m_iChatIndex = 0;
        m_iMoveOffsetX = 0;
        m_iMoveOffsetY = 0;
        m_iDataX = 0;
        m_iDataY = 0;
        m_iEffectType = 0;
        m_iEffectFrame = 0;
        m_bSpriteOmit = false;
    }

    // Set name from C-string (copies up to ENTITY_NAME_LENGTH-1 chars)
    void SetName(const char* name)
    {
        m_cName.fill('\0');
        if (name) {
            std::strncpy(m_cName.data(), name, ENTITY_NAME_LENGTH - 1);
        }
    }

    // Get name as C-string
    const char* GetName() const { return m_cName.data(); }

    //------------------------------------------------------------------
    // Status Flag Helpers
    //------------------------------------------------------------------

    bool IsPK() const { return (m_iStatus & EntityStatusFlags::PK) != 0; }
    bool IsCitizen() const { return (m_iStatus & EntityStatusFlags::Citizen) != 0; }
    bool IsAresden() const { return (m_iStatus & EntityStatusFlags::Aresden) != 0; }
    bool IsElvine() const { return IsCitizen() && !IsAresden(); }
    bool IsHunter() const { return (m_iStatus & EntityStatusFlags::Hunter) != 0; }
    bool IsPoisoned() const { return (m_iStatus & EntityStatusFlags::Poisoned) != 0; }
    bool IsFrozen() const { return (m_iStatus & EntityStatusFlags::Frozen) != 0; }
    bool IsConfused() const { return (m_iStatus & EntityStatusFlags::Confused) != 0; }

    //------------------------------------------------------------------
    // Entity Type Helpers
    //------------------------------------------------------------------

    // Player types are 1-6 (male/female for each class)
    bool IsPlayer() const { return m_sOwnerType >= 1 && m_sOwnerType <= 6; }

    // NPC/Monster types are 7+
    bool IsNPC() const { return m_sOwnerType >= 7; }

    //------------------------------------------------------------------
    // Member Variables
    //------------------------------------------------------------------

    // Identification
    uint16_t m_wObjectID;       // Entity's unique object ID
    short m_sOwnerType;         // Entity type (player/mob/NPC type ID)

    // Appearance
    short m_sAppr1;             // Appearance data 1 (gender, hair, etc.)
    short m_sAppr2;             // Appearance data 2 (weapon, shield)
    short m_sAppr3;             // Appearance data 3 (armor, helm)
    short m_sAppr4;             // Appearance data 4 (mantle, etc.)
    int m_iApprColor;           // Appearance color tint

    // State
    int m_iStatus;              // Status flags (poisoned, invisible, etc.)
    int8_t m_iAction;           // Current action (idle, walk, attack, etc.)
    int8_t m_iDir;              // Facing direction (1-8)
    int8_t m_iFrame;            // Current animation frame

    // Display
    std::array<char, ENTITY_NAME_LENGTH> m_cName;  // Entity name
    int m_iChatIndex;           // Index into chat message array (0 = no message)

    // Position/Movement
    int m_iMoveOffsetX;         // Pixel offset during movement (was _tmp_dx)
    int m_iMoveOffsetY;         // Pixel offset during movement (was _tmp_dy)
    int m_iDataX;               // Map data array X index (was _tmp_dX)
    int m_iDataY;               // Map data array Y index (was _tmp_dY)

    // Effects
    int m_iEffectType;          // Visual effect type (aura, spell, etc.)
    int m_iEffectFrame;         // Visual effect animation frame

    // Rendering flags
    bool m_bSpriteOmit;         // Skip certain sprite rendering
};

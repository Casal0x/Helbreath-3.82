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

#include "AppearanceData.h"
#include "PlayerStatusData.h"
#include "OwnerType.h"

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

class CEntityRenderState
{
public:
    CEntityRenderState() { Reset(); }

    void Reset()
    {
        m_wObjectID = 0;
        m_sOwnerType = 0;
        m_appearance.Clear();
        m_status.Clear();
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
    }

    // Get name as C-string
    const char* GetName() const { return m_cName.data(); }

    //------------------------------------------------------------------
    // Entity Type Helpers
    //------------------------------------------------------------------

    bool IsPlayer() const { return hb::owner::IsPlayer(m_sOwnerType); }
    bool IsNPC() const { return hb::owner::IsNPC(m_sOwnerType); }
    bool IsMale() const { return hb::owner::IsMale(m_sOwnerType); }
    bool IsFemale() const { return hb::owner::IsFemale(m_sOwnerType); }

    //------------------------------------------------------------------
    // Member Variables
    //------------------------------------------------------------------

    // Identification
    uint16_t m_wObjectID;       // Entity's unique object ID
    short m_sOwnerType;         // Entity type (player/mob/NPC type ID)

    // Unpacked appearance (named fields extracted from sAppr1-4 at packet reception)
    PlayerAppearance m_appearance;

    // State
    PlayerStatus m_status;
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

};

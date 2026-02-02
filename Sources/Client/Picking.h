// Picking.h: Object picking and cursor management system
//
// Provides static global access to picking state with frame-based semantics.
// Separates hit testing from rendering for cleaner architecture.
//////////////////////////////////////////////////////////////////////

#pragma once

#include <cstdint>
#include "SpriteTypes.h"

//=============================================================================
// Cursor Types (replacing magic frame numbers)
//=============================================================================
enum class CursorType {
    Arrow = 0,           // Default cursor
    ItemGround1 = 1,     // Item on ground (animated frame 1)
    ItemGround2 = 2,     // Item on ground (animated frame 2)
    TargetHostile = 3,   // Red - hostile/dead body
    SpellFriendly = 4,   // Blue - spell on friendly
    SpellHostile = 5,    // Red - spell on hostile
    TargetNeutral = 6,   // Blue - neutral/friendly
    Unavailable = 8,     // Can't perform action
    ItemUse = 10         // Hand cursor for item use
};

//=============================================================================
// Focused Object Type
//=============================================================================
enum class FocusedObjectType {
    None,
    Player,
    NPC,
    DeadBody,
    GroundItem,
    DynamicObject
};

//=============================================================================
// Focused Object Information
//=============================================================================
struct FocusedObject {
    bool valid = false;

    // Identification
    uint16_t objectID = 0;
    short mapX = 0, mapY = 0;
    short screenX = 0, screenY = 0;
    short dataX = 0, dataY = 0;  // Map data array indices

    // Type info
    FocusedObjectType type = FocusedObjectType::None;
    short ownerType = 0;
    char action = 0;
    char direction = 0;
    char frame = 0;

    // Display info
    char name[12] = {0};
    short appr1 = 0, appr2 = 0, appr3 = 0, appr4 = 0;
    int apprColor = 0;
    int status = 0;

    // Query helpers
    bool IsHostile(int (*foeFunc)(int)) const {
        return foeFunc(status) < 0;
    }
    bool IsDead() const {
        return type == FocusedObjectType::DeadBody;
    }
};

//=============================================================================
// Object Info for Testing (passed during iteration)
//=============================================================================
struct PickingObjectInfo {
    uint16_t objectID;
    short mapX, mapY;
    short screenX, screenY;
    short dataX, dataY;  // Map data array indices
    short ownerType;
    char action, direction, frame;
    const char* name;  // Points to existing string, no copy
    short appr1, appr2, appr3, appr4;
    int apprColor;
    int status;
    FocusedObjectType type;
};

//=============================================================================
// Picking System API
//=============================================================================
namespace Picking {
    //-------------------------------------------------------------------------
    // Frame Lifecycle
    //-------------------------------------------------------------------------
    // Call at start of DrawObjects() to reset state
    void BeginFrame();

    // Call at end of DrawObjects() to finalize cursor type
    // foeResult: result of _iGetFOE(focusedStatus), <0 means hostile (or 0 if no focus)
    // commandType: m_iPointCommandType
    // commandAvailable: m_bCommandAvailable
    // isGetPointingMode: m_bIsGetPointingMode
    void EndFrame(int foeResult, int commandType, bool commandAvailable, bool isGetPointingMode);

    //-------------------------------------------------------------------------
    // Object Testing (called during DrawObjects iteration)
    //-------------------------------------------------------------------------
    // Test if mouse is over object's bounding rect
    // screenY used for depth sorting (lower Y = further back)
    // maxScreenY: bottom boundary for valid picking (typically LOGICAL_HEIGHT() - 49)
    void TestObject(const SpriteLib::BoundRect& bounds, const PickingObjectInfo& info, int screenY, int maxScreenY);

    // Test ground item with circular proximity (13px radius)
    void TestGroundItem(int screenX, int screenY, int maxScreenY);

    // Test dynamic object (minerals, etc)
    void TestDynamicObject(const SpriteLib::BoundRect& bounds, short mapX, short mapY, int maxScreenY);

    //-------------------------------------------------------------------------
    // Query Functions
    //-------------------------------------------------------------------------
    const FocusedObject& GetFocusedObject();
    bool HasFocusedObject();

    CursorType GetCursorType();
    int GetCursorFrame();  // Returns (int)CursorType for compatibility

    // Map coordinates (for m_sMCX/m_sMCY compatibility)
    short GetFocusedMapX();
    short GetFocusedMapY();
    const char* GetFocusedName();  // Returns pointer to internal name buffer

    // Ground item hover state
    bool IsOverGroundItem();

    //-------------------------------------------------------------------------
    // Focus Highlight Data (for redrawing focused object)
    //-------------------------------------------------------------------------
    // Returns full appearance data needed to redraw focused object
    // with transparency highlight
    bool GetFocusHighlightData(
        short& outScreenX, short& outScreenY,
        uint16_t& outObjectID,
        short& outOwnerType, char& outAction, char& outDir, char& outFrame,
        short& outAppr1, short& outAppr2, short& outAppr3, short& outAppr4,
        int& outApprColor, int& outStatus,
        short& outDataX, short& outDataY
    );

    // Get focus status for FOE calculation
    int GetFocusStatus();

    //-------------------------------------------------------------------------
    // Utilities
    //-------------------------------------------------------------------------
    bool PointInRect(int x, int y, const SpriteLib::BoundRect& rect);
    bool PointInCircle(int x, int y, int cx, int cy, int radius);
}

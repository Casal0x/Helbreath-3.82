// Picking.cpp: Object picking and cursor management implementation
//
//////////////////////////////////////////////////////////////////////

#include "Picking.h"
#include "IInput.h"
#include <cstring>

// For GetTickCount()
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

// Internal state (static, not exposed)
namespace {
    // Mouse position (cached from Input:: at BeginFrame)
    int s_mouseX = 0;
    int s_mouseY = 0;

    // Current frame's focused object
    FocusedObject s_focusedObject;
    int s_bestHitY = -99999;  // Depth sorting - higher Y = closer

    // Ground item state
    bool s_overGroundItem = false;

    // Cursor state
    CursorType s_cursorType = CursorType::Arrow;

    // Item cursor animation
    uint32_t s_itemAnimTime = 0;
    int s_itemAnimFrame = 1;
}

//-----------------------------------------------------------------------------
// Frame Lifecycle
//-----------------------------------------------------------------------------

void Picking::BeginFrame()
{
    // Cache mouse position
    s_mouseX = Input::GetMouseX();
    s_mouseY = Input::GetMouseY();

    // Reset focus state
    s_focusedObject = FocusedObject{};
    s_bestHitY = -99999;

    // Reset ground item state
    s_overGroundItem = false;

    // Reset cursor (will be determined in EndFrame)
    s_cursorType = CursorType::Arrow;
}

void Picking::EndFrame(int foeResult, int commandType, bool commandAvailable, bool isGetPointingMode)
{
    uint32_t now = GetTickCount();

    // Ground item takes priority for cursor animation
    if (s_overGroundItem) {
        // Animate every 200ms
        if (now - s_itemAnimTime > 200) {
            s_itemAnimTime = now;
            s_itemAnimFrame = (s_itemAnimFrame == 1) ? 2 : 1;
        }
        s_cursorType = (s_itemAnimFrame == 1) ?
            CursorType::ItemGround1 : CursorType::ItemGround2;
        return;
    }

    // Check pointing mode for spell/item targeting
    if (isGetPointingMode) {
        // Spell targeting mode (100-199)
        if (commandType >= 100 && commandType < 200) {
            if (commandAvailable) {
                if (s_focusedObject.valid && foeResult < 0)
                    s_cursorType = CursorType::SpellHostile;
                else
                    s_cursorType = CursorType::SpellFriendly;
            } else {
                s_cursorType = CursorType::Unavailable;
            }
            return;
        }

        // Item use mode (0-49)
        if (commandType >= 0 && commandType < 50) {
            s_cursorType = CursorType::ItemUse;
            return;
        }
    }

    // Normal mode - show target cursor based on focus
    if (s_focusedObject.valid) {
        if (foeResult < 0)
            s_cursorType = CursorType::TargetHostile;
        else
            s_cursorType = CursorType::TargetNeutral;
        return;
    }

    // Default
    s_cursorType = CursorType::Arrow;
}

//-----------------------------------------------------------------------------
// Object Testing
//-----------------------------------------------------------------------------

void Picking::TestObject(const SpriteLib::BoundRect& bounds, const PickingObjectInfo& info, int screenY, int maxScreenY)
{
    // Skip invalid bounds
    if (bounds.top == -1) return;

    // Skip if mouse is below the valid picking area (UI region)
    if (s_mouseY > maxScreenY) return;

    // Hit test - check if mouse is within sprite bounds
    // Note: Original code used < and > exclusively, not <= and >=
    if (s_mouseX > bounds.left && s_mouseX < bounds.right &&
        s_mouseY > bounds.top && s_mouseY < bounds.bottom) {

        // Last valid hit wins (tiles are processed back-to-front)
        // Copy info to focused object
        s_focusedObject.valid = true;
        s_focusedObject.objectID = info.objectID;
        s_focusedObject.mapX = info.mapX;
        s_focusedObject.mapY = info.mapY;
        s_focusedObject.screenX = info.screenX;
        s_focusedObject.screenY = info.screenY;
        s_focusedObject.dataX = info.dataX;
        s_focusedObject.dataY = info.dataY;
        s_focusedObject.ownerType = info.ownerType;
        s_focusedObject.type = info.type;
        s_focusedObject.action = info.action;
        s_focusedObject.direction = info.direction;
        s_focusedObject.frame = info.frame;
        s_focusedObject.appr1 = info.appr1;
        s_focusedObject.appr2 = info.appr2;
        s_focusedObject.appr3 = info.appr3;
        s_focusedObject.appr4 = info.appr4;
        s_focusedObject.apprColor = info.apprColor;
        s_focusedObject.status = info.status;

        // Copy name
        std::memset(s_focusedObject.name, 0, sizeof(s_focusedObject.name));
        if (info.name) {
            std::strncpy(s_focusedObject.name, info.name, sizeof(s_focusedObject.name) - 1);
        }
    }
}

void Picking::TestGroundItem(int screenX, int screenY, int maxScreenY)
{
    // Skip if mouse is below the valid picking area
    if (s_mouseY > maxScreenY) return;

    // Check circular proximity (13px radius)
    if (PointInCircle(s_mouseX, s_mouseY, screenX, screenY, 13)) {
        s_overGroundItem = true;
    }
}

void Picking::TestDynamicObject(const SpriteLib::BoundRect& bounds, short mapX, short mapY, int maxScreenY)
{
    // Skip invalid bounds
    if (bounds.top == -1) return;

    // Skip if mouse is below the valid picking area
    if (s_mouseY > maxScreenY) return;

    // Hit test
    if (s_mouseX >= bounds.left && s_mouseX < bounds.right &&
        s_mouseY >= bounds.top && s_mouseY < bounds.bottom) {

        // Dynamic objects (minerals) set focus without full object info
        s_focusedObject.valid = true;
        s_focusedObject.mapX = mapX;
        s_focusedObject.mapY = mapY;
        s_focusedObject.type = FocusedObjectType::DynamicObject;
        s_focusedObject.status = 0;
        std::memset(s_focusedObject.name, 0, sizeof(s_focusedObject.name));
    }
}

//-----------------------------------------------------------------------------
// Query Functions
//-----------------------------------------------------------------------------

const FocusedObject& Picking::GetFocusedObject()
{
    return s_focusedObject;
}

bool Picking::HasFocusedObject()
{
    return s_focusedObject.valid;
}

CursorType Picking::GetCursorType()
{
    return s_cursorType;
}

int Picking::GetCursorFrame()
{
    return static_cast<int>(s_cursorType);
}

short Picking::GetFocusedMapX()
{
    return s_focusedObject.valid ? s_focusedObject.mapX : 0;
}

short Picking::GetFocusedMapY()
{
    return s_focusedObject.valid ? s_focusedObject.mapY : 0;
}

const char* Picking::GetFocusedName()
{
    return s_focusedObject.name;
}

bool Picking::IsOverGroundItem()
{
    return s_overGroundItem;
}

int Picking::GetFocusStatus()
{
    return s_focusedObject.status;
}

//-----------------------------------------------------------------------------
// Focus Highlight Data
//-----------------------------------------------------------------------------

bool Picking::GetFocusHighlightData(
    short& outScreenX, short& outScreenY,
    uint16_t& outObjectID,
    short& outOwnerType, char& outAction, char& outDir, char& outFrame,
    short& outAppr1, short& outAppr2, short& outAppr3, short& outAppr4,
    int& outApprColor, int& outStatus,
    short& outDataX, short& outDataY)
{
    if (!s_focusedObject.valid) {
        return false;
    }

    outScreenX = s_focusedObject.screenX;
    outScreenY = s_focusedObject.screenY;
    outObjectID = s_focusedObject.objectID;
    outOwnerType = s_focusedObject.ownerType;
    outAction = s_focusedObject.action;
    outDir = s_focusedObject.direction;
    outFrame = s_focusedObject.frame;
    outAppr1 = s_focusedObject.appr1;
    outAppr2 = s_focusedObject.appr2;
    outAppr3 = s_focusedObject.appr3;
    outAppr4 = s_focusedObject.appr4;
    outApprColor = s_focusedObject.apprColor;
    outStatus = s_focusedObject.status;
    outDataX = s_focusedObject.dataX;
    outDataY = s_focusedObject.dataY;

    return true;
}

//-----------------------------------------------------------------------------
// Utilities
//-----------------------------------------------------------------------------

bool Picking::PointInRect(int x, int y, const SpriteLib::BoundRect& rect)
{
    return x >= rect.left && x < rect.right &&
           y >= rect.top && y < rect.bottom;
}

bool Picking::PointInCircle(int x, int y, int cx, int cy, int radius)
{
    int dx = x - cx;
    int dy = y - cy;
    return (dx * dx + dy * dy) <= (radius * radius);
}

// Camera.h: Game camera/viewport management
//
// Handles smooth camera movement, tracking player position, and viewport calculations.
// The camera smoothly interpolates towards its target destination with deceleration.
//
//////////////////////////////////////////////////////////////////////

#pragma once

#include <cstdint>
#include <cstdlib>

class CCamera
{
public:
    CCamera();

    // Reset camera to initial state
    void Reset();

    // Update camera position (smooth interpolation towards destination)
    // Call this once per frame
    void Update();

    //------------------------------------------------------------------
    // Position Accessors
    //------------------------------------------------------------------

    // Get current camera position in world pixels
    int GetX() const { return m_iPositionX; }
    int GetY() const { return m_iPositionY; }

    // Get target destination in world pixels
    int GetDestinationX() const { return m_iDestinationX; }
    int GetDestinationY() const { return m_iDestinationY; }

    //------------------------------------------------------------------
    // Position Modifiers
    //------------------------------------------------------------------

    // Snap camera immediately to position (no interpolation)
    void SetPosition(int x, int y);

    // Set target destination (camera will smoothly move there)
    void SetDestination(int x, int y);

    // Snap both position and destination (teleport)
    void SnapTo(int x, int y);

    // Move destination by delta (relative movement)
    void MoveDestination(int dx, int dy);

    //------------------------------------------------------------------
    // Utility Methods
    //------------------------------------------------------------------

    // Center camera on a tile position
    // viewCenterTileX/Y are the center offsets (typically VIEW_CENTER_TILE_X/Y)
    void CenterOnTile(int tileX, int tileY, int viewCenterTileX, int viewCenterTileY);

    // Convert world coordinates to screen coordinates
    int WorldToScreenX(int worldX) const { return worldX - m_iPositionX; }
    int WorldToScreenY(int worldY) const { return worldY - m_iPositionY; }

    // Convert screen coordinates to world coordinates
    int ScreenToWorldX(int screenX) const { return screenX + m_iPositionX; }
    int ScreenToWorldY(int screenY) const { return screenY + m_iPositionY; }

    //------------------------------------------------------------------
    // Camera Effects
    //------------------------------------------------------------------

    // Set shake intensity (will decrement over time)
    void SetShake(int degree) { m_iShakeDegree = degree; }

    // Get current shake degree
    int GetShakeDegree() const { return m_iShakeDegree; }

    // Apply and update camera shake effect
    // Saves position, applies random shake based on current degree, then decrements degree
    // Returns true if shake was applied (caller should RestorePosition after rendering)
    bool ApplyShake();

    // Save current position (for shake effect)
    void SavePosition();

    // Restore saved position (after shake effect)
    void RestorePosition();

private:
    // Current camera position in world pixels
    int m_iPositionX;
    int m_iPositionY;

    // Target destination in world pixels
    int m_iDestinationX;
    int m_iDestinationY;

    // Movement velocity for smooth interpolation
    int m_iVelocityX;
    int m_iVelocityY;

    // Saved position for shake effect restoration
    int m_iSavedPositionX;
    int m_iSavedPositionY;

    // Camera shake degree (decrements each frame)
    int m_iShakeDegree;
};

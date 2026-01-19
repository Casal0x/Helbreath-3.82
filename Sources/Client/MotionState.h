#pragma once

#include "CommonTypes.h"
#include <unordered_map>
#include <vector>
#include <cmath>

struct MotionState {
    float fOffsetX;
    float fOffsetY;
    float fStartOffsetX;
    float fStartOffsetY;
    float fTargetOffsetX;
    float fTargetOffsetY;
    
    // Timing
    uint32_t dwStartTime;
    uint32_t dwDuration;
    
    bool bIsMoving;

    MotionState() {
        Reset();
    }

    void Reset() {
        fOffsetX = 0.0f;
        fOffsetY = 0.0f;
        fStartOffsetX = 0.0f;
        fStartOffsetY = 0.0f;
        fTargetOffsetX = 0.0f;
        fTargetOffsetY = 0.0f;
        dwStartTime = 0;
        dwDuration = 0;
        bIsMoving = false;
    }

    // Initialize a move from a neighbor tile TO the center (0,0)
    // If moving South (increasing Y), we visually start from North (-32 Y) relative to new tile center
    void StartMove(int iDir, uint32_t dwTime, uint32_t dwMoveDuration = 400) {
        // Directions: 1:Up, 2:Right-Up, 3:Right, 4:Right-Down, 5:Down, 6:Left-Down, 7:Left, 8:Left-Up
        // TILE_SIZE is 32x32 typically.
        constexpr float TILE_SIZE = 32.0f;

        // Reset target to center (we are "sliding in" to the new tile)
        fTargetOffsetX = 0.0f;
        fTargetOffsetY = 0.0f;

        // Calculate start offset based on direction we came FROM
        // e.g. if we move UP (Dir 1), we came from DOWN (positive Y offset)
        switch (iDir) {
        case 1: fStartOffsetX = 0.0f;       fStartOffsetY = TILE_SIZE;  break; // Up
        case 2: fStartOffsetX = -TILE_SIZE; fStartOffsetY = TILE_SIZE;  break; // Right-Up
        case 3: fStartOffsetX = -TILE_SIZE; fStartOffsetY = 0.0f;       break; // Right
        case 4: fStartOffsetX = -TILE_SIZE; fStartOffsetY = -TILE_SIZE; break; // Right-Down
        case 5: fStartOffsetX = 0.0f;       fStartOffsetY = -TILE_SIZE; break; // Down
        case 6: fStartOffsetX = TILE_SIZE;  fStartOffsetY = -TILE_SIZE; break; // Left-Down
        case 7: fStartOffsetX = TILE_SIZE;  fStartOffsetY = 0.0f;       break; // Left
        case 8: fStartOffsetX = TILE_SIZE;  fStartOffsetY = TILE_SIZE;  break; // Left-Up
        }

        fOffsetX = fStartOffsetX;
        fOffsetY = fStartOffsetY;

        dwStartTime = dwTime;
        dwDuration = dwMoveDuration;
        bIsMoving = true;
    }

    void Update(uint32_t dwCurrentTime) {
        if (!bIsMoving) return;

        if (dwCurrentTime >= dwStartTime + dwDuration) {
            // Finished
            fOffsetX = fTargetOffsetX;
            fOffsetY = fTargetOffsetY;
            bIsMoving = false;
        }
        else {
            // Interpolate
            float t = (float)(dwCurrentTime - dwStartTime) / (float)dwDuration;
            
            // Linear Interpolation (Lerp)
            fOffsetX = fStartOffsetX + (fTargetOffsetX - fStartOffsetX) * t;
            fOffsetY = fStartOffsetY + (fTargetOffsetY - fStartOffsetY) * t;
        }
    }
};

class EntityMotionManager {
public:
    // Map of EntityID -> MotionState
    std::unordered_map<uint16_t, MotionState> m_EntityMotions;

    void StartEntityMove(uint16_t wObjectID, int iDir, uint32_t dwStartTime, uint32_t dwDuration) {
        m_EntityMotions[wObjectID].StartMove(iDir, dwStartTime, dwDuration);
    }

    void UpdateMovingEntities(uint32_t dwCurrentTime) {
        // Iterate only through existing states
        // In a very large world with many static entities, this map should only contain active ones ideally,
        // but explicit cleanup might be needed if map grows too large. 
        // For v4, we can keep it simple or remove finished ones.
        for (auto& pair : m_EntityMotions) {
            pair.second.Update(dwCurrentTime);
        }
    }

    void GetEntityOffset(uint16_t wObjectID, float& outX, float& outY) {
        auto it = m_EntityMotions.find(wObjectID);
        if (it != m_EntityMotions.end() && it->second.bIsMoving) {
            outX = it->second.fOffsetX;
            outY = it->second.fOffsetY;
        }
        else {
            outX = 0.0f;
            outY = 0.0f;
        }
    }

    void StopEntity(uint16_t wObjectID) {
        auto it = m_EntityMotions.find(wObjectID);
        if (it != m_EntityMotions.end()) {
            it->second.bIsMoving = false;
            it->second.fOffsetX = 0.0f;
            it->second.fOffsetY = 0.0f;
        }
    }

    void RemoveEntity(uint16_t wObjectID) {
        m_EntityMotions.erase(wObjectID);
    }
    
    void ClearAll() {
        m_EntityMotions.clear();
    }
};

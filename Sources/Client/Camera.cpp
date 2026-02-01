// Camera.cpp: Game camera/viewport management implementation
//
//////////////////////////////////////////////////////////////////////

#include "Camera.h"

CCamera::CCamera()
{
    Reset();
}

void CCamera::Reset()
{
    m_iPositionX = 0;
    m_iPositionY = 0;
    m_iDestinationX = 0;
    m_iDestinationY = 0;
    m_dwLastUpdateTime = 0;
    m_iSavedPositionX = 0;
    m_iSavedPositionY = 0;
    m_iShakeDegree = 0;
}

void CCamera::Update(uint32_t currentTime)
{
    int dx = m_iDestinationX - m_iPositionX;
    int dy = m_iDestinationY - m_iPositionY;

    // Already at destination
    if (dx == 0 && dy == 0)
    {
        m_dwLastUpdateTime = currentTime;
        return;
    }

    float dt = (currentTime - m_dwLastUpdateTime) * 0.001f;
    m_dwLastUpdateTime = currentTime;

    // First frame or large time gap — snap immediately
    if (dt <= 0.0f || dt > 0.5f)
    {
        m_iPositionX = m_iDestinationX;
        m_iPositionY = m_iDestinationY;
        return;
    }

    // Large distance (teleport) — snap immediately
    if (abs(dx) > 128 || abs(dy) > 128)
    {
        m_iPositionX = m_iDestinationX;
        m_iPositionY = m_iDestinationY;
        return;
    }

    // Exponential lerp: fast tracking with smooth micro-gap bridging
    constexpr float SMOOTH_SPEED = 18.0f;
    float t = 1.0f - expf(-SMOOTH_SPEED * dt);

    int moveX = static_cast<int>(dx * t);
    int moveY = static_cast<int>(dy * t);

    // Snap when sub-pixel close to avoid never arriving
    if (moveX == 0 && moveY == 0)
    {
        m_iPositionX = m_iDestinationX;
        m_iPositionY = m_iDestinationY;
    }
    else
    {
        m_iPositionX += moveX;
        m_iPositionY += moveY;
    }
}

void CCamera::SetPosition(int x, int y)
{
    m_iPositionX = x;
    m_iPositionY = y;
}

void CCamera::SetDestination(int x, int y)
{
    m_iDestinationX = x;
    m_iDestinationY = y;
}

void CCamera::SnapTo(int x, int y)
{
    m_iPositionX = x;
    m_iPositionY = y;
    m_iDestinationX = x;
    m_iDestinationY = y;
    m_dwLastUpdateTime = 0;
}

void CCamera::MoveDestination(int dx, int dy)
{
    m_iDestinationX += dx;
    m_iDestinationY += dy;
}

void CCamera::CenterOnTile(int tileX, int tileY, int viewCenterTileX, int viewCenterTileY)
{
    int x = (tileX - viewCenterTileX) * 32;
    int y = (tileY - (viewCenterTileY + 1)) * 32;
    SnapTo(x, y);
}

bool CCamera::ApplyShake()
{
    if (m_iShakeDegree > 0)
    {
        m_iPositionX += m_iShakeDegree - (rand() % (m_iShakeDegree * 2));
        m_iPositionY += m_iShakeDegree - (rand() % (m_iShakeDegree * 2));
        m_iShakeDegree--;
        return true;
    }
    return false;
}

void CCamera::SavePosition()
{
    m_iSavedPositionX = m_iPositionX;
    m_iSavedPositionY = m_iPositionY;
}

void CCamera::RestorePosition()
{
    m_iPositionX = m_iSavedPositionX;
    m_iPositionY = m_iSavedPositionY;
}

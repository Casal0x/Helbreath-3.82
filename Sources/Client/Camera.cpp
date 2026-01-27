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
    m_iVelocityX = 0;
    m_iVelocityY = 0;
    m_iSavedPositionX = 0;
    m_iSavedPositionY = 0;
    m_iShakeDegree = 0;
}

void CCamera::Update()
{
    // Calculate distance to destination
    int deltaX = m_iPositionX - m_iDestinationX;
    int deltaY = m_iPositionY - m_iDestinationY;

    // X-axis interpolation
    if (abs(deltaX) < abs(m_iVelocityX))
    {
        // Close enough - snap to destination
        m_iPositionX = m_iDestinationX;
        m_iVelocityX = 0;
    }
    else
    {
        // Adjust velocity based on direction
        if (deltaX > 0) m_iVelocityX--;
        else if (deltaX < 0) m_iVelocityX++;
        else m_iVelocityX = 0;

        // Limit velocity when close to destination (deceleration)
        if (abs(deltaX) < 40)
        {
            if (m_iVelocityX > 2) m_iVelocityX = 2;
            else if (m_iVelocityX < -2) m_iVelocityX = -2;
        }

        m_iPositionX += m_iVelocityX;
    }

    // Y-axis interpolation
    if (abs(deltaY) < abs(m_iVelocityY))
    {
        // Close enough - snap to destination
        m_iPositionY = m_iDestinationY;
        m_iVelocityY = 0;
    }
    else
    {
        // Adjust velocity based on direction
        if (deltaY > 0) m_iVelocityY--;
        else if (deltaY < 0) m_iVelocityY++;
        else m_iVelocityY = 0;

        // Limit velocity when close to destination (deceleration)
        if (abs(deltaY) < 40)
        {
            if (m_iVelocityY > 2) m_iVelocityY = 2;
            else if (m_iVelocityY < -2) m_iVelocityY = -2;
        }

        m_iPositionY += m_iVelocityY;
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
    m_iVelocityX = 0;
    m_iVelocityY = 0;
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

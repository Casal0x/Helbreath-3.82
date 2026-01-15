// DXC_dinput.cpp: implementation of the DXC_dinput class.
//
//////////////////////////////////////////////////////////////////////

#include "DXC_dinput.h"
#include "GlobalDef.h"
#include <cstdio>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

DXC_dinput::DXC_dinput()
{
	m_pDI    = 0;
	m_pMouse = 0;
	m_hWnd   = 0;
	m_sX     = 0;
	m_sY     = 0;
	m_sZ     = 0;
}

DXC_dinput::~DXC_dinput()
{
	if (m_pMouse != 0) {
		m_pMouse->Unacquire();
        m_pMouse->Release();
        m_pMouse = 0;
	}
	if (m_pDI != 0) {
		m_pDI->Release();
        m_pDI = 0;
	}
}

bool DXC_dinput::bInit(HWND hWnd, HINSTANCE hInst)
{
 HRESULT hr;
 DIMOUSESTATE dims;

	m_hWnd = hWnd;
	m_sX   = 400;
	m_sY   = 300;

	hr = DirectInputCreate( hInst, DIRECTINPUT_VERSION, &m_pDI, 0 );
    if (hr != DI_OK) return false;
	hr = m_pDI->CreateDevice( GUID_SysMouse, &m_pMouse, 0 );
	if (hr != DI_OK) return false;
	hr = m_pMouse->SetDataFormat( &c_dfDIMouse );
	if (hr != DI_OK) return false;
	hr = m_pMouse->SetCooperativeLevel( hWnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
	if (hr != DI_OK) return false;

	if ( m_pMouse->GetDeviceState( sizeof(DIMOUSESTATE), &dims ) != DI_OK )
	{
		m_pMouse->Acquire();
	}

	// Clip cursor to window on initialization
	RECT rcClient;
	GetClientRect(m_hWnd, &rcClient);
	POINT ptTopLeft = {rcClient.left, rcClient.top};
	POINT ptBottomRight = {rcClient.right, rcClient.bottom};
	ClientToScreen(m_hWnd, &ptTopLeft);
	ClientToScreen(m_hWnd, &ptBottomRight);
	RECT rcClip = {ptTopLeft.x, ptTopLeft.y, ptBottomRight.x, ptBottomRight.y};
	ClipCursor(&rcClip);

	return true;
}


void DXC_dinput::SetAcquire(bool bFlag)
{
 DIMOUSESTATE dims;

	if (m_pMouse == 0) return;
	if (bFlag == true) {
		m_pMouse->Acquire();
		m_pMouse->GetDeviceState( sizeof(DIMOUSESTATE), &dims );

		// Clip cursor to window client area to prevent it from leaving
		if (m_hWnd) {
			RECT rcClient;
			GetClientRect(m_hWnd, &rcClient);
			POINT ptTopLeft = {rcClient.left, rcClient.top};
			POINT ptBottomRight = {rcClient.right, rcClient.bottom};
			ClientToScreen(m_hWnd, &ptTopLeft);
			ClientToScreen(m_hWnd, &ptBottomRight);
			RECT rcClip = {ptTopLeft.x, ptTopLeft.y, ptBottomRight.x, ptBottomRight.y};
			ClipCursor(&rcClip);
		}
	}
	else {
		m_pMouse->Unacquire();
		// Release cursor clip when losing focus
		ClipCursor(NULL);
	}
}

void DXC_dinput::UpdateMouseState(short * pX, short * pY, short * pZ, char * pLB, char * pRB)
{
	// Get button states from DirectInput
	if ( m_pMouse->GetDeviceState( sizeof(DIMOUSESTATE), &dims ) != DI_OK )
	{
		m_pMouse->Acquire();
		// Clear wheel delta on failure to prevent stale values causing endless scrolling
		*pZ = 0;
		return;
	}

	// In windowed NONEXCLUSIVE mode, DirectInput position tracking doesn't work reliably on multi-monitor setups
	// Use Windows cursor position instead and convert to window-relative coordinates
	POINT pt;
	GetCursorPos(&pt);
	ScreenToClient(m_hWnd, &pt);

	// Map window coordinates to logical render space, respecting letterbox
	RECT rcClient;
	GetClientRect(m_hWnd, &rcClient);
	int winW = rcClient.right - rcClient.left;
	int winH = rcClient.bottom - rcClient.top;
	double scale = (double)winW / (double)LOGICAL_WIDTH;
	double scaleY = (double)winH / (double)LOGICAL_HEIGHT;
	if (scaleY < scale) scale = scaleY;
	if (scale <= 0.0) scale = 1.0;
	int destW = (int)(LOGICAL_WIDTH * scale);
	int destH = (int)(LOGICAL_HEIGHT * scale);
	int offsetX = (winW - destW) / 2;
	int offsetY = (winH - destH) / 2;

	long scaledX = (long)((pt.x - offsetX) / scale);
	long scaledY = (long)((pt.y - offsetY) / scale);
	m_sX = (short)scaledX;
	m_sY = (short)scaledY;

	// Handle mouse wheel from DirectInput
	// DirectInput may not reset lZ to 0 between frames, so track previous value
	// Only update m_sZ when dims.lZ changes (indicating new input)
	static short s_sPrevDimsZ = 0;
	short sCurrentZ = (short)dims.lZ;
	if (sCurrentZ != s_sPrevDimsZ)
	{
		// New wheel input detected - calculate delta from previous
		m_sZ = sCurrentZ - s_sPrevDimsZ;
		s_sPrevDimsZ = sCurrentZ;
	}

	// Clamp to game bounds
	if (m_sX < 0) m_sX = 0;
	if (m_sY < 0) m_sY = 0;
	if (m_sX > LOGICAL_MAX_X) m_sX = LOGICAL_MAX_X;
	if (m_sY > LOGICAL_MAX_Y) m_sY = LOGICAL_MAX_Y;

	*pX = m_sX;
	*pY = m_sY;
	*pZ = m_sZ;
	*pLB = (char)dims.rgbButtons[0];
	*pRB = (char)dims.rgbButtons[1];
}

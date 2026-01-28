// Mydib.h: BMP file loading for sprite surfaces
//
// Part of DDrawEngine static library
//////////////////////////////////////////////////////////////////////

#pragma once

#include <windows.h>
#include <winbase.h>

class CMyDib
{
public:
	CMyDib(char *szFilename, unsigned long dwFilePointer);
	~CMyDib();
	void PaintImage(HDC hDC);
	WORD m_wWidthX;
	WORD m_wWidthY;
	WORD m_wColorNums;//bmp
	WORD m_wMaskSize; // BI_BITFIELDS RGB mask size (12 bytes if present, 0 otherwise)
	LPSTR m_lpDib;
	LPBITMAPINFO m_bmpInfo; //bmp
};

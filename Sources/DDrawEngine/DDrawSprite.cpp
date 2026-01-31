// DDrawSprite.cpp: DirectDraw implementation of ISprite
//
//////////////////////////////////////////////////////////////////////

#include "DDrawSprite.h"
#include "CommonTypes.h"
#include <cstring>
#include <algorithm>

// External transparency/blend tables from DXC_ddraw.cpp
extern char G_cSpriteAlphaDegree;
extern int G_iAddTable31[64][510], G_iAddTable63[64][510];
extern int G_iAddTransTable31[510][64], G_iAddTransTable63[510][64];
extern long G_lTransG100[64][64], G_lTransRB100[64][64];
extern long G_lTransG70[64][64], G_lTransRB70[64][64];
extern long G_lTransG50[64][64], G_lTransRB50[64][64];
extern long G_lTransG25[64][64], G_lTransRB25[64][64];
extern long G_lTransG2[64][64], G_lTransRB2[64][64];

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

DDrawSprite::DDrawSprite(DXC_ddraw* pDDraw, const std::string& pakFilePath, int spriteIndex, bool alphaEffect)
    : m_pDDraw(pDDraw)
    , m_pakFilePath(pakFilePath)
    , m_spriteIndex(spriteIndex)
    , m_lpSurface(nullptr)
    , m_pSurfaceAddr(nullptr)
    , m_sPitch(0)
    , m_wColorKey(0)
    , m_wBitmapSizeX(0)
    , m_wBitmapSizeY(0)
    , m_bSurfaceLoaded(false)
    , m_bAlphaEffect(alphaEffect)
    , m_cAlphaDegree(1)
    , m_bOnCriticalSection(false)
    , m_sPivotX(0)
    , m_sPivotY(0)
    , m_dwRefTime(0)
{
    std::memset(&m_rcBound, 0, sizeof(m_rcBound));
    m_rcBound.top = -1;

    // Load sprite data from PAK using PAKLib
    try {
        PAKLib::sprite spriteData = PAKLib::get_sprite_fast(pakFilePath, spriteIndex);

        // Copy frame rectangles
        m_frames = std::move(spriteData.sprite_rectangles);

        // Keep image data for surface creation
        m_imageData = std::move(spriteData.image_data);
    }
    catch (const std::exception&) {
        // Failed to load sprite - leave empty
        m_frames.clear();
        m_imageData.clear();
    }
}

// Constructor from pre-loaded PAK sprite data (no file I/O)
DDrawSprite::DDrawSprite(DXC_ddraw* pDDraw, const PAKLib::sprite& spriteData, bool alphaEffect)
    : m_pDDraw(pDDraw)
    , m_pakFilePath()
    , m_spriteIndex(-1)
    , m_lpSurface(nullptr)
    , m_pSurfaceAddr(nullptr)
    , m_sPitch(0)
    , m_wColorKey(0)
    , m_wBitmapSizeX(0)
    , m_wBitmapSizeY(0)
    , m_bSurfaceLoaded(false)
    , m_bAlphaEffect(alphaEffect)
    , m_cAlphaDegree(1)
    , m_bOnCriticalSection(false)
    , m_sPivotX(0)
    , m_sPivotY(0)
    , m_dwRefTime(0)
{
    std::memset(&m_rcBound, 0, sizeof(m_rcBound));
    m_rcBound.top = -1;

    // Copy frame rectangles from pre-loaded data
    m_frames = spriteData.sprite_rectangles;

    // Copy image data for surface creation
    m_imageData = spriteData.image_data;
}

DDrawSprite::~DDrawSprite()
{
    Unload();
}

//////////////////////////////////////////////////////////////////////
// ISprite Implementation - Resource Management
//////////////////////////////////////////////////////////////////////

void DDrawSprite::Preload()
{
    if (m_bSurfaceLoaded || m_imageData.empty()) {
        return;
    }
    CreateSurface();
}

void DDrawSprite::Unload()
{
    ReleaseSurface();
}

bool DDrawSprite::IsLoaded() const
{
    return m_bSurfaceLoaded && m_lpSurface != nullptr;
}

void DDrawSprite::Restore()
{
    if (!m_bSurfaceLoaded || m_lpSurface == nullptr) {
        return;
    }

    if (m_lpSurface->IsLost() == DD_OK) {
        return;
    }

    // Surface was lost, recreate it
    m_lpSurface->Restore();

    // Repaint the image data
    if (!m_imageData.empty()) {
        // Parse BMP header from image data
        if (m_imageData.size() < 14) return;

        BITMAPFILEHEADER* pFileHeader = reinterpret_cast<BITMAPFILEHEADER*>(m_imageData.data());
        LPBITMAPINFO pBmpInfo = reinterpret_cast<LPBITMAPINFO>(m_imageData.data() + 14);
        LPBITMAPINFOHEADER pInfoHeader = &pBmpInfo->bmiHeader;

        WORD colorNums = 0;
        if (pInfoHeader->biClrUsed == 0) {
            if (pInfoHeader->biBitCount == 24) colorNums = 0;
            else if (pInfoHeader->biBitCount == 8) colorNums = 256;
            else if (pInfoHeader->biBitCount == 1) colorNums = 2;
            else if (pInfoHeader->biBitCount == 4) colorNums = 16;
        }
        else {
            colorNums = static_cast<WORD>(pInfoHeader->biClrUsed);
        }

        // BI_BITFIELDS (3) has 12 bytes of RGB masks after header
        WORD maskSize = 0;
        if (pInfoHeader->biCompression == BI_BITFIELDS)
            maskSize = 12;

        HDC hDC;
        if (m_lpSurface->GetDC(&hDC) == DD_OK) {
            LPSTR lpBits = reinterpret_cast<LPSTR>(m_imageData.data() + 14 + pInfoHeader->biSize + maskSize + 4 * colorNums);
            SetDIBitsToDevice(hDC, 0, 0, m_wBitmapSizeX, m_wBitmapSizeY,
                              0, 0, 0, m_wBitmapSizeY, lpBits, pBmpInfo, DIB_RGB_COLORS);
            m_lpSurface->ReleaseDC(hDC);
        }
    }

    // Update surface address
    DDSURFACEDESC2 ddsd;
    ddsd.dwSize = sizeof(DDSURFACEDESC2);
    if (m_lpSurface->Lock(nullptr, &ddsd, DDLOCK_WAIT, nullptr) == DD_OK) {
        m_pSurfaceAddr = reinterpret_cast<uint16_t*>(ddsd.lpSurface);
        m_lpSurface->Unlock(nullptr);
    }
}

bool DDrawSprite::IsInUse() const
{
    return m_bOnCriticalSection;
}

uint32_t DDrawSprite::GetLastAccessTime() const
{
    return m_dwRefTime;
}

//////////////////////////////////////////////////////////////////////
// Surface Management
//////////////////////////////////////////////////////////////////////

bool DDrawSprite::CreateSurface()
{
    if (m_lpSurface != nullptr || m_imageData.empty()) {
        return false;
    }

    m_bOnCriticalSection = true;

    // Parse BMP header from image data
    if (m_imageData.size() < 14 + sizeof(BITMAPINFOHEADER)) {
        m_bOnCriticalSection = false;
        return false;
    }

    // Image data includes the full BMP file (header + DIB)
    BITMAPFILEHEADER* pFileHeader = reinterpret_cast<BITMAPFILEHEADER*>(m_imageData.data());
    LPBITMAPINFO pBmpInfo = reinterpret_cast<LPBITMAPINFO>(m_imageData.data() + 14);
    LPBITMAPINFOHEADER pInfoHeader = &pBmpInfo->bmiHeader;

    m_wBitmapSizeX = static_cast<uint16_t>(pInfoHeader->biWidth);
    m_wBitmapSizeY = static_cast<uint16_t>(pInfoHeader->biHeight);

    // Calculate color table size
    WORD colorNums = 0;
    if (pInfoHeader->biClrUsed == 0) {
        if (pInfoHeader->biBitCount == 24) colorNums = 0;
        else if (pInfoHeader->biBitCount == 8) colorNums = 256;
        else if (pInfoHeader->biBitCount == 1) colorNums = 2;
        else if (pInfoHeader->biBitCount == 4) colorNums = 16;
    }
    else {
        colorNums = static_cast<WORD>(pInfoHeader->biClrUsed);
    }

    // BI_BITFIELDS (3) has 12 bytes of RGB masks after header
    WORD maskSize = 0;
    if (pInfoHeader->biCompression == BI_BITFIELDS)
        maskSize = 12;

    // Create DDraw surface
    m_lpSurface = m_pDDraw->pCreateOffScreenSurface(m_wBitmapSizeX, m_wBitmapSizeY);
    if (m_lpSurface == nullptr) {
        m_bOnCriticalSection = false;
        return false;
    }

    // Paint the BMP to the surface
    HDC hDC;
    if (m_lpSurface->GetDC(&hDC) != DD_OK) {
        m_lpSurface->Release();
        m_lpSurface = nullptr;
        m_bOnCriticalSection = false;
        return false;
    }

    // Calculate pointer to actual bitmap bits
    LPSTR lpBits = reinterpret_cast<LPSTR>(m_imageData.data() + 14 + pInfoHeader->biSize + maskSize + 4 * colorNums);
    SetDIBitsToDevice(hDC, 0, 0, m_wBitmapSizeX, m_wBitmapSizeY,
                      0, 0, 0, m_wBitmapSizeY, lpBits, pBmpInfo, DIB_RGB_COLORS);
    m_lpSurface->ReleaseDC(hDC);

    // Get surface info and color key
    DDSURFACEDESC2 ddsd;
    ddsd.dwSize = sizeof(DDSURFACEDESC2);
    if (m_lpSurface->Lock(nullptr, &ddsd, DDLOCK_WAIT, nullptr) != DD_OK) {
        m_lpSurface->Release();
        m_lpSurface = nullptr;
        m_bOnCriticalSection = false;
        return false;
    }

    m_pSurfaceAddr = reinterpret_cast<uint16_t*>(ddsd.lpSurface);
    m_sPitch = static_cast<int16_t>(ddsd.lPitch >> 1);
    m_wColorKey = *m_pSurfaceAddr;  // Top-left pixel is the color key
    m_lpSurface->Unlock(nullptr);

    // Set the color key on the surface
    m_pDDraw->iSetColorKey(m_lpSurface, m_wColorKey);

    m_bSurfaceLoaded = true;
    m_bOnCriticalSection = false;

    // Apply current alpha degree
    ApplyAlphaDegree();

    return true;
}

void DDrawSprite::ReleaseSurface()
{
    if (m_lpSurface != nullptr) {
        if (m_lpSurface->IsLost() == DD_OK) {
            m_lpSurface->Release();
        }
        m_lpSurface = nullptr;
    }
    m_pSurfaceAddr = nullptr;
    m_bSurfaceLoaded = false;
    m_cAlphaDegree = 1;
}

void DDrawSprite::SetAlphaDegree(char degree)
{
    if (m_cAlphaDegree != degree) {
        m_cAlphaDegree = degree;
        if (m_bSurfaceLoaded) {
            ApplyAlphaDegree();
        }
    }
}

void DDrawSprite::ApplyAlphaDegree()
{
    if (!m_bAlphaEffect || m_lpSurface == nullptr) {
        return;
    }

    if (m_cAlphaDegree == G_cSpriteAlphaDegree) {
        return;
    }

    m_cAlphaDegree = G_cSpriteAlphaDegree;
    m_bOnCriticalSection = true;

    // Night mode uses 70% brightness to match SFML implementation
    float multiplier = 1.0f;
    switch (m_cAlphaDegree) {
    case 1:
        multiplier = 1.0f;
        break;
    case 2:
        multiplier = 0.7f;
        break;
    }

    // Lock the surface to modify pixels
    DDSURFACEDESC2 ddsd;
    ddsd.dwSize = sizeof(DDSURFACEDESC2);
    if (m_lpSurface->Lock(nullptr, &ddsd, DDLOCK_WAIT, nullptr) != DD_OK) {
        m_bOnCriticalSection = false;
        return;
    }

    uint16_t* pSrc = reinterpret_cast<uint16_t*>(ddsd.lpSurface);
    int16_t pitch = static_cast<int16_t>(ddsd.lPitch >> 1);

    switch (m_pDDraw->m_cPixelFormat) {
    case 1: // RGB565
        for (uint16_t iy = 0; iy < m_wBitmapSizeY; iy++) {
            for (uint16_t ix = 0; ix < m_wBitmapSizeX; ix++) {
                if (pSrc[ix] != m_wColorKey) {
                    int wR = (pSrc[ix] & 0xF800) >> 11;
                    int wG = (pSrc[ix] & 0x7E0) >> 5;
                    int wB = pSrc[ix] & 0x1F;

                    int iR = static_cast<int>(wR * multiplier);
                    int iG = static_cast<int>(wG * multiplier);
                    int iB = static_cast<int>(wB * multiplier);

                    iR = std::clamp(iR, 0, 31);
                    iG = std::clamp(iG, 0, 63);
                    iB = std::clamp(iB, 0, 31);

                    uint16_t wTemp = static_cast<uint16_t>((iR << 11) | (iG << 5) | iB);
                    pSrc[ix] = (wTemp != m_wColorKey) ? wTemp : static_cast<uint16_t>((iR << 11) | (iG << 5) | (iB + 1));
                }
            }
            pSrc += pitch;
        }
        break;

    case 2: // RGB555
        for (uint16_t iy = 0; iy < m_wBitmapSizeY; iy++) {
            for (uint16_t ix = 0; ix < m_wBitmapSizeX; ix++) {
                if (pSrc[ix] != m_wColorKey) {
                    int wR = (pSrc[ix] & 0x7C00) >> 10;
                    int wG = (pSrc[ix] & 0x3E0) >> 5;
                    int wB = pSrc[ix] & 0x1F;

                    int iR = static_cast<int>(wR * multiplier);
                    int iG = static_cast<int>(wG * multiplier);
                    int iB = static_cast<int>(wB * multiplier);

                    iR = std::clamp(iR, 0, 31);
                    iG = std::clamp(iG, 0, 31);
                    iB = std::clamp(iB, 0, 31);

                    uint16_t wTemp = static_cast<uint16_t>((iR << 10) | (iG << 5) | iB);
                    pSrc[ix] = (wTemp != m_wColorKey) ? wTemp : static_cast<uint16_t>((iR << 10) | (iG << 5) | (iB + 1));
                }
            }
            pSrc += pitch;
        }
        break;
    }

    m_lpSurface->Unlock(nullptr);
    m_bOnCriticalSection = false;
}

//////////////////////////////////////////////////////////////////////
// ISprite Implementation - Frame Information
//////////////////////////////////////////////////////////////////////

int DDrawSprite::GetFrameCount() const
{
    return static_cast<int>(m_frames.size());
}

SpriteLib::SpriteRect DDrawSprite::GetFrameRect(int frame) const
{
    if (frame < 0 || frame >= static_cast<int>(m_frames.size())) {
        return SpriteLib::SpriteRect{};
    }

    const auto& f = m_frames[frame];
    return SpriteLib::SpriteRect{
        f.x, f.y,
        f.width, f.height,
        f.pivotX, f.pivotY
    };
}

void DDrawSprite::GetBoundingRect(int x, int y, int frame, int& left, int& top, int& right, int& bottom)
{
    if (frame < 0 || frame >= static_cast<int>(m_frames.size())) {
        left = top = right = bottom = 0;
        return;
    }

    const auto& f = m_frames[frame];
    left = x + f.pivotX;
    top = y + f.pivotY;
    right = left + f.width;
    bottom = top + f.height;
}

void DDrawSprite::CalculateBounds(int x, int y, int frame)
{
    if (frame < 0 || frame >= static_cast<int>(m_frames.size())) {
        m_rcBound.left = m_rcBound.right = m_rcBound.bottom = 0;
        m_rcBound.top = -1;  // Invalid
        return;
    }

    const auto& f = m_frames[frame];
    m_rcBound.left = x + f.pivotX;
    m_rcBound.top = y + f.pivotY;
    m_rcBound.right = m_rcBound.left + f.width;
    m_rcBound.bottom = m_rcBound.top + f.height;
}

bool DDrawSprite::GetLastDrawBounds(int& left, int& top, int& right, int& bottom) const
{
    if (m_rcBound.top == -1) {
        left = top = right = bottom = 0;
        return false;
    }

    left = m_rcBound.left;
    top = m_rcBound.top;
    right = m_rcBound.right;
    bottom = m_rcBound.bottom;
    return true;
}

SpriteLib::BoundRect DDrawSprite::GetBoundRect() const
{
    SpriteLib::BoundRect rect;
    rect.left = m_rcBound.left;
    rect.top = m_rcBound.top;
    rect.right = m_rcBound.right;
    rect.bottom = m_rcBound.bottom;
    return rect;
}

//////////////////////////////////////////////////////////////////////
// Internal Clipping Helper
//////////////////////////////////////////////////////////////////////

bool DDrawSprite::ClipCoordinates(int& dX, int& dY, int& sx, int& sy, int& szx, int& szy) const
{
    if (dX < m_pDDraw->m_rcClipArea.left) {
        int diff = m_pDDraw->m_rcClipArea.left - dX;
        sx += diff;
        szx -= diff;
        if (szx <= 0) return false;
        dX = m_pDDraw->m_rcClipArea.left;
    }
    else if (dX + szx > m_pDDraw->m_rcClipArea.right) {
        szx -= (dX + szx) - m_pDDraw->m_rcClipArea.right;
        if (szx <= 0) return false;
    }

    if (dY < m_pDDraw->m_rcClipArea.top) {
        int diff = m_pDDraw->m_rcClipArea.top - dY;
        sy += diff;
        szy -= diff;
        if (szy <= 0) return false;
        dY = m_pDDraw->m_rcClipArea.top;
    }
    else if (dY + szy > m_pDDraw->m_rcClipArea.bottom) {
        szy -= (dY + szy) - m_pDDraw->m_rcClipArea.bottom;
        if (szy <= 0) return false;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////
// ISprite Implementation - Core Drawing
//////////////////////////////////////////////////////////////////////

void DDrawSprite::Draw(int x, int y, int frame, const SpriteLib::DrawParams& params)
{
    if (m_frames.empty()) return;
    if (frame < 0 || frame >= static_cast<int>(m_frames.size())) return;

    // Update last access time for cache eviction
    m_dwRefTime = GetTickCount();

    // Ensure surface is loaded
    if (!m_bSurfaceLoaded) {
        if (!CreateSurface()) {
            return;
        }
    }
    else if (m_bAlphaEffect && m_cAlphaDegree != G_cSpriteAlphaDegree) {
        if (G_cSpriteAlphaDegree == 2) {
            ApplyAlphaDegree();
        }
        else {
            ReleaseSurface();
            if (!CreateSurface()) return;
        }
    }

    // Determine which drawing method to use based on params
    if (params.isShadow) {
        DrawShadowInternal(x, y, frame, true);
    }
    else if (params.isFade) {
        DrawFadeInternal(x, y, frame);
    }
    else if (params.blendMode == SpriteLib::BlendMode::Average) {
        // 50/50 averaging blend: result = (src + dst) / 2
        DrawAverage(x, y, frame, params.useColorKey);
    }
    else if (params.blendMode == SpriteLib::BlendMode::Additive) {
        // Additive blending - adds source to destination
        DrawAdditive(x, y, frame, params.tintR, params.tintG, params.tintB, params.alpha, params.isColorReplace, params.useColorKey);
    }
    else if (params.tintR != 0 || params.tintG != 0 || params.tintB != 0) {
        if (params.alpha < 1.0f) {
            DrawTintedTransparent(x, y, frame, params.tintR, params.tintG, params.tintB, params.alpha, params.useColorKey, params.isColorReplace);
        }
        else {
            DrawTinted(x, y, frame, params.tintR, params.tintG, params.tintB, params.useColorKey, params.isColorReplace);
        }
    }
    else if (params.alpha < 1.0f) {
        DrawTransparent(x, y, frame, params.alpha, params.useColorKey);
    }
    else {
        // Fast path - hardware blit
        DrawBltFast(x, y, frame, params.useColorKey);
    }
}

void DDrawSprite::DrawToSurface(void* destSurface, int x, int y, int frame, const SpriteLib::DrawParams& params)
{
    if (destSurface == nullptr) return;
    DrawBltFast(x, y, frame, params.useColorKey, static_cast<LPDIRECTDRAWSURFACE7>(destSurface));
}

void DDrawSprite::DrawWidth(int x, int y, int frame, int width, bool vertical)
{
    if (m_frames.empty()) return;
    if (frame < 0 || frame >= static_cast<int>(m_frames.size())) return;

    m_rcBound.top = -1;
    m_bOnCriticalSection = true;

    // Ensure surface is loaded
    if (!m_bSurfaceLoaded) {
        if (!CreateSurface()) return;
    }

    const auto& f = m_frames[frame];
    int sx = f.x;
    int sy = f.y;
    int szx = f.width;
    int szy = f.height;
    int dX = x + f.pivotX;
    int dY = y + f.pivotY;

    // Limit by width parameter
    if (!vertical) {
        if (width < szx) szx = width;
    }
    else {
        if (width < szy) szy = width;
    }

    if (!ClipCoordinates(dX, dY, sx, sy, szx, szy)) {
        m_bOnCriticalSection = false;
        return;
    }

    RECT rcRect = { sx, sy, sx + szx, sy + szy };
    m_rcBound = { dX, dY, dX + szx, dY + szy };

    m_pDDraw->m_lpBackB4->BltFast(dX, dY, m_lpSurface, &rcRect, DDBLTFAST_SRCCOLORKEY | DDBLTFAST_WAIT);

    m_bOnCriticalSection = false;
}

void DDrawSprite::DrawShifted(int x, int y, int shiftX, int shiftY, int frame, const SpriteLib::DrawParams& params)
{
    // DrawShifted draws a 128x128 subregion of the sprite starting at (shiftX, shiftY)
    // This is used for the guide map to show a window into a large map sprite
    // Matches original CSprite::PutShiftSpriteFast / PutShiftTransSprite2

    if (m_frames.empty()) return;
    if (frame < 0 || frame >= static_cast<int>(m_frames.size())) return;

    m_rcBound.top = -1;
    m_bOnCriticalSection = true;

    const auto& f = m_frames[frame];

    // Source position within the sprite surface
    int sx = f.x;
    int sy = f.y;
    int szx = 128;  // Guide map is always 128x128
    int szy = 128;

    // Apply shift to source
    sx += shiftX;
    sy += shiftY;

    // Destination position with pivot offsets (matching original)
    int dX = x + f.pivotX;
    int dY = y + f.pivotY;

    // Clip to screen boundaries
    RECT& clipArea = m_pDDraw->m_rcClipArea;

    if (dX < clipArea.left) {
        int diff = clipArea.left - dX;
        sx += diff;
        szx -= diff;
        if (szx <= 0) {
            m_rcBound.top = -1;
            m_bOnCriticalSection = false;
            return;
        }
        dX = clipArea.left;
    }
    else if (dX + szx > clipArea.right) {
        szx -= (dX + szx) - clipArea.right;
        if (szx <= 0) {
            m_rcBound.top = -1;
            m_bOnCriticalSection = false;
            return;
        }
    }

    if (dY < clipArea.top) {
        int diff = clipArea.top - dY;
        sy += diff;
        szy -= diff;
        if (szy <= 0) {
            m_rcBound.top = -1;
            m_bOnCriticalSection = false;
            return;
        }
        dY = clipArea.top;
    }
    else if (dY + szy > clipArea.bottom) {
        szy -= (dY + szy) - clipArea.bottom;
        if (szy <= 0) {
            m_rcBound.top = -1;
            m_bOnCriticalSection = false;
            return;
        }
    }

    // Ensure surface is loaded
    if (!m_bSurfaceLoaded) {
        if (!CreateSurface()) {
            m_bOnCriticalSection = false;
            return;
        }
    }
    if (m_lpSurface == nullptr) {
        m_bOnCriticalSection = false;
        return;
    }

    // Set bounding rect
    m_rcBound = { dX, dY, dX + szx, dY + szy };

    // Check if we need transparency
    if (params.alpha < 1.0f) {
        // CPU-based transparent drawing using G_lTransRB2/G2 tables (matches PutShiftTransSprite2)
        DrawShiftedTransparent(x, y, shiftX, shiftY, frame, params.alpha, params.useColorKey);
    }
    else {
        // Hardware blit with NO color key (matches original PutShiftSpriteFast)
        RECT rcSrc = { sx, sy, sx + szx, sy + szy };
        m_pDDraw->m_lpBackB4->BltFast(dX, dY, m_lpSurface, &rcSrc, DDBLTFAST_NOCOLORKEY | DDBLTFAST_WAIT);
    }

    m_bOnCriticalSection = false;
}

//////////////////////////////////////////////////////////////////////
// Hardware Blitting
//////////////////////////////////////////////////////////////////////

void DDrawSprite::DrawBltFast(int x, int y, int frame, bool useColorKey, LPDIRECTDRAWSURFACE7 destSurface)
{
    // Validate frame bounds
    if (m_frames.empty()) return;
    if (frame < 0 || frame >= static_cast<int>(m_frames.size())) return;

    // Ensure surface is loaded
    if (!m_bSurfaceLoaded) {
        if (!CreateSurface()) return;
    }
    if (m_lpSurface == nullptr) return;

    m_rcBound.top = -1;
    m_bOnCriticalSection = true;

    const auto& f = m_frames[frame];
    int sx = f.x;
    int sy = f.y;
    int szx = f.width;
    int szy = f.height;
    int dX = x + f.pivotX;
    int dY = y + f.pivotY;

    if (!ClipCoordinates(dX, dY, sx, sy, szx, szy)) {
        m_bOnCriticalSection = false;
        return;
    }

    RECT rcRect = { sx, sy, sx + szx, sy + szy };
    m_rcBound = { dX, dY, dX + szx, dY + szy };

    LPDIRECTDRAWSURFACE7 dest = destSurface ? destSurface : m_pDDraw->m_lpBackB4;
    if (dest == nullptr) {
        m_bOnCriticalSection = false;
        return;
    }

    DWORD flags = useColorKey ? (DDBLTFAST_SRCCOLORKEY | DDBLTFAST_WAIT) : (DDBLTFAST_NOCOLORKEY | DDBLTFAST_WAIT);

    dest->BltFast(dX, dY, m_lpSurface, &rcRect, flags);

    m_bOnCriticalSection = false;
}

//////////////////////////////////////////////////////////////////////
// CPU-Based Transparent Drawing
//////////////////////////////////////////////////////////////////////

void DDrawSprite::DrawTransparent(int x, int y, int frame, float alpha, bool useColorKey)
{
    m_rcBound.top = -1;
    m_bOnCriticalSection = true;

    const auto& f = m_frames[frame];
    int sx = f.x;
    int sy = f.y;
    int szx = f.width;
    int szy = f.height;
    int dX = x + f.pivotX;
    int dY = y + f.pivotY;

    if (!ClipCoordinates(dX, dY, sx, sy, szx, szy)) {
        m_bOnCriticalSection = false;
        return;
    }

    m_rcBound = { dX, dY, dX + szx, dY + szy };

    uint16_t* pSrc = m_pSurfaceAddr + sx + sy * m_sPitch;
    uint16_t* pDst = m_pDDraw->m_pBackB4Addr + dX + dY * m_pDDraw->m_sBackB4Pitch;

    if (szx == 0 || szy == 0) {
        m_bOnCriticalSection = false;
        return;
    }

    // Select appropriate transparency table based on alpha level
    long (*pTransRB)[64] = G_lTransRB100;
    long (*pTransG)[64] = G_lTransG100;

    if (alpha <= 0.25f) {
        pTransRB = G_lTransRB25;
        pTransG = G_lTransG25;
    }
    else if (alpha <= 0.50f) {
        pTransRB = G_lTransRB50;
        pTransG = G_lTransG50;
    }
    else if (alpha <= 0.70f) {
        pTransRB = G_lTransRB70;
        pTransG = G_lTransG70;
    }

    switch (m_pDDraw->m_cPixelFormat) {
    case 1: // RGB565
        for (int iy = 0; iy < szy; iy++) {
            for (int ix = 0; ix < szx; ix++) {
                if (!useColorKey || pSrc[ix] != m_wColorKey) {
                    pDst[ix] = static_cast<uint16_t>(
                        (pTransRB[(pDst[ix] & 0xF800) >> 11][(pSrc[ix] & 0xF800) >> 11] << 11) |
                        (pTransG[(pDst[ix] & 0x7E0) >> 5][(pSrc[ix] & 0x7E0) >> 5] << 5) |
                        pTransRB[(pDst[ix] & 0x1F)][(pSrc[ix] & 0x1F)]
                    );
                }
            }
            pSrc += m_sPitch;
            pDst += m_pDDraw->m_sBackB4Pitch;
        }
        break;

    case 2: // RGB555
        for (int iy = 0; iy < szy; iy++) {
            for (int ix = 0; ix < szx; ix++) {
                if (!useColorKey || pSrc[ix] != m_wColorKey) {
                    pDst[ix] = static_cast<uint16_t>(
                        (pTransRB[(pDst[ix] & 0x7C00) >> 10][(pSrc[ix] & 0x7C00) >> 10] << 10) |
                        (pTransG[(pDst[ix] & 0x3E0) >> 5][(pSrc[ix] & 0x3E0) >> 5] << 5) |
                        pTransRB[(pDst[ix] & 0x1F)][(pSrc[ix] & 0x1F)]
                    );
                }
            }
            pSrc += m_sPitch;
            pDst += m_pDDraw->m_sBackB4Pitch;
        }
        break;
    }

    m_bOnCriticalSection = false;
}

void DDrawSprite::DrawAverage(int x, int y, int frame, bool useColorKey)
{
    // 50/50 averaging blend: result = (source + destination) / 2
    // Uses G_lTransRB2/G_lTransG2 tables for accurate color blending
    m_rcBound.top = -1;
    m_bOnCriticalSection = true;

    const auto& f = m_frames[frame];
    int sx = f.x;
    int sy = f.y;
    int szx = f.width;
    int szy = f.height;
    int dX = x + f.pivotX;
    int dY = y + f.pivotY;

    if (!ClipCoordinates(dX, dY, sx, sy, szx, szy)) {
        m_bOnCriticalSection = false;
        return;
    }

    m_rcBound = { dX, dY, dX + szx, dY + szy };

    uint16_t* pSrc = m_pSurfaceAddr + sx + sy * m_sPitch;
    uint16_t* pDst = m_pDDraw->m_pBackB4Addr + dX + dY * m_pDDraw->m_sBackB4Pitch;

    if (szx == 0 || szy == 0) {
        m_bOnCriticalSection = false;
        return;
    }

    switch (m_pDDraw->m_cPixelFormat) {
    case 1: // RGB565
        for (int iy = 0; iy < szy; iy++) {
            for (int ix = 0; ix < szx; ix++) {
                if (!useColorKey || pSrc[ix] != m_wColorKey) {
                    pDst[ix] = static_cast<uint16_t>(
                        (G_lTransRB2[(pDst[ix] & 0xF800) >> 11][(pSrc[ix] & 0xF800) >> 11] << 11) |
                        (G_lTransG2[(pDst[ix] & 0x7E0) >> 5][(pSrc[ix] & 0x7E0) >> 5] << 5) |
                        G_lTransRB2[(pDst[ix] & 0x1F)][(pSrc[ix] & 0x1F)]
                    );
                }
            }
            pSrc += m_sPitch;
            pDst += m_pDDraw->m_sBackB4Pitch;
        }
        break;

    case 2: // RGB555
        for (int iy = 0; iy < szy; iy++) {
            for (int ix = 0; ix < szx; ix++) {
                if (!useColorKey || pSrc[ix] != m_wColorKey) {
                    pDst[ix] = static_cast<uint16_t>(
                        (G_lTransRB2[(pDst[ix] & 0x7C00) >> 10][(pSrc[ix] & 0x7C00) >> 10] << 10) |
                        (G_lTransG2[(pDst[ix] & 0x3E0) >> 5][(pSrc[ix] & 0x3E0) >> 5] << 5) |
                        G_lTransRB2[(pDst[ix] & 0x1F)][(pSrc[ix] & 0x1F)]
                    );
                }
            }
            pSrc += m_sPitch;
            pDst += m_pDDraw->m_sBackB4Pitch;
        }
        break;
    }

    m_bOnCriticalSection = false;
}

void DDrawSprite::DrawShiftedTransparent(int x, int y, int shiftX, int shiftY, int frame, float alpha, bool useColorKey)
{
    // Matches original CSprite::PutShiftTransSprite2
    // Uses G_lTransRB2/G_lTransG2 tables for light transparency effect
    if (m_frames.empty()) return;
    if (frame < 0 || frame >= static_cast<int>(m_frames.size())) return;

    m_rcBound.top = -1;
    m_bOnCriticalSection = true;

    const auto& f = m_frames[frame];

    // Source position within the sprite surface
    int sx = f.x;
    int sy = f.y;
    int szx = 128;  // Guide map is always 128x128
    int szy = 128;

    // Apply shift to source
    sx += shiftX;
    sy += shiftY;

    // Destination position with pivot offsets (matching original)
    int dX = x + f.pivotX;
    int dY = y + f.pivotY;

    // Clip to screen boundaries (matching original logic)
    RECT& clipArea = m_pDDraw->m_rcClipArea;

    if (dX < clipArea.left) {
        int diff = clipArea.left - dX;
        sx += diff;
        szx -= diff;
        if (szx < 0) {
            m_rcBound.top = -1;
            m_bOnCriticalSection = false;
            return;
        }
        dX = clipArea.left;
    }
    else if (dX + szx > clipArea.right) {
        szx -= (dX + szx) - clipArea.right;
        if (szx < 0) {
            m_rcBound.top = -1;
            m_bOnCriticalSection = false;
            return;
        }
    }

    if (dY < clipArea.top) {
        int diff = clipArea.top - dY;
        sy += diff;
        szy -= diff;
        if (szy < 0) {
            m_rcBound.top = -1;
            m_bOnCriticalSection = false;
            return;
        }
        dY = clipArea.top;
    }
    else if (dY + szy > clipArea.bottom) {
        szy -= (dY + szy) - clipArea.bottom;
        if (szy < 0) {
            m_rcBound.top = -1;
            m_bOnCriticalSection = false;
            return;
        }
    }

    if (szx == 0 || szy == 0) {
        m_bOnCriticalSection = false;
        return;
    }

    m_rcBound = { dX, dY, dX + szx, dY + szy };

    uint16_t* pSrc = m_pSurfaceAddr + sx + sy * m_sPitch;
    uint16_t* pDst = m_pDDraw->m_pBackB4Addr + dX + dY * m_pDDraw->m_sBackB4Pitch;

    // Use G_lTransRB2/G_lTransG2 tables (matches original PutShiftTransSprite2)
    switch (m_pDDraw->m_cPixelFormat) {
    case 1: // RGB565
        for (int iy = 0; iy < szy; iy++) {
            for (int ix = 0; ix < szx; ix++) {
                if (pSrc[ix] != m_wColorKey) {
                    pDst[ix] = static_cast<uint16_t>(
                        (G_lTransRB2[(pDst[ix] & 0xF800) >> 11][(pSrc[ix] & 0xF800) >> 11] << 11) |
                        (G_lTransG2[(pDst[ix] & 0x7E0) >> 5][(pSrc[ix] & 0x7E0) >> 5] << 5) |
                        G_lTransRB2[(pDst[ix] & 0x1F)][(pSrc[ix] & 0x1F)]
                    );
                }
            }
            pSrc += m_sPitch;
            pDst += m_pDDraw->m_sBackB4Pitch;
        }
        break;

    case 2: // RGB555
        for (int iy = 0; iy < szy; iy++) {
            for (int ix = 0; ix < szx; ix++) {
                if (pSrc[ix] != m_wColorKey) {
                    pDst[ix] = static_cast<uint16_t>(
                        (G_lTransRB2[(pDst[ix] & 0x7C00) >> 10][(pSrc[ix] & 0x7C00) >> 10] << 10) |
                        (G_lTransG2[(pDst[ix] & 0x3E0) >> 5][(pSrc[ix] & 0x3E0) >> 5] << 5) |
                        G_lTransRB2[(pDst[ix] & 0x1F)][(pSrc[ix] & 0x1F)]
                    );
                }
            }
            pSrc += m_sPitch;
            pDst += m_pDDraw->m_sBackB4Pitch;
        }
        break;
    }

    m_bOnCriticalSection = false;
}

//////////////////////////////////////////////////////////////////////
// CPU-Based Tinted Drawing
//////////////////////////////////////////////////////////////////////

void DDrawSprite::DrawTinted(int x, int y, int frame, int16_t r, int16_t g, int16_t b, bool useColorKey, bool isColorReplace)
{
    m_rcBound.top = -1;
    m_bOnCriticalSection = true;

    const auto& f = m_frames[frame];
    int sx = f.x;
    int sy = f.y;
    int szx = f.width;
    int szy = f.height;
    int dX = x + f.pivotX;
    int dY = y + f.pivotY;

    if (!ClipCoordinates(dX, dY, sx, sy, szx, szy)) {
        m_bOnCriticalSection = false;
        return;
    }

    m_rcBound = { dX, dY, dX + szx, dY + szy };

    uint16_t* pSrc = m_pSurfaceAddr + sx + sy * m_sPitch;
    uint16_t* pDst = m_pDDraw->m_pBackB4Addr + dX + dY * m_pDDraw->m_sBackB4Pitch;

    if (szx == 0 || szy == 0) {
        m_bOnCriticalSection = false;
        return;
    }

    int iRedPlus255, iGreenPlus255, iBluePlus255;

    if (isColorReplace) {
        // Color replacement (text, UI): use values directly without scaling
        iRedPlus255 = r + 255;
        iGreenPlus255 = g + 255;
        iBluePlus255 = b + 255;
    }
    else {
        // Sprite tinting (frozen, items): scale 8bpp to 16bpp
        // R/B: 8bpp -> 5-bit (multiply by 31/255)
        // G: 8bpp -> 6-bit (multiply by 63/255)
        int scaledR = (r * 31) / 255;
        int scaledG = (g * 63) / 255;
        int scaledB = (b * 31) / 255;

        iRedPlus255 = scaledR + 255;
        iGreenPlus255 = scaledG + 255;
        iBluePlus255 = scaledB + 255;
    }

    switch (m_pDDraw->m_cPixelFormat) {
    case 1: // RGB565
        for (int iy = 0; iy < szy; iy++) {
            for (int ix = 0; ix < szx; ix++) {
                if (!useColorKey || pSrc[ix] != m_wColorKey) {
                    pDst[ix] = static_cast<uint16_t>(
                        (G_iAddTable31[(pSrc[ix] & 0xF800) >> 11][iRedPlus255] << 11) |
                        (G_iAddTable63[(pSrc[ix] & 0x7E0) >> 5][iGreenPlus255] << 5) |
                        G_iAddTable31[(pSrc[ix] & 0x1F)][iBluePlus255]
                    );
                }
            }
            pSrc += m_sPitch;
            pDst += m_pDDraw->m_sBackB4Pitch;
        }
        break;

    case 2: // RGB555
        for (int iy = 0; iy < szy; iy++) {
            for (int ix = 0; ix < szx; ix++) {
                if (!useColorKey || pSrc[ix] != m_wColorKey) {
                    pDst[ix] = static_cast<uint16_t>(
                        (G_iAddTable31[(pSrc[ix] & 0x7C00) >> 10][iRedPlus255] << 10) |
                        (G_iAddTable31[(pSrc[ix] & 0x3E0) >> 5][iGreenPlus255] << 5) |
                        G_iAddTable31[(pSrc[ix] & 0x1F)][iBluePlus255]
                    );
                }
            }
            pSrc += m_sPitch;
            pDst += m_pDDraw->m_sBackB4Pitch;
        }
        break;
    }

    m_bOnCriticalSection = false;
}

//////////////////////////////////////////////////////////////////////
// CPU-Based Tinted + Transparent Drawing
//////////////////////////////////////////////////////////////////////

void DDrawSprite::DrawTintedTransparent(int x, int y, int frame, int16_t r, int16_t g, int16_t b, float alpha, bool useColorKey, bool isColorReplace)
{
    m_rcBound.top = -1;
    m_bOnCriticalSection = true;

    const auto& f = m_frames[frame];
    int sx = f.x;
    int sy = f.y;
    int szx = f.width;
    int szy = f.height;
    int dX = x + f.pivotX;
    int dY = y + f.pivotY;

    if (!ClipCoordinates(dX, dY, sx, sy, szx, szy)) {
        m_bOnCriticalSection = false;
        return;
    }

    m_rcBound = { dX, dY, dX + szx, dY + szy };

    uint16_t* pSrc = m_pSurfaceAddr + sx + sy * m_sPitch;
    uint16_t* pDst = m_pDDraw->m_pBackB4Addr + dX + dY * m_pDDraw->m_sBackB4Pitch;

    if (szx == 0 || szy == 0) {
        m_bOnCriticalSection = false;
        return;
    }

    int iRedPlus255, iGreenPlus255, iBluePlus255;

    if (isColorReplace) {
        // Color replacement (text, UI): use values directly without scaling
        iRedPlus255 = r + 255;
        iGreenPlus255 = g + 255;
        iBluePlus255 = b + 255;
    }
    else {
        // Sprite tinting (frozen, items): scale 8bpp to 16bpp
        // R/B: 8bpp -> 5-bit (multiply by 31/255)
        // G: 8bpp -> 6-bit (multiply by 63/255)
        int scaledR = (r * 31) / 255;
        int scaledG = (g * 63) / 255;
        int scaledB = (b * 31) / 255;

        iRedPlus255 = scaledR + 255;
        iGreenPlus255 = scaledG + 255;
        iBluePlus255 = scaledB + 255;
    }

    // Tinted transparent drawing with variable alpha
    // 1. Apply tint to source
    // 2. Blend result with destination based on alpha

    switch (m_pDDraw->m_cPixelFormat) {
    case 1: // RGB565
        for (int iy = 0; iy < szy; iy++) {
            for (int ix = 0; ix < szx; ix++) {
                if (!useColorKey || pSrc[ix] != m_wColorKey) {
                    int dstR = (pDst[ix] & 0xF800) >> 11;
                    int dstG = (pDst[ix] & 0x7E0) >> 5;
                    int dstB = (pDst[ix] & 0x1F);

                    // Apply tint using lookup tables
                    int tintedR = G_iAddTable31[(pSrc[ix] & 0xF800) >> 11][iRedPlus255];
                    int tintedG = G_iAddTable63[(pSrc[ix] & 0x7E0) >> 5][iGreenPlus255];
                    int tintedB = G_iAddTable31[(pSrc[ix] & 0x1F)][iBluePlus255];

                    // Blend with destination based on alpha
                    int outR = static_cast<int>(tintedR * alpha + dstR * (1.0f - alpha));
                    int outG = static_cast<int>(tintedG * alpha + dstG * (1.0f - alpha));
                    int outB = static_cast<int>(tintedB * alpha + dstB * (1.0f - alpha));

                    if (outR > 31) outR = 31;
                    if (outG > 63) outG = 63;
                    if (outB > 31) outB = 31;

                    pDst[ix] = static_cast<uint16_t>((outR << 11) | (outG << 5) | outB);
                }
            }
            pSrc += m_sPitch;
            pDst += m_pDDraw->m_sBackB4Pitch;
        }
        break;

    case 2: // RGB555
        for (int iy = 0; iy < szy; iy++) {
            for (int ix = 0; ix < szx; ix++) {
                if (!useColorKey || pSrc[ix] != m_wColorKey) {
                    int dstR = (pDst[ix] & 0x7C00) >> 10;
                    int dstG = (pDst[ix] & 0x3E0) >> 5;
                    int dstB = (pDst[ix] & 0x1F);

                    // Apply tint using lookup tables
                    int tintedR = G_iAddTable31[(pSrc[ix] & 0x7C00) >> 10][iRedPlus255];
                    int tintedG = G_iAddTable31[(pSrc[ix] & 0x3E0) >> 5][iGreenPlus255];
                    int tintedB = G_iAddTable31[(pSrc[ix] & 0x1F)][iBluePlus255];

                    // Blend with destination based on alpha
                    int outR = static_cast<int>(tintedR * alpha + dstR * (1.0f - alpha));
                    int outG = static_cast<int>(tintedG * alpha + dstG * (1.0f - alpha));
                    int outB = static_cast<int>(tintedB * alpha + dstB * (1.0f - alpha));

                    if (outR > 31) outR = 31;
                    if (outG > 31) outG = 31;
                    if (outB > 31) outB = 31;

                    pDst[ix] = static_cast<uint16_t>((outR << 10) | (outG << 5) | outB);
                }
            }
            pSrc += m_sPitch;
            pDst += m_pDDraw->m_sBackB4Pitch;
        }
        break;
    }

    m_bOnCriticalSection = false;
}

//////////////////////////////////////////////////////////////////////
// CPU-Based Shadow Drawing
//////////////////////////////////////////////////////////////////////

void DDrawSprite::DrawShadowInternal(int x, int y, int frame, bool clipped)
{
    m_rcBound.top = -1;
    m_bOnCriticalSection = true;

    const auto& f = m_frames[frame];
    int sx = f.x;
    int sy = f.y;
    int szx = f.width;
    int szy = f.height;
    int dX = x + f.pivotX;
    int dY = y + f.pivotY;

    if (clipped) {
        if (!ClipCoordinates(dX, dY, sx, sy, szx, szy)) {
            m_bOnCriticalSection = false;
            return;
        }
    }

    uint16_t* pSrc = m_pSurfaceAddr + sx + (sy + szy - 1) * m_sPitch;
    uint16_t* pDst = m_pDDraw->m_pBackB4Addr + dX + (dY + szy - 1) * m_pDDraw->m_sBackB4Pitch;

    // At night (G_cSpriteAlphaDegree == 2), reduce shadow opacity by 50%
    // Day: >> 2 = 25% brightness (darker shadow)
    // Night: >> 1 = 50% brightness (lighter shadow)
    bool isNight = (G_cSpriteAlphaDegree == 2);

    switch (m_pDDraw->m_cPixelFormat) {
    case 1: // RGB565
        if (isNight) {
            for (int iy = 0; iy < szy; iy += 3) {
                for (int ix = 0; ix < szx; ix++) {
                    if (pSrc[ix] != m_wColorKey) {
                        if ((dX - (iy / 3) + ix) > 0) {
                            pDst[ix] = (pDst[ix] & 0xF7DE) >> 1;  // 50% brightness at night
                        }
                    }
                }
                pSrc -= m_sPitch * 3;
                pDst -= m_pDDraw->m_sBackB4Pitch + 1;
            }
        } else {
            for (int iy = 0; iy < szy; iy += 3) {
                for (int ix = 0; ix < szx; ix++) {
                    if (pSrc[ix] != m_wColorKey) {
                        if ((dX - (iy / 3) + ix) > 0) {
                            pDst[ix] = (pDst[ix] & 0xE79C) >> 2;  // 25% brightness during day
                        }
                    }
                }
                pSrc -= m_sPitch * 3;
                pDst -= m_pDDraw->m_sBackB4Pitch + 1;
            }
        }
        break;

    case 2: // RGB555
        if (isNight) {
            for (int iy = 0; iy < szy; iy += 3) {
                for (int ix = 0; ix < szx; ix++) {
                    if (pSrc[ix] != m_wColorKey) {
                        if ((dX - (iy / 3) + ix) > 0) {
                            pDst[ix] = (pDst[ix] & 0x7BDE) >> 1;  // 50% brightness at night
                        }
                    }
                }
                pSrc -= m_sPitch * 3;
                pDst -= m_pDDraw->m_sBackB4Pitch + 1;
            }
        } else {
            for (int iy = 0; iy < szy; iy += 3) {
                for (int ix = 0; ix < szx; ix++) {
                    if (pSrc[ix] != m_wColorKey) {
                        if ((dX - (iy / 3) + ix) > 0) {
                            pDst[ix] = (pDst[ix] & 0x739C) >> 2;  // 25% brightness during day
                        }
                    }
                }
                pSrc -= m_sPitch * 3;
                pDst -= m_pDDraw->m_sBackB4Pitch + 1;
            }
        }
        break;
    }

    m_bOnCriticalSection = false;
}

//////////////////////////////////////////////////////////////////////
// CPU-Based Fade Drawing
//////////////////////////////////////////////////////////////////////

void DDrawSprite::DrawFadeInternal(int x, int y, int frame, uint16_t* pDestAddr, int16_t destPitch)
{
    m_rcBound.top = -1;
    m_bOnCriticalSection = true;

    const auto& f = m_frames[frame];
    int sx = f.x;
    int sy = f.y;
    int szx = f.width;
    int szy = f.height;
    int dX = x + f.pivotX;
    int dY = y + f.pivotY;

    if (!ClipCoordinates(dX, dY, sx, sy, szx, szy)) {
        m_bOnCriticalSection = false;
        return;
    }

    m_rcBound = { dX, dY, dX + szx, dY + szy };

    uint16_t* pSrc = m_pSurfaceAddr + sx + sy * m_sPitch;
    uint16_t* pDst;
    int16_t pitch;

    if (pDestAddr != nullptr) {
        pDst = pDestAddr + dX + dY * destPitch;
        pitch = destPitch;
    }
    else {
        pDst = m_pDDraw->m_pBackB4Addr + dX + dY * m_pDDraw->m_sBackB4Pitch;
        pitch = m_pDDraw->m_sBackB4Pitch;
    }

    if (szx == 0 || szy == 0) {
        m_bOnCriticalSection = false;
        return;
    }

    // Fade effect: darken destination pixels where source is non-transparent
    // Uses bit masking and right shift to reduce each color channel
    // At night (G_cSpriteAlphaDegree == 2), reduce fade intensity by 50%
    // Day: >> 2 = 25% brightness | Night: >> 1 = 50% brightness
    bool isNight = (G_cSpriteAlphaDegree == 2);

    switch (m_pDDraw->m_cPixelFormat) {
    case 1: // RGB565
        if (isNight) {
            for (int iy = 0; iy < szy; iy++) {
                for (int ix = 0; ix < szx; ix++) {
                    if (pSrc[ix] != m_wColorKey) {
                        pDst[ix] = ((pDst[ix] & 0xF7DE) >> 1);  // 50% brightness at night
                    }
                }
                pSrc += m_sPitch;
                pDst += pitch;
            }
        } else {
            for (int iy = 0; iy < szy; iy++) {
                for (int ix = 0; ix < szx; ix++) {
                    if (pSrc[ix] != m_wColorKey) {
                        pDst[ix] = ((pDst[ix] & 0xE79C) >> 2);  // 25% brightness during day
                    }
                }
                pSrc += m_sPitch;
                pDst += pitch;
            }
        }
        break;

    case 2: // RGB555
        if (isNight) {
            for (int iy = 0; iy < szy; iy++) {
                for (int ix = 0; ix < szx; ix++) {
                    if (pSrc[ix] != m_wColorKey) {
                        pDst[ix] = ((pDst[ix] & 0x7BDE) >> 1);  // 50% brightness at night
                    }
                }
                pSrc += m_sPitch;
                pDst += pitch;
            }
        } else {
            for (int iy = 0; iy < szy; iy++) {
                for (int ix = 0; ix < szx; ix++) {
                    if (pSrc[ix] != m_wColorKey) {
                        pDst[ix] = ((pDst[ix] & 0x739C) >> 2);  // 25% brightness during day
                    }
                }
                pSrc += m_sPitch;
                pDst += pitch;
            }
        }
        break;
    }

    m_bOnCriticalSection = false;
}

//////////////////////////////////////////////////////////////////////
// CPU-Based Additive Blending
//////////////////////////////////////////////////////////////////////

void DDrawSprite::DrawAdditive(int x, int y, int frame, int16_t r, int16_t g, int16_t b, float alpha, bool isColorReplace, bool useColorKey)
{
    m_rcBound.top = -1;
    m_bOnCriticalSection = true;

    const auto& f = m_frames[frame];
    int sx = f.x;
    int sy = f.y;
    int szx = f.width;
    int szy = f.height;
    int dX = x + f.pivotX;
    int dY = y + f.pivotY;

    if (!ClipCoordinates(dX, dY, sx, sy, szx, szy)) {
        m_bOnCriticalSection = false;
        return;
    }

    m_rcBound = { dX, dY, dX + szx, dY + szy };

    uint16_t* pSrc = m_pSurfaceAddr + sx + sy * m_sPitch;
    uint16_t* pDst = m_pDDraw->m_pBackB4Addr + dX + dY * m_pDDraw->m_sBackB4Pitch;

    if (szx == 0 || szy == 0) {
        m_bOnCriticalSection = false;
        return;
    }

    // Color multipliers for isColorReplace mode (normalized to 0-31/63 range)
    // r,g,b are in 0-255 range, we need to scale them
    float rMult = isColorReplace ? (r / 255.0f) : 1.0f;
    float gMult = isColorReplace ? (g / 255.0f) : 1.0f;
    float bMult = isColorReplace ? (b / 255.0f) : 1.0f;

    // Apply alpha to the multipliers
    rMult *= alpha;
    gMult *= alpha;
    bMult *= alpha;

    switch (m_pDDraw->m_cPixelFormat) {
    case 1: // RGB565
        for (int iy = 0; iy < szy; iy++) {
            for (int ix = 0; ix < szx; ix++) {
                if (!useColorKey || pSrc[ix] != m_wColorKey) {
                    // Extract source RGB (5-6-5 bits)
                    int srcR = (pSrc[ix] & 0xF800) >> 11;
                    int srcG = (pSrc[ix] & 0x7E0) >> 5;
                    int srcB = (pSrc[ix] & 0x1F);

                    // Apply color multiplier
                    srcR = static_cast<int>(srcR * rMult);
                    srcG = static_cast<int>(srcG * gMult);
                    srcB = static_cast<int>(srcB * bMult);

                    // Extract destination RGB
                    int dstR = (pDst[ix] & 0xF800) >> 11;
                    int dstG = (pDst[ix] & 0x7E0) >> 5;
                    int dstB = (pDst[ix] & 0x1F);

                    // Additive blend (src + dst, clamped)
                    int outR = srcR + dstR; if (outR > 31) outR = 31;
                    int outG = srcG + dstG; if (outG > 63) outG = 63;
                    int outB = srcB + dstB; if (outB > 31) outB = 31;

                    pDst[ix] = static_cast<uint16_t>((outR << 11) | (outG << 5) | outB);
                }
            }
            pSrc += m_sPitch;
            pDst += m_pDDraw->m_sBackB4Pitch;
        }
        break;

    case 2: // RGB555
        for (int iy = 0; iy < szy; iy++) {
            for (int ix = 0; ix < szx; ix++) {
                if (!useColorKey || pSrc[ix] != m_wColorKey) {
                    // Extract source RGB (5-5-5 bits)
                    int srcR = (pSrc[ix] & 0x7C00) >> 10;
                    int srcG = (pSrc[ix] & 0x3E0) >> 5;
                    int srcB = (pSrc[ix] & 0x1F);

                    // Apply color multiplier
                    srcR = static_cast<int>(srcR * rMult);
                    srcG = static_cast<int>(srcG * gMult);
                    srcB = static_cast<int>(srcB * bMult);

                    // Extract destination RGB
                    int dstR = (pDst[ix] & 0x7C00) >> 10;
                    int dstG = (pDst[ix] & 0x3E0) >> 5;
                    int dstB = (pDst[ix] & 0x1F);

                    // Additive blend (src + dst, clamped)
                    int outR = srcR + dstR; if (outR > 31) outR = 31;
                    int outG = srcG + dstG; if (outG > 31) outG = 31;
                    int outB = srcB + dstB; if (outB > 31) outB = 31;

                    pDst[ix] = static_cast<uint16_t>((outR << 10) | (outG << 5) | outB);
                }
            }
            pSrc += m_sPitch;
            pDst += m_pDDraw->m_sBackB4Pitch;
        }
        break;
    }

    m_bOnCriticalSection = false;
}

//////////////////////////////////////////////////////////////////////
// Collision Detection
//////////////////////////////////////////////////////////////////////

bool DDrawSprite::CheckCollision(int spriteX, int spriteY, int frame, int pointX, int pointY)
{
    if (m_frames.empty()) return false;
    if (frame < 0 || frame >= static_cast<int>(m_frames.size())) return false;
    if (!m_bSurfaceLoaded) return false;

    const auto& f = m_frames[frame];
    int dX = spriteX + f.pivotX;
    int dY = spriteY + f.pivotY;
    int szx = f.width;
    int szy = f.height;

    // Quick bounds check
    if (pointX < dX || pointX >= dX + szx) return false;
    if (pointY < dY || pointY >= dY + szy) return false;

    // Clip area check
    if (pointX < m_pDDraw->m_rcClipArea.left + 3 || pointX > m_pDDraw->m_rcClipArea.right - 3) return false;
    if (pointY < m_pDDraw->m_rcClipArea.top + 3 || pointY > m_pDDraw->m_rcClipArea.bottom - 3) return false;

    // Pixel-level check
    int localX = pointX - dX;
    int localY = pointY - dY;

    uint16_t* pSrc = m_pSurfaceAddr + f.x + localX + (f.y + localY) * m_sPitch;
    return (*pSrc != m_wColorKey);
}

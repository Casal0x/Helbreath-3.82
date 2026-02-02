// DDrawRenderer.cpp: DirectDraw renderer implementation
//
// Part of DDrawEngine static library
//////////////////////////////////////////////////////////////////////

#include "DDrawRenderer.h"
#include "ITextRenderer.h"

// External global tables defined in DXC_ddraw.cpp
extern long G_lTransG100[64][64], G_lTransRB100[64][64];
extern long G_lTransG70[64][64], G_lTransRB70[64][64];
extern long G_lTransG50[64][64], G_lTransRB50[64][64];
extern long G_lTransG25[64][64], G_lTransRB25[64][64];
extern long G_lTransG2[64][64], G_lTransRB2[64][64];
extern int G_iAddTable31[64][510], G_iAddTable63[64][510];
extern int G_iAddTransTable31[510][64], G_iAddTransTable63[510][64];
extern char G_cSpriteAlphaDegree;

DDrawRenderer::DDrawRenderer()
    : m_backBufferLocked(false)
{
}

DDrawRenderer::~DDrawRenderer()
{
    Shutdown();
}

bool DDrawRenderer::Init(NativeWindowHandle hWnd)
{
    return m_ddraw.bInit(hWnd);
}

void DDrawRenderer::Shutdown()
{
    // DXC_ddraw destructor handles cleanup
}

void DDrawRenderer::SetFullscreen(bool fullscreen)
{
    m_ddraw.m_bFullMode = fullscreen;
}

bool DDrawRenderer::IsFullscreen() const
{
    return m_ddraw.m_bFullMode;
}

void DDrawRenderer::ChangeDisplayMode(NativeWindowHandle hWnd)
{
    m_ddraw.ChangeDisplayMode(hWnd);
}

void DDrawRenderer::BeginFrame()
{
    m_ddraw.ClearBackB4();
}

void DDrawRenderer::EndFrame()
{
    m_ddraw.iFlip();
}

void DDrawRenderer::PutPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b)
{
    m_ddraw.PutPixel(static_cast<short>(x), static_cast<short>(y), r, g, b);
}

void DDrawRenderer::DrawShadowBox(int x1, int y1, int x2, int y2, int type)
{
    m_ddraw.DrawShadowBox(static_cast<short>(x1), static_cast<short>(y1),
                          static_cast<short>(x2), static_cast<short>(y2), type);
}

void DDrawRenderer::DrawItemShadowBox(int x1, int y1, int x2, int y2, int type)
{
    m_ddraw.DrawItemShadowBox(static_cast<short>(x1), static_cast<short>(y1),
                               static_cast<short>(x2), static_cast<short>(y2), type);
}

void DDrawRenderer::DrawLine(int x0, int y0, int x1, int y1, int iR, int iG, int iB, float alpha)
{
    if ((x0 == x1) && (y0 == y1)) return;

    int pitch = 0;
    uint16_t* pBuffer = LockBackBuffer(&pitch);
    if (!pBuffer) return;

    // Select transparency tables based on alpha
    const long (*transRB)[64];
    const long (*transG)[64];
    if (alpha >= 1.0f) {
        transRB = GetTransTableRB100();
        transG = GetTransTableG100();
    } else if (alpha >= 0.7f) {
        transRB = GetTransTableRB70();
        transG = GetTransTableG70();
    } else if (alpha >= 0.5f) {
        transRB = GetTransTableRB50();
        transG = GetTransTableG50();
    } else {
        transRB = GetTransTableRB25();
        transG = GetTransTableG25();
    }

    int pixelFormat = GetPixelFormat();
    int width = GetWidth();
    int height = GetHeight();

    // Bresenham's line algorithm
    int dx = x1 - x0;
    int dy = y1 - y0;
    int x_inc = (dx >= 0) ? 1 : -1;
    int y_inc = (dy >= 0) ? 1 : -1;
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;

    int iResultX = x0;
    int iResultY = y0;
    int error = 0;

    if (dx > dy)
    {
        for (int index = 0; index <= dx; index++)
        {
            error += dy;
            if (error > dx)
            {
                error -= dx;
                iResultY += y_inc;
            }
            iResultX += x_inc;
            if ((iResultX >= 0) && (iResultX < width) && (iResultY >= 0) && (iResultY < height))
            {
                uint16_t* pDst = pBuffer + iResultX + (iResultY * pitch);
                int dstR, dstG, dstB;
                if (pixelFormat == 1) // RGB565
                {
                    dstR = static_cast<int>(transRB[(pDst[0] & 0xF800) >> 11][iR]);
                    dstG = static_cast<int>(transG[(pDst[0] & 0x7E0) >> 5][iG]);
                    dstB = static_cast<int>(transRB[(pDst[0] & 0x1F)][iB]);
                    *pDst = static_cast<uint16_t>((dstR << 11) | (dstG << 5) | dstB);
                }
                else // RGB555
                {
                    dstR = static_cast<int>(transRB[(pDst[0] & 0x7C00) >> 10][iR]);
                    dstG = static_cast<int>(transG[(pDst[0] & 0x3E0) >> 5][iG]);
                    dstB = static_cast<int>(transRB[(pDst[0] & 0x1F)][iB]);
                    *pDst = static_cast<uint16_t>((dstR << 10) | (dstG << 5) | dstB);
                }
            }
        }
    }
    else
    {
        for (int index = 0; index <= dy; index++)
        {
            error += dx;
            if (error > dy)
            {
                error -= dy;
                iResultX += x_inc;
            }
            iResultY += y_inc;
            if ((iResultX >= 0) && (iResultX < width) && (iResultY >= 0) && (iResultY < height))
            {
                uint16_t* pDst = pBuffer + iResultX + (iResultY * pitch);
                int dstR, dstG, dstB;
                if (pixelFormat == 1) // RGB565
                {
                    dstR = static_cast<int>(transRB[(pDst[0] & 0xF800) >> 11][iR]);
                    dstG = static_cast<int>(transG[(pDst[0] & 0x7E0) >> 5][iG]);
                    dstB = static_cast<int>(transRB[(pDst[0] & 0x1F)][iB]);
                    *pDst = static_cast<uint16_t>((dstR << 11) | (dstG << 5) | dstB);
                }
                else // RGB555
                {
                    dstR = static_cast<int>(transRB[(pDst[0] & 0x7C00) >> 10][iR]);
                    dstG = static_cast<int>(transG[(pDst[0] & 0x3E0) >> 5][iG]);
                    dstB = static_cast<int>(transRB[(pDst[0] & 0x1F)][iB]);
                    *pDst = static_cast<uint16_t>((dstR << 10) | (dstG << 5) | dstB);
                }
            }
        }
    }

    UnlockBackBuffer();
}

void DDrawRenderer::DrawFadeOverlay(float alpha)
{
    if (alpha <= 0.0f) return;  // Fully transparent, nothing to draw

    uint16_t* pDst = m_ddraw.m_pBackB4Addr;
    if (!pDst) return;

    int width = m_ddraw.res_x;
    int height = m_ddraw.res_y;
    int pitch = m_ddraw.m_sBackB4Pitch;

    // Clamp alpha to [0, 1]
    if (alpha > 1.0f) alpha = 1.0f;

    // Convert alpha to inverse multiplier (1.0 - alpha) as fixed point
    // We want to darken, so we multiply by (1 - alpha)
    // For alpha = 0: multiply by 1 (no change)
    // For alpha = 1: multiply by 0 (black)
    int invAlpha = static_cast<int>((1.0f - alpha) * 256.0f);

    // Fast path for fully opaque (black screen)
    if (alpha >= 1.0f)
    {
        for (int y = 0; y < height; y++)
        {
            memset(pDst, 0, width * sizeof(uint16_t));
            pDst += pitch;
        }
        return;
    }

    // Variable alpha: multiply each pixel's color channels
    // RGB565: RRRRRGGGGGGBBBBB (5-6-5 bits)
    // RGB555: 0RRRRRGGGGGBBBBB (5-5-5 bits)

    if (m_ddraw.m_cPixelFormat == 1) // RGB565
    {
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                uint16_t pixel = pDst[x];
                int r = (pixel >> 11) & 0x1F;
                int g = (pixel >> 5) & 0x3F;
                int b = pixel & 0x1F;

                r = (r * invAlpha) >> 8;
                g = (g * invAlpha) >> 8;
                b = (b * invAlpha) >> 8;

                pDst[x] = static_cast<uint16_t>((r << 11) | (g << 5) | b);
            }
            pDst += pitch;
        }
    }
    else // RGB555
    {
        for (int y = 0; y < height; y++)
        {
            for (int x = 0; x < width; x++)
            {
                uint16_t pixel = pDst[x];
                int r = (pixel >> 10) & 0x1F;
                int g = (pixel >> 5) & 0x1F;
                int b = pixel & 0x1F;

                r = (r * invAlpha) >> 8;
                g = (g * invAlpha) >> 8;
                b = (b * invAlpha) >> 8;

                pDst[x] = static_cast<uint16_t>((r << 10) | (g << 5) | b);
            }
            pDst += pitch;
        }
    }
}

void DDrawRenderer::DrawDarkRect(int x1, int y1, int x2, int y2, float alpha)
{
    if (alpha <= 0.0f) return;

    uint16_t* pBase = m_ddraw.m_pBackB4Addr;
    if (!pBase) return;

    int width = m_ddraw.res_x;
    int height = m_ddraw.res_y;
    int pitch = m_ddraw.m_sBackB4Pitch;

    // Clamp bounds
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 > width) x2 = width;
    if (y2 > height) y2 = height;
    if (x1 >= x2 || y1 >= y2) return;

    if (alpha > 1.0f) alpha = 1.0f;
    int invAlpha = static_cast<int>((1.0f - alpha) * 256.0f);

    if (alpha >= 1.0f)
    {
        uint16_t* pDst = pBase + y1 * pitch;
        for (int y = y1; y < y2; y++)
        {
            memset(&pDst[x1], 0, (x2 - x1) * sizeof(uint16_t));
            pDst += pitch;
        }
        return;
    }

    if (m_ddraw.m_cPixelFormat == 1) // RGB565
    {
        uint16_t* pDst = pBase + y1 * pitch;
        for (int y = y1; y < y2; y++)
        {
            for (int x = x1; x < x2; x++)
            {
                uint16_t pixel = pDst[x];
                int r = (pixel >> 11) & 0x1F;
                int g = (pixel >> 5) & 0x3F;
                int b = pixel & 0x1F;
                r = (r * invAlpha) >> 8;
                g = (g * invAlpha) >> 8;
                b = (b * invAlpha) >> 8;
                pDst[x] = static_cast<uint16_t>((r << 11) | (g << 5) | b);
            }
            pDst += pitch;
        }
    }
    else // RGB555
    {
        uint16_t* pDst = pBase + y1 * pitch;
        for (int y = y1; y < y2; y++)
        {
            for (int x = x1; x < x2; x++)
            {
                uint16_t pixel = pDst[x];
                int r = (pixel >> 10) & 0x1F;
                int g = (pixel >> 5) & 0x1F;
                int b = pixel & 0x1F;
                r = (r * invAlpha) >> 8;
                g = (g * invAlpha) >> 8;
                b = (b * invAlpha) >> 8;
                pDst[x] = static_cast<uint16_t>((r << 10) | (g << 5) | b);
            }
            pDst += pitch;
        }
    }
}

void DDrawRenderer::BeginTextBatch()
{
    // Delegate to TextLib - single point of font handling
    TextLib::ITextRenderer* pTextRenderer = TextLib::GetTextRenderer();
    if (pTextRenderer)
        pTextRenderer->BeginBatch();
}

void DDrawRenderer::EndTextBatch()
{
    // Delegate to TextLib - single point of font handling
    TextLib::ITextRenderer* pTextRenderer = TextLib::GetTextRenderer();
    if (pTextRenderer)
        pTextRenderer->EndBatch();
}

void DDrawRenderer::DrawText(int x, int y, const char* text, uint32_t color)
{
    if (!text)
        return;

    // Delegate to TextLib - single point of font handling
    TextLib::ITextRenderer* pTextRenderer = TextLib::GetTextRenderer();
    if (pTextRenderer)
        pTextRenderer->DrawText(x, y, text, color);
}

void DDrawRenderer::DrawTextRect(RECT* rect, const char* text, uint32_t color)
{
    if (!rect || !text)
        return;

    // Delegate to TextLib - single point of font handling
    TextLib::ITextRenderer* pTextRenderer = TextLib::GetTextRenderer();
    if (pTextRenderer)
    {
        int width = rect->right - rect->left;
        int height = rect->bottom - rect->top;
        pTextRenderer->DrawTextAligned(rect->left, rect->top, width, height, text, color,
                                        TextLib::Align::TopCenter);
    }
}

ITexture* DDrawRenderer::CreateTexture(uint16_t width, uint16_t height)
{
    LPDIRECTDRAWSURFACE7 surface = m_ddraw.pCreateOffScreenSurface(width, height);
    if (!surface)
        return nullptr;

    return new DDrawTexture(surface, width, height);
}

void DDrawRenderer::DestroyTexture(ITexture* texture)
{
    delete texture;
}

uint16_t* DDrawRenderer::LockBackBuffer(int* pitch)
{
    if (m_backBufferLocked)
        return m_ddraw.m_pBackB4Addr;

    // The back buffer is typically pre-locked in DXC_ddraw
    // Return the existing pointer
    if (pitch)
        *pitch = m_ddraw.m_sBackB4Pitch;

    return m_ddraw.m_pBackB4Addr;
}

void DDrawRenderer::UnlockBackBuffer()
{
    // DXC_ddraw manages back buffer locking internally
    // This is a no-op for the current implementation
}

void DDrawRenderer::SetClipArea(int x, int y, int w, int h)
{
    SetRect(&m_ddraw.m_rcClipArea, x, y, x + w, y + h);
}

RECT DDrawRenderer::GetClipArea() const
{
    return m_ddraw.m_rcClipArea;
}

int DDrawRenderer::GetPixelFormat() const
{
    return m_ddraw.m_cPixelFormat;
}

bool DDrawRenderer::Screenshot(const char* filename)
{
    return m_ddraw.Screenshot(filename, m_ddraw.m_lpBackB4);
}

int DDrawRenderer::GetWidth() const
{
    return m_ddraw.res_x;
}

int DDrawRenderer::GetHeight() const
{
    return m_ddraw.res_y;
}

int DDrawRenderer::GetWidthMid() const
{
    return m_ddraw.res_x_mid;
}

int DDrawRenderer::GetHeightMid() const
{
    return m_ddraw.res_y_mid;
}

void DDrawRenderer::ResizeBackBuffer(int width, int height)
{
    // DirectDraw handles resolution through ChangeDisplayMode, not dynamic resizing
    // This is a no-op for DDraw - the back buffer is recreated via ChangeDisplayMode
}

uint32_t DDrawRenderer::GetColorKey(ITexture* texture, uint16_t colorKey)
{
    if (!texture)
        return 0;

    LPDIRECTDRAWSURFACE7 surface = static_cast<LPDIRECTDRAWSURFACE7>(texture->GetNativeHandle());
    return m_ddraw._dwColorMatch(surface, colorKey);
}

uint32_t DDrawRenderer::GetColorKeyRGB(ITexture* texture, uint8_t r, uint8_t g, uint8_t b)
{
    if (!texture)
        return 0;

    LPDIRECTDRAWSURFACE7 surface = static_cast<LPDIRECTDRAWSURFACE7>(texture->GetNativeHandle());
    COLORREF rgb = RGB(r, g, b);
    return m_ddraw._dwColorMatch(surface, rgb);
}

void DDrawRenderer::SetColorKey(ITexture* texture, uint16_t colorKey)
{
    if (!texture)
        return;

    LPDIRECTDRAWSURFACE7 surface = static_cast<LPDIRECTDRAWSURFACE7>(texture->GetNativeHandle());
    m_ddraw.iSetColorKey(surface, colorKey);
}

void DDrawRenderer::SetColorKeyRGB(ITexture* texture, uint8_t r, uint8_t g, uint8_t b)
{
    if (!texture)
        return;

    LPDIRECTDRAWSURFACE7 surface = static_cast<LPDIRECTDRAWSURFACE7>(texture->GetNativeHandle());
    m_ddraw.iSetColorKey(surface, RGB(r, g, b));
}

// Transparency table getters
const long (*DDrawRenderer::GetTransTableRB100() const)[64]
{
    return G_lTransRB100;
}

const long (*DDrawRenderer::GetTransTableG100() const)[64]
{
    return G_lTransG100;
}

const long (*DDrawRenderer::GetTransTableRB70() const)[64]
{
    return G_lTransRB70;
}

const long (*DDrawRenderer::GetTransTableG70() const)[64]
{
    return G_lTransG70;
}

const long (*DDrawRenderer::GetTransTableRB50() const)[64]
{
    return G_lTransRB50;
}

const long (*DDrawRenderer::GetTransTableG50() const)[64]
{
    return G_lTransG50;
}

const long (*DDrawRenderer::GetTransTableRB25() const)[64]
{
    return G_lTransRB25;
}

const long (*DDrawRenderer::GetTransTableG25() const)[64]
{
    return G_lTransG25;
}

const long (*DDrawRenderer::GetTransTableRB2() const)[64]
{
    return G_lTransRB2;
}

const long (*DDrawRenderer::GetTransTableG2() const)[64]
{
    return G_lTransG2;
}

const long (*DDrawRenderer::GetTransTableFadeRB() const)[64]
{
    return m_ddraw.m_lFadeRB;
}

const long (*DDrawRenderer::GetTransTableFadeG() const)[64]
{
    return m_ddraw.m_lFadeG;
}

// Add table getters
const int (*DDrawRenderer::GetAddTable31() const)[510]
{
    return G_iAddTable31;
}

const int (*DDrawRenderer::GetAddTable63() const)[510]
{
    return G_iAddTable63;
}

const int (*DDrawRenderer::GetAddTransTable31() const)[64]
{
    return G_iAddTransTable31;
}

const int (*DDrawRenderer::GetAddTransTable63() const)[64]
{
    return G_iAddTransTable63;
}

char DDrawRenderer::GetSpriteAlphaDegree() const
{
    return G_cSpriteAlphaDegree;
}

void DDrawRenderer::SetSpriteAlphaDegree(char degree)
{
    G_cSpriteAlphaDegree = degree;
}

bool DDrawRenderer::EndFrameCheckLostSurface()
{
    HRESULT hr = m_ddraw.iFlip();
    return (hr == DDERR_SURFACELOST);
}

void DDrawRenderer::ColorTransferRGB(uint32_t rgb, int* outR, int* outG, int* outB)
{
    m_ddraw.ColorTransferRGB(static_cast<COLORREF>(rgb), outR, outG, outB);
}

int DDrawRenderer::GetTextLength(const char* text, int maxWidth)
{
    if (!text)
        return 0;

    // Delegate to TextLib - single point of font handling
    TextLib::ITextRenderer* pTextRenderer = TextLib::GetTextRenderer();
    if (pTextRenderer)
        return pTextRenderer->GetFittingCharCount(text, maxWidth);

    return 0;
}

int DDrawRenderer::GetTextWidth(const char* text)
{
    if (!text)
        return 0;

    // Delegate to TextLib - single point of font handling
    TextLib::ITextRenderer* pTextRenderer = TextLib::GetTextRenderer();
    if (pTextRenderer)
    {
        TextLib::TextMetrics metrics = pTextRenderer->MeasureText(text);
        return metrics.width;
    }

    return 0;
}

void* DDrawRenderer::GetBackBufferNative()
{
    return m_ddraw.m_lpBackB4;
}

void* DDrawRenderer::GetNativeRenderer()
{
    return &m_ddraw;
}

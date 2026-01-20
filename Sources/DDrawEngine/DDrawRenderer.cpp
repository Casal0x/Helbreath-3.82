// DDrawRenderer.cpp: DirectDraw renderer implementation
//
// Part of DDrawEngine static library
//////////////////////////////////////////////////////////////////////

#include "DDrawRenderer.h"

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
    : m_pdbgsWrapper(nullptr)
    , m_backBufferLocked(false)
{
}

DDrawRenderer::~DDrawRenderer()
{
    Shutdown();
}

bool DDrawRenderer::Init(HWND hWnd)
{
    return m_ddraw.bInit(hWnd);
}

void DDrawRenderer::Shutdown()
{
    if (m_pdbgsWrapper)
    {
        // Don't delete the wrapper's surface - it's owned by m_ddraw
        delete m_pdbgsWrapper;
        m_pdbgsWrapper = nullptr;
    }
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

void DDrawRenderer::ChangeDisplayMode(HWND hWnd)
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

void DDrawRenderer::BeginTextBatch()
{
    m_ddraw._GetBackBufferDC();
}

void DDrawRenderer::EndTextBatch()
{
    m_ddraw._ReleaseBackBufferDC();
}

void DDrawRenderer::DrawText(int x, int y, const char* text, uint32_t color)
{
    m_ddraw.TextOut(x, y, const_cast<char*>(text), static_cast<COLORREF>(color));
}

void DDrawRenderer::DrawTextRect(RECT* rect, const char* text, uint32_t color)
{
    m_ddraw.DrawText(rect, text, static_cast<COLORREF>(color));
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

ITexture* DDrawRenderer::GetBackgroundSurface()
{
    if (!m_pdbgsWrapper && m_ddraw.m_lpPDBGS)
    {
        // Create a wrapper that doesn't own the surface
        // Note: This is a bit of a hack - the wrapper won't release the surface
        // because PDBGS is owned by DXC_ddraw
        m_pdbgsWrapper = new DDrawTexture(m_ddraw.m_lpPDBGS,
                                           static_cast<uint16_t>(m_ddraw.res_x),
                                           static_cast<uint16_t>(m_ddraw.res_y));
    }
    return m_pdbgsWrapper;
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
    // Get text length that fits within maxWidth pixels
    // This uses the DC which should be acquired via BeginTextBatch first
    if (!m_ddraw.m_hDC || !text)
        return 0;

    SIZE size;
    int len = static_cast<int>(strlen(text));

    for (int i = len; i > 0; i--)
    {
        GetTextExtentPoint32A(m_ddraw.m_hDC, text, i, &size);
        if (size.cx <= maxWidth)
            return i;
    }
    return 0;
}

int DDrawRenderer::GetTextWidth(const char* text)
{
    // Get pixel width of text string
    // This uses the DC which should be acquired via BeginTextBatch first
    if (!m_ddraw.m_hDC || !text)
        return 0;

    SIZE size;
    GetTextExtentPoint32A(m_ddraw.m_hDC, text, static_cast<int>(strlen(text)), &size);
    return size.cx;
}

void DDrawRenderer::BltBackBufferFromPDBGS(RECT* srcRect)
{
    if (!m_ddraw.m_lpBackB4 || !m_ddraw.m_lpPDBGS)
        return;

    // Blit from srcRect in PDBGS to (0, 0) in back buffer
    m_ddraw.m_lpBackB4->BltFast(0, 0, m_ddraw.m_lpPDBGS, srcRect, DDBLTFAST_NOCOLORKEY | DDBLTFAST_WAIT);
}

void* DDrawRenderer::GetBackBufferNative()
{
    return m_ddraw.m_lpBackB4;
}

void* DDrawRenderer::GetPDBGSNative()
{
    return m_ddraw.m_lpPDBGS;
}

void* DDrawRenderer::GetNativeRenderer()
{
    return &m_ddraw;
}

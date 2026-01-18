// DDrawTexture.cpp: DirectDraw surface wrapper implementation
//
// Part of DDrawEngine static library
//////////////////////////////////////////////////////////////////////

#include "DDrawTexture.h"

DDrawTexture::DDrawTexture(LPDIRECTDRAWSURFACE7 surface, uint16_t width, uint16_t height)
    : m_surface(surface)
    , m_width(width)
    , m_height(height)
    , m_locked(false)
{
}

DDrawTexture::~DDrawTexture()
{
    if (m_surface)
    {
        if (m_locked)
        {
            m_surface->Unlock(nullptr);
        }
        m_surface->Release();
        m_surface = nullptr;
    }
}

void DDrawTexture::SetColorKey(uint16_t colorKey)
{
    if (m_surface)
    {
        DDCOLORKEY ddck;
        ddck.dwColorSpaceLowValue = colorKey;
        ddck.dwColorSpaceHighValue = colorKey;
        m_surface->SetColorKey(DDCKEY_SRCBLT, &ddck);
    }
}

void DDrawTexture::SetColorKeyRGB(uint8_t r, uint8_t g, uint8_t b)
{
    // Convert RGB to 16-bit color (assuming RGB565)
    uint16_t colorKey = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
    SetColorKey(colorKey);
}

uint16_t* DDrawTexture::Lock(int* pitch)
{
    if (!m_surface || m_locked)
        return nullptr;

    DDSURFACEDESC2 ddsd;
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);

    HRESULT hr = m_surface->Lock(nullptr, &ddsd, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, nullptr);
    if (FAILED(hr))
        return nullptr;

    m_locked = true;
    if (pitch)
        *pitch = ddsd.lPitch / 2;  // Return in pixels (16-bit), not bytes

    return static_cast<uint16_t*>(ddsd.lpSurface);
}

void DDrawTexture::Unlock()
{
    if (m_surface && m_locked)
    {
        m_surface->Unlock(nullptr);
        m_locked = false;
    }
}

bool DDrawTexture::Blt(RECT* destRect, ITexture* src, RECT* srcRect, uint32_t flags)
{
    if (!m_surface)
        return false;

    DWORD ddFlags = 0;
    if (flags & TextureFlags::WAIT)
        ddFlags |= DDBLT_WAIT;
    if (flags & TextureFlags::SRCCOLORKEY)
        ddFlags |= DDBLT_KEYSRC;
    if (flags & TextureFlags::DSTCOLORKEY)
        ddFlags |= DDBLT_KEYDEST;

    LPDIRECTDRAWSURFACE7 srcSurface = nullptr;
    if (src)
    {
        srcSurface = static_cast<LPDIRECTDRAWSURFACE7>(src->GetNativeHandle());
    }

    HRESULT hr = m_surface->Blt(destRect, srcSurface, srcRect, ddFlags, nullptr);
    return SUCCEEDED(hr);
}

bool DDrawTexture::BltFast(int x, int y, ITexture* src, RECT* srcRect, uint32_t flags)
{
    if (!m_surface || !src)
        return false;

    DWORD ddFlags = 0;
    if (flags & TextureFlags::WAIT)
        ddFlags |= DDBLTFAST_WAIT;
    if (flags & TextureFlags::SRCCOLORKEY)
        ddFlags |= DDBLTFAST_SRCCOLORKEY;
    if (flags & TextureFlags::DSTCOLORKEY)
        ddFlags |= DDBLTFAST_DESTCOLORKEY;
    if (flags & TextureFlags::NOCOLORKEY)
        ddFlags |= DDBLTFAST_NOCOLORKEY;

    LPDIRECTDRAWSURFACE7 srcSurface = static_cast<LPDIRECTDRAWSURFACE7>(src->GetNativeHandle());

    HRESULT hr = m_surface->BltFast(x, y, srcSurface, srcRect, ddFlags);
    return SUCCEEDED(hr);
}

bool DDrawTexture::IsLost() const
{
    if (!m_surface)
        return true;

    return m_surface->IsLost() == DDERR_SURFACELOST;
}

bool DDrawTexture::Restore()
{
    if (!m_surface)
        return false;

    HRESULT hr = m_surface->Restore();
    return SUCCEEDED(hr);
}

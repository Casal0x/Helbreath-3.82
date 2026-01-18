// DDrawTexture.h: DirectDraw surface wrapper implementing ITexture
//
// Part of DDrawEngine static library
//////////////////////////////////////////////////////////////////////

#pragma once

#include "ITexture.h"
#include "ddraw.h"

class DDrawTexture : public ITexture
{
public:
    DDrawTexture(LPDIRECTDRAWSURFACE7 surface, uint16_t width, uint16_t height);
    virtual ~DDrawTexture();

    // ITexture interface implementation
    virtual uint16_t GetWidth() const override { return m_width; }
    virtual uint16_t GetHeight() const override { return m_height; }

    virtual void SetColorKey(uint16_t colorKey) override;
    virtual void SetColorKeyRGB(uint8_t r, uint8_t g, uint8_t b) override;

    virtual uint16_t* Lock(int* pitch) override;
    virtual void Unlock() override;

    virtual bool Blt(RECT* destRect, ITexture* src, RECT* srcRect, uint32_t flags) override;
    virtual bool BltFast(int x, int y, ITexture* src, RECT* srcRect, uint32_t flags) override;

    virtual void* GetNativeHandle() override { return m_surface; }

    virtual bool IsLost() const override;
    virtual bool Restore() override;

    // DirectDraw-specific accessors
    LPDIRECTDRAWSURFACE7 GetSurface() const { return m_surface; }

private:
    LPDIRECTDRAWSURFACE7 m_surface;
    uint16_t m_width;
    uint16_t m_height;
    bool m_locked;
};

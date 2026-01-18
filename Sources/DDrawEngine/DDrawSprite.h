// DDrawSprite.h: DirectDraw implementation of ISprite
//
// Wraps legacy DDraw sprite rendering with the modern ISprite interface.
// Uses PAKLib for PAK file reading.
//////////////////////////////////////////////////////////////////////

#pragma once

#include "ISprite.h"
#include "DXC_ddraw.h"
#include "PAK.h"
#include <string>
#include <vector>
#include <cstdint>

class DDrawSprite : public SpriteLib::ISprite {
public:
    // Construction from file path (opens file, reads sprite data)
    DDrawSprite(DXC_ddraw* pDDraw, const std::string& pakFilePath, int spriteIndex, bool alphaEffect = true);

    // Construction from pre-loaded PAK data (no file I/O - used by SpriteLoader)
    DDrawSprite(DXC_ddraw* pDDraw, const PAKLib::sprite& spriteData, bool alphaEffect = true);

    virtual ~DDrawSprite();

    //------------------------------------------------------------------
    // ISprite Implementation
    //------------------------------------------------------------------

    // Core drawing
    void Draw(int x, int y, int frame, const SpriteLib::DrawParams& params = SpriteLib::DrawParams{}) override;
    void DrawToSurface(void* destSurface, int x, int y, int frame, const SpriteLib::DrawParams& params = SpriteLib::DrawParams{}) override;
    void DrawWidth(int x, int y, int frame, int width, bool vertical = false) override;
    void DrawShifted(int x, int y, int shiftX, int shiftY, int frame, const SpriteLib::DrawParams& params = SpriteLib::DrawParams{}) override;

    // Frame information
    int GetFrameCount() const override;
    SpriteLib::SpriteRect GetFrameRect(int frame) const override;
    void GetBoundingRect(int x, int y, int frame, int& left, int& top, int& right, int& bottom) override;
    void CalculateBounds(int x, int y, int frame) override;
    bool GetLastDrawBounds(int& left, int& top, int& right, int& bottom) const override;
    SpriteLib::BoundRect GetBoundRect() const override;

    // Collision detection
    bool CheckCollision(int spriteX, int spriteY, int frame, int pointX, int pointY) override;

    // Resource management
    void Preload() override;
    void Unload() override;
    bool IsLoaded() const override;
    void Restore() override;
    bool IsInUse() const override;
    uint32_t GetLastAccessTime() const override;

    //------------------------------------------------------------------
    // DDraw-Specific Methods (for legacy compatibility during transition)
    //------------------------------------------------------------------

    // Get the underlying DDraw surface (for code that still needs direct access)
    LPDIRECTDRAWSURFACE7 GetSurface() const { return m_lpSurface; }

    // Get surface pixel address (for direct pixel manipulation)
    uint16_t* GetSurfaceAddr() const { return m_pSurfaceAddr; }
    int16_t GetPitch() const { return m_sPitch; }

    // Get color key
    uint16_t GetColorKey() const { return m_wColorKey; }

    // Alpha degree management (legacy feature)
    void SetAlphaDegree(char degree);
    char GetAlphaDegree() const { return m_cAlphaDegree; }
    bool HasAlphaEffect() const { return m_bAlphaEffect; }

    // Bounding rect from last draw (legacy feature)
    const RECT& GetLastBoundRect() const { return m_rcBound; }

private:
    //------------------------------------------------------------------
    // Internal Drawing Methods
    //------------------------------------------------------------------

    // Clip coordinates and return false if completely clipped
    bool ClipCoordinates(int& dX, int& dY, int& sx, int& sy, int& szx, int& szy) const;

    // Hardware blitting (fast path)
    void DrawBltFast(int x, int y, int frame, bool useColorKey, LPDIRECTDRAWSURFACE7 destSurface = nullptr);

    // CPU-based transparent drawing
    void DrawTransparent(int x, int y, int frame, float alpha, bool useColorKey);
    void DrawShiftedTransparent(int x, int y, int shiftX, int shiftY, int frame, float alpha, bool useColorKey);

    // CPU-based tinted drawing
    void DrawTinted(int x, int y, int frame, int16_t r, int16_t g, int16_t b, bool useColorKey);

    // CPU-based transparent + tinted drawing
    void DrawTintedTransparent(int x, int y, int frame, int16_t r, int16_t g, int16_t b, float alpha, bool useColorKey);

    // CPU-based shadow drawing
    void DrawShadowInternal(int x, int y, int frame, bool clipped);

    // CPU-based fade drawing
    void DrawFadeInternal(int x, int y, int frame, uint16_t* pDestAddr = nullptr, int16_t destPitch = 0);

    //------------------------------------------------------------------
    // Surface Management
    //------------------------------------------------------------------

    bool CreateSurface();
    void ReleaseSurface();
    void ApplyAlphaDegree();

    //------------------------------------------------------------------
    // Member Variables
    //------------------------------------------------------------------

    // DDraw reference
    DXC_ddraw* m_pDDraw;

    // PAK file info
    std::string m_pakFilePath;
    int m_spriteIndex;

    // Frame data from PAK (loaded once in constructor)
    std::vector<PAKLib::sprite_rect> m_frames;

    // Image data from PAK (kept for surface recreation)
    std::vector<uint8_t> m_imageData;
    uint16_t m_wBitmapSizeX;
    uint16_t m_wBitmapSizeY;

    // DDraw surface
    LPDIRECTDRAWSURFACE7 m_lpSurface;
    uint16_t* m_pSurfaceAddr;
    int16_t m_sPitch;
    uint16_t m_wColorKey;
    bool m_bSurfaceLoaded;

    // Alpha effect
    bool m_bAlphaEffect;
    char m_cAlphaDegree;

    // State
    bool m_bOnCriticalSection;
    RECT m_rcBound;
    int16_t m_sPivotX;
    int16_t m_sPivotY;
    uint32_t m_dwRefTime;
};

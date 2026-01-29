// DDrawTextRenderer.cpp: DirectDraw implementation of ITextRenderer
//
// Part of DDrawEngine static library
//////////////////////////////////////////////////////////////////////

#include "DDrawTextRenderer.h"
#include "DXC_ddraw.h"
#include <cstring>

namespace TextLib {

// Global accessor implementation
static ITextRenderer* s_pTextRenderer = nullptr;

ITextRenderer* GetTextRenderer()
{
    return s_pTextRenderer;
}

void SetTextRenderer(ITextRenderer* renderer)
{
    s_pTextRenderer = renderer;
}

DDrawTextRenderer::DDrawTextRenderer(DXC_ddraw* ddraw)
    : m_pDDraw(ddraw)
    , m_hFont(nullptr)
    , m_hOldFont(nullptr)
    , m_fontSize(12)
    , m_batchActive(false)
    , m_fontLoaded(false)
{
    m_fontName[0] = '\0';

    // Load default font
    LoadFontByName("Tahoma");
}

DDrawTextRenderer::~DDrawTextRenderer()
{
    DestroyGDIFont();
}

void DDrawTextRenderer::CreateGDIFont()
{
    DestroyGDIFont();

    if (m_fontName[0] == '\0')
        return;

    m_hFont = CreateFontA(
        -m_fontSize,            // Height (negative for character height)
        0,                      // Width (0 = default)
        0,                      // Escapement
        0,                      // Orientation
        FW_NORMAL,              // Weight
        FALSE,                  // Italic
        FALSE,                  // Underline
        FALSE,                  // StrikeOut
        DEFAULT_CHARSET,        // CharSet
        OUT_DEFAULT_PRECIS,     // OutPrecision
        CLIP_DEFAULT_PRECIS,    // ClipPrecision
        ANTIALIASED_QUALITY,    // Quality
        DEFAULT_PITCH | FF_DONTCARE,  // PitchAndFamily
        m_fontName              // Face name
    );

    m_fontLoaded = (m_hFont != nullptr);
}

void DDrawTextRenderer::DestroyGDIFont()
{
    if (m_hFont)
    {
        DeleteObject(m_hFont);
        m_hFont = nullptr;
    }
    m_fontLoaded = false;
}

bool DDrawTextRenderer::LoadFontFromFile(const char* fontPath)
{
    if (!fontPath)
        return false;

    // Add the font resource to Windows
    int result = AddFontResourceExA(fontPath, FR_PRIVATE, nullptr);
    if (result == 0)
        return false;

    // Extract font name from path (simple extraction - takes filename without extension)
    const char* filename = strrchr(fontPath, '\\');
    if (!filename)
        filename = strrchr(fontPath, '/');
    if (filename)
        filename++;  // Skip the slash
    else
        filename = fontPath;

    // Copy and remove extension
    strncpy_s(m_fontName, filename, sizeof(m_fontName) - 1);
    char* dot = strrchr(m_fontName, '.');
    if (dot)
        *dot = '\0';

    CreateGDIFont();
    return m_fontLoaded;
}

bool DDrawTextRenderer::LoadFontByName(const char* fontName)
{
    if (!fontName)
        return false;

    strncpy_s(m_fontName, fontName, sizeof(m_fontName) - 1);
    m_fontName[sizeof(m_fontName) - 1] = '\0';

    CreateGDIFont();
    return m_fontLoaded;
}

void DDrawTextRenderer::SetFontSize(int size)
{
    if (m_fontSize != size)
    {
        m_fontSize = size;
        if (m_fontName[0] != '\0')
        {
            CreateGDIFont();
        }
    }
}

bool DDrawTextRenderer::IsFontLoaded() const
{
    return m_fontLoaded;
}

TextMetrics DDrawTextRenderer::MeasureText(const char* text) const
{
    TextMetrics metrics = {0, 0};

    if (!text || text[0] == '\0' || !m_pDDraw)
        return metrics;

    // Acquire DC for measurement (same as BeginTextBatch)
    m_pDDraw->_GetBackBufferDC();

    // The DC and font are now set up by _GetBackBufferDC
    // m_hFontInUse is selected into m_hDC
    SIZE size;
    if (GetTextExtentPoint32A(m_pDDraw->m_hDC, text, static_cast<int>(strlen(text)), &size))
    {
        metrics.width = size.cx;
        metrics.height = size.cy;
    }

    // Release DC (same as EndTextBatch)
    m_pDDraw->_ReleaseBackBufferDC();

    return metrics;
}

int DDrawTextRenderer::GetFittingCharCount(const char* text, int maxWidth) const
{
    if (!text || !m_pDDraw || !m_pDDraw->m_hDC)
        return 0;

    // Select our font temporarily
    HFONT hOldFont = nullptr;
    if (m_hFont)
    {
        hOldFont = (HFONT)SelectObject(m_pDDraw->m_hDC, m_hFont);
    }

    int len = static_cast<int>(strlen(text));
    SIZE size;

    int result = 0;
    for (int i = len; i > 0; i--)
    {
        if (GetTextExtentPoint32A(m_pDDraw->m_hDC, text, i, &size))
        {
            if (size.cx <= maxWidth)
            {
                result = i;
                break;
            }
        }
    }

    // Restore old font
    if (hOldFont)
    {
        SelectObject(m_pDDraw->m_hDC, hOldFont);
    }

    return result;
}

void DDrawTextRenderer::DrawText(int x, int y, const char* text, uint32_t color)
{
    if (!text || !m_pDDraw || !m_pDDraw->m_hDC)
        return;

    // Select our font
    HFONT hOldFont = nullptr;
    if (m_hFont)
    {
        hOldFont = (HFONT)SelectObject(m_pDDraw->m_hDC, m_hFont);
    }

    SetTextColor(m_pDDraw->m_hDC, static_cast<COLORREF>(color));
    SetBkMode(m_pDDraw->m_hDC, TRANSPARENT);
    TextOutA(m_pDDraw->m_hDC, x, y, text, static_cast<int>(strlen(text)));

    // Restore old font
    if (hOldFont)
    {
        SelectObject(m_pDDraw->m_hDC, hOldFont);
    }
}

void DDrawTextRenderer::DrawTextCentered(int x1, int x2, int y, const char* text, uint32_t color)
{
    if (!text || !m_pDDraw || !m_pDDraw->m_hDC)
        return;

    // Select our font
    HFONT hOldFont = nullptr;
    if (m_hFont)
    {
        hOldFont = (HFONT)SelectObject(m_pDDraw->m_hDC, m_hFont);
    }

    RECT rect;
    rect.left = x1;
    rect.top = y;
    rect.right = x2;
    rect.bottom = y + 20;  // Approximate height, DT_NOCLIP is used anyway

    SetTextColor(m_pDDraw->m_hDC, static_cast<COLORREF>(color));
    SetBkMode(m_pDDraw->m_hDC, TRANSPARENT);
    ::DrawTextA(m_pDDraw->m_hDC, text, static_cast<int>(strlen(text)), &rect,
                DT_CENTER | DT_NOCLIP | DT_WORDBREAK | DT_NOPREFIX);

    // Restore old font
    if (hOldFont)
    {
        SelectObject(m_pDDraw->m_hDC, hOldFont);
    }
}

void DDrawTextRenderer::BeginBatch()
{
    if (!m_batchActive && m_pDDraw)
    {
        m_pDDraw->_GetBackBufferDC();
        m_batchActive = true;
    }
}

void DDrawTextRenderer::EndBatch()
{
    if (m_batchActive && m_pDDraw)
    {
        m_pDDraw->_ReleaseBackBufferDC();
        m_batchActive = false;
    }
}

} // namespace TextLib

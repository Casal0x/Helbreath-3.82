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
    , m_hOldFont(nullptr)
    , m_fontSize(16)  // Match original game font size
    , m_batchDepth(0)
    , m_fontLoaded(false)
{
    m_fontName[0] = '\0';

    // Load default font
    LoadFontByName("Tahoma");
}

DDrawTextRenderer::~DDrawTextRenderer()
{
    ClearFontCache();
}

HFONT DDrawTextRenderer::GetOrCreateFont(int size)
{
    if (m_fontName[0] == '\0')
        return nullptr;

    // Check cache first
    auto it = m_fontCache.find(size);
    if (it != m_fontCache.end())
        return it->second;

    // Create new font for this size (matching original game settings)
    HFONT hFont = CreateFontA(
        size,                   // Height (positive = cell height, like original)
        0,                      // Width (0 = default)
        0,                      // Escapement
        0,                      // Orientation
        FW_NORMAL,              // Weight
        FALSE,                  // Italic
        FALSE,                  // Underline
        FALSE,                  // StrikeOut
        ANSI_CHARSET,           // CharSet (match original)
        OUT_DEFAULT_PRECIS,     // OutPrecision
        CLIP_DEFAULT_PRECIS,    // ClipPrecision
        NONANTIALIASED_QUALITY, // Quality (match original)
        VARIABLE_PITCH,         // PitchAndFamily (match original)
        m_fontName              // Face name
    );

    if (hFont)
    {
        m_fontCache[size] = hFont;
        m_fontLoaded = true;
    }

    return hFont;
}

void DDrawTextRenderer::ClearFontCache()
{
    for (auto& pair : m_fontCache)
    {
        if (pair.second)
            DeleteObject(pair.second);
    }
    m_fontCache.clear();
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

    // Clear cache since font changed, then create for current size
    ClearFontCache();
    GetOrCreateFont(m_fontSize);
    return m_fontLoaded;
}

bool DDrawTextRenderer::LoadFontByName(const char* fontName)
{
    if (!fontName)
        return false;

    strncpy_s(m_fontName, fontName, sizeof(m_fontName) - 1);
    m_fontName[sizeof(m_fontName) - 1] = '\0';

    // Clear cache since font changed, then create for current size
    ClearFontCache();
    GetOrCreateFont(m_fontSize);
    return m_fontLoaded;
}

void DDrawTextRenderer::SetFontSize(int size)
{
    // Just update the current size - fonts are lazy loaded/cached on draw
    m_fontSize = size;
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

    // Auto-batch if not already in a batch
    DDrawTextRenderer* pThis = const_cast<DDrawTextRenderer*>(this);
    bool needEndBatch = (m_batchDepth == 0);
    if (needEndBatch)
        pThis->BeginBatch();

    // Get the font for current size (cast away const for cache access)
    HFONT hFont = pThis->GetOrCreateFont(m_fontSize);

    HFONT hOldFont = nullptr;
    if (hFont)
    {
        hOldFont = (HFONT)SelectObject(m_pDDraw->m_hDC, hFont);
    }

    SIZE size;
    if (GetTextExtentPoint32A(m_pDDraw->m_hDC, text, static_cast<int>(strlen(text)), &size))
    {
        metrics.width = size.cx;
        metrics.height = size.cy;
    }

    if (hOldFont)
    {
        SelectObject(m_pDDraw->m_hDC, hOldFont);
    }

    if (needEndBatch)
        pThis->EndBatch();

    return metrics;
}

int DDrawTextRenderer::GetFittingCharCount(const char* text, int maxWidth) const
{
    if (!text || !m_pDDraw)
        return 0;

    // Auto-batch if not already in a batch
    DDrawTextRenderer* pThis = const_cast<DDrawTextRenderer*>(this);
    bool needEndBatch = (m_batchDepth == 0);
    if (needEndBatch)
        pThis->BeginBatch();

    // Get the font for current size (cast away const for cache access)
    HFONT hFont = pThis->GetOrCreateFont(m_fontSize);

    // Select our font temporarily
    HFONT hOldFont = nullptr;
    if (hFont)
    {
        hOldFont = (HFONT)SelectObject(m_pDDraw->m_hDC, hFont);
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

    if (needEndBatch)
        pThis->EndBatch();

    return result;
}

void DDrawTextRenderer::DrawText(int x, int y, const char* text, uint32_t color)
{
    if (!text || !m_pDDraw)
        return;

    // Auto-batch if not already in a batch
    bool needEndBatch = (m_batchDepth == 0);
    if (needEndBatch)
        BeginBatch();

    // Get the font for current size (lazy load/cached)
    HFONT hFont = GetOrCreateFont(m_fontSize);

    // Select our font
    HFONT hOldFont = nullptr;
    if (hFont)
    {
        hOldFont = (HFONT)SelectObject(m_pDDraw->m_hDC, hFont);
    }

    SetTextColor(m_pDDraw->m_hDC, static_cast<COLORREF>(color));
    SetBkMode(m_pDDraw->m_hDC, TRANSPARENT);
    TextOutA(m_pDDraw->m_hDC, x, y, text, static_cast<int>(strlen(text)));

    // Restore old font
    if (hOldFont)
    {
        SelectObject(m_pDDraw->m_hDC, hOldFont);
    }

    if (needEndBatch)
        EndBatch();
}

void DDrawTextRenderer::DrawTextAligned(int x, int y, int width, int height, const char* text, uint32_t color,
                                         Align alignment)
{
    if (!text || !m_pDDraw)
        return;

    // Auto-batch if not already in a batch
    bool needEndBatch = (m_batchDepth == 0);
    if (needEndBatch)
        BeginBatch();

    // Get the font for current size (lazy load/cached)
    HFONT hFont = GetOrCreateFont(m_fontSize);

    // Select our font
    HFONT hOldFont = nullptr;
    if (hFont)
    {
        hOldFont = (HFONT)SelectObject(m_pDDraw->m_hDC, hFont);
    }

    RECT rect;
    rect.left = x;
    rect.top = y;
    rect.right = x + width;
    rect.bottom = y + height;

    // Build DrawText flags based on alignment
    UINT dtFlags = DT_NOCLIP | DT_WORDBREAK | DT_NOPREFIX;

    // Horizontal alignment
    uint8_t hAlign = alignment & Align::HMask;
    if (hAlign == Align::HCenter)
        dtFlags |= DT_CENTER;
    else if (hAlign == Align::Right)
        dtFlags |= DT_RIGHT;
    else
        dtFlags |= DT_LEFT;

    // Vertical alignment
    uint8_t vAlign = alignment & Align::VMask;
    if (vAlign == Align::VCenter)
        dtFlags |= DT_VCENTER | DT_SINGLELINE;
    else if (vAlign == Align::Bottom)
        dtFlags |= DT_BOTTOM | DT_SINGLELINE;
    else
        dtFlags |= DT_TOP;

    SetTextColor(m_pDDraw->m_hDC, static_cast<COLORREF>(color));
    SetBkMode(m_pDDraw->m_hDC, TRANSPARENT);
    ::DrawTextA(m_pDDraw->m_hDC, text, static_cast<int>(strlen(text)), &rect, dtFlags);

    // Restore old font
    if (hOldFont)
    {
        SelectObject(m_pDDraw->m_hDC, hOldFont);
    }

    if (needEndBatch)
        EndBatch();
}

void DDrawTextRenderer::BeginBatch()
{
    if (m_pDDraw)
    {
        if (m_batchDepth == 0)
        {
            // First batch - acquire DC
            m_pDDraw->_GetBackBufferDC();
        }
        m_batchDepth++;
    }
}

void DDrawTextRenderer::EndBatch()
{
    if (m_pDDraw && m_batchDepth > 0)
    {
        m_batchDepth--;
        if (m_batchDepth == 0)
        {
            // Last batch ended - release DC
            m_pDDraw->_ReleaseBackBufferDC();
        }
    }
}

} // namespace TextLib

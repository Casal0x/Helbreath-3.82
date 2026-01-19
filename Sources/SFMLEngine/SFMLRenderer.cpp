// SFMLRenderer.cpp: SFML renderer implementation
//
// Part of SFMLEngine static library
//////////////////////////////////////////////////////////////////////

#include "SFMLRenderer.h"
#include "SFMLWindow.h"
#include "RendererFactory.h"
#include "PixelConversion.h"
#include "RenderConstants.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/OpenGL.hpp>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#endif

// Static dummy tables
long SFMLRenderer::s_dummyTransTable[64][64] = {};
int SFMLRenderer::s_dummyAddTable[64][510] = {};
int SFMLRenderer::s_dummyAddTransTable[510][64] = {};

SFMLRenderer::SFMLRenderer()
    : m_pRenderWindow(nullptr)
    , m_texturesCreated(false)
    , m_pdbgsWrapper(nullptr)
    , m_fontLoaded(false)
    , m_backBufferLocked(false)
    , m_width(RENDER_LOGICAL_WIDTH)
    , m_height(RENDER_LOGICAL_HEIGHT)
    , m_fullscreen(false)
    , m_spriteAlphaDegree(1)
{
    SetRect(&m_clipArea, 0, 0, m_width, m_height);
    InitDummyTables();
}

SFMLRenderer::~SFMLRenderer()
{
    Shutdown();
}

void SFMLRenderer::InitDummyTables()
{
    // Initialize transparency tables with identity-like values
    // These are dummy values since SFML uses GPU blending
    for (int i = 0; i < 64; i++)
    {
        for (int j = 0; j < 64; j++)
        {
            // Simple identity: return the source value
            s_dummyTransTable[i][j] = i;
        }
    }

    for (int i = 0; i < 64; i++)
    {
        for (int j = 0; j < 510; j++)
        {
            s_dummyAddTable[i][j] = (i + j > 63) ? 63 : (i + j);
        }
    }

    for (int i = 0; i < 510; i++)
    {
        for (int j = 0; j < 64; j++)
        {
            s_dummyAddTransTable[i][j] = (i + j > 63) ? 63 : (i + j);
        }
    }
}

bool SFMLRenderer::LoadFont()
{
    if (m_fontLoaded)
        return true;

    // Fallback to Windows system fonts if client didn't load a font
    const char* fontPaths[] = {
        "C:\\Windows\\Fonts\\tahoma.ttf",
        "C:\\Windows\\Fonts\\arial.ttf",
        "C:\\Windows\\Fonts\\segoeui.ttf"
    };

    for (const char* path : fontPaths)
    {
        char debugMsg[256];
        sprintf_s(debugMsg, "Trying to load font: %s\n", path);
        printf(debugMsg);

        if (m_font.openFromFile(path))
        {
            // Disable texture smoothing for pixel-perfect rendering to match DDraw/GDI
            m_font.setSmooth(false);
            m_fontLoaded = true;
            sprintf_s(debugMsg, "Font loaded successfully from: %s\n", path);
            printf(debugMsg);
            return true;
        }
        else
        {
            printf("  - Failed to load\n");
        }
    }

    printf("ERROR: Failed to load any font!\n");
    return false;
}

bool SFMLRenderer::Init(HWND hWnd)
{
    printf("SFMLRenderer::Init() called\n");

    // Note: RenderTextures will be created when SetRenderWindow is called,
    // because they require an active OpenGL context from the window.
    // For now, just return success - the textures will be created lazily.

    // Load font for text rendering (this doesn't need OpenGL context)
    if (LoadFont())
        printf("Font loaded successfully\n");
    else
        printf("Warning: Failed to load font\n");

    printf("SFMLRenderer::Init() returning true\n");
    return true;
}

bool SFMLRenderer::CreateRenderTextures()
{
    if (m_texturesCreated)
        return true;

    printf("SFMLRenderer::CreateRenderTextures() - Starting\n");

    // Create back buffer render texture
    char debugMsg[256];
    sprintf_s(debugMsg, "Creating back buffer: %dx%d\n", m_width, m_height);
    printf(debugMsg);

    if (!m_backBuffer.resize({static_cast<unsigned int>(m_width), static_cast<unsigned int>(m_height)}))
    {
        printf("ERROR: Failed to create back buffer render texture!\n");
        return false;
    }
    printf("Back buffer created successfully\n");

    // Create PDBGS (Pre-Draw Background Surface)
    // PDBGS must be larger than visible area for smooth tile scrolling
    // Game draws tiles with 32-pixel buffer zone and copies with offset
    sprintf_s(debugMsg, "Creating PDBGS: %dx%d\n", PDBGS_WIDTH, PDBGS_HEIGHT);
    printf(debugMsg);
    if (!m_pdbgs.resize({static_cast<unsigned int>(PDBGS_WIDTH), static_cast<unsigned int>(PDBGS_HEIGHT)}))
    {
        printf("ERROR: Failed to create PDBGS render texture!\n");
        return false;
    }
    printf("PDBGS created successfully\n");

    // Disable texture smoothing for pixel-perfect scaling (matches DDraw sharpness)
    // This uses nearest-neighbor filtering instead of bilinear
    m_backBuffer.setSmooth(false);
    m_pdbgs.setSmooth(false);

    // Disable texture repeating to prevent edge artifacts
    m_backBuffer.setRepeated(false);
    m_pdbgs.setRepeated(false);

    // Set explicit views to ensure 1:1 pixel mapping and prevent edge artifacts
    // The view must match exactly the surface dimensions
    sf::View backBufferView(sf::FloatRect({0.f, 0.f}, {static_cast<float>(m_width), static_cast<float>(m_height)}));
    m_backBuffer.setView(backBufferView);

    // PDBGS has its own larger view
    sf::View pdbgsView(sf::FloatRect({0.f, 0.f}, {static_cast<float>(PDBGS_WIDTH), static_cast<float>(PDBGS_HEIGHT)}));
    m_pdbgs.setView(pdbgsView);

    // Clear both buffers - use transparent for proper alpha compositing
    m_backBuffer.clear(sf::Color::Transparent);
    m_backBuffer.display();
    m_pdbgs.clear(sf::Color::Transparent);
    m_pdbgs.display();

    m_texturesCreated = true;
    printf("SFMLRenderer::CreateRenderTextures() - Complete\n");
    return true;
}

void SFMLRenderer::SetRenderWindow(sf::RenderWindow* window)
{
    m_pRenderWindow = window;

    // Create render textures now that we have an OpenGL context
    if (window && !m_texturesCreated)
    {
        CreateRenderTextures();
    }
}

void SFMLRenderer::Shutdown()
{
    if (m_pdbgsWrapper)
    {
        delete m_pdbgsWrapper;
        m_pdbgsWrapper = nullptr;
    }
}

void SFMLRenderer::SetFullscreen(bool fullscreen)
{
    m_fullscreen = fullscreen;
}

bool SFMLRenderer::IsFullscreen() const
{
    return m_fullscreen;
}

void SFMLRenderer::ChangeDisplayMode(HWND hWnd)
{
    // Get the window through the Window factory
    IWindow* pWindow = Window::Get();
    if (!pWindow)
        return;

    // Cast to SFMLWindow to access SFML-specific methods
    SFMLWindow* pSFMLWindow = static_cast<SFMLWindow*>(pWindow);

    // Apply the fullscreen setting to the window
    // This will recreate the window with the new mode
    pSFMLWindow->SetFullscreen(m_fullscreen);

    // Update our render window pointer (window was recreated)
    m_pRenderWindow = pSFMLWindow->GetRenderWindow();

    // Ensure the OpenGL context is active after window recreation
    if (m_pRenderWindow)
    {
        (void)m_pRenderWindow->setActive(true);
    }

    // Verify render textures are still valid, recreate if needed
    if (m_texturesCreated)
    {
        // Check if textures need to be recreated by verifying they're still valid
        sf::Vector2u backBufferSize = m_backBuffer.getSize();
        if (backBufferSize.x == 0 || backBufferSize.y == 0)
        {
            // Textures became invalid, recreate them
            m_texturesCreated = false;
            CreateRenderTextures();
        }
    }
}

void SFMLRenderer::BeginFrame()
{
    // Ensure OpenGL context is active before any rendering operations
    if (m_pRenderWindow)
    {
        (void)m_pRenderWindow->setActive(true);
    }

    // Reset the view to ensure 1:1 pixel mapping (prevents edge artifacts)
    sf::View pixelView(sf::FloatRect({0.f, 0.f}, {static_cast<float>(m_width), static_cast<float>(m_height)}));
    m_backBuffer.setView(pixelView);

    // Clear to TRANSPARENT black (alpha=0) so that sprite alpha compositing works correctly
    // When we present to the window, we'll draw over a black background
    m_backBuffer.clear(sf::Color::Transparent);

    // Enable scissor test to clip all drawing to exactly 640x480
    // This prevents any content from being drawn outside the intended area
    glEnable(GL_SCISSOR_TEST);
    glScissor(0, 0, m_width, m_height);
}

void SFMLRenderer::EndFrame()
{
    // Disable scissor test before presenting
    glDisable(GL_SCISSOR_TEST);

    m_backBuffer.display();

    if (m_pRenderWindow && m_pRenderWindow->isOpen())
    {
        m_pRenderWindow->clear(sf::Color::Black);

        // Get actual window size in physical pixels
        float windowWidth, windowHeight;

#ifdef _WIN32
        // When DPI aware, use GetClientRect for accurate physical pixel dimensions
        HWND hWnd = static_cast<HWND>(m_pRenderWindow->getNativeHandle());
        if (hWnd)
        {
            RECT clientRect;
            if (GetClientRect(hWnd, &clientRect))
            {
                windowWidth = static_cast<float>(clientRect.right - clientRect.left);
                windowHeight = static_cast<float>(clientRect.bottom - clientRect.top);
            }
            else
            {
                sf::Vector2u size = m_pRenderWindow->getSize();
                windowWidth = static_cast<float>(size.x);
                windowHeight = static_cast<float>(size.y);
            }
        }
        else
#endif
        {
            sf::Vector2u size = m_pRenderWindow->getSize();
            windowWidth = static_cast<float>(size.x);
            windowHeight = static_cast<float>(size.y);
        }

        // Reset the view to match actual window size (1:1 pixel mapping)
        // This is crucial after window resize - SFML's default view changes with setSize()
        sf::View pixelView(sf::FloatRect({0.f, 0.f}, {windowWidth, windowHeight}));
        m_pRenderWindow->setView(pixelView);

        // Use explicit source rectangle to avoid including any texture padding
        // SFML render textures may have internal padding beyond the requested size
        sf::IntRect sourceRect({0, 0}, {m_width, m_height});
        sf::Sprite backBufferSprite(m_backBuffer.getTexture(), sourceRect);

        float scaleX = windowWidth / static_cast<float>(m_width);
        float scaleY = windowHeight / static_cast<float>(m_height);

        if (m_fullscreen)
        {
            // Fullscreen: use uniform scale with letterboxing to maintain aspect ratio
            float scale = (scaleY < scaleX) ? scaleY : scaleX;

            float destWidth = static_cast<float>(m_width) * scale;
            float destHeight = static_cast<float>(m_height) * scale;
            float offsetX = (windowWidth - destWidth) / 2.0f;
            float offsetY = (windowHeight - destHeight) / 2.0f;

            backBufferSprite.setScale({scale, scale});
            backBufferSprite.setPosition({offsetX, offsetY});
        }
        else
        {
            // Windowed: stretch to fill window (all resolutions are 4:3)
            backBufferSprite.setScale({scaleX, scaleY});
            backBufferSprite.setPosition({0.0f, 0.0f});
        }

        m_pRenderWindow->draw(backBufferSprite, sf::RenderStates(sf::BlendAlpha));
        m_pRenderWindow->display();
    }
}

bool SFMLRenderer::EndFrameCheckLostSurface()
{
    EndFrame();
    // SFML doesn't have surface loss like DirectDraw
    return false;
}

void SFMLRenderer::PutPixel(int x, int y, uint8_t r, uint8_t g, uint8_t b)
{
    if (x < m_clipArea.left || x >= m_clipArea.right ||
        y < m_clipArea.top || y >= m_clipArea.bottom)
        return;

    sf::RectangleShape pixel({1.0f, 1.0f});
    pixel.setPosition({static_cast<float>(x), static_cast<float>(y)});
    pixel.setFillColor(sf::Color(r, g, b));
    m_backBuffer.draw(pixel);
}

void SFMLRenderer::DrawShadowBox(int x1, int y1, int x2, int y2, int type)
{
    // Draw a semi-transparent dark box for shadow effect
    sf::RectangleShape box(
        {static_cast<float>(x2 - x1), static_cast<float>(y2 - y1)}
    );
    box.setPosition({static_cast<float>(x1), static_cast<float>(y1)});

    // Different alpha levels based on type
    uint8_t alpha;
    switch (type)
    {
    case 0: alpha = 128; break;  // 50% transparent
    case 1: alpha = 180; break;  // 70% transparent
    case 2: alpha = 64;  break;  // 25% transparent
    default: alpha = 128; break;
    }

    box.setFillColor(sf::Color(0, 0, 0, alpha));
    m_backBuffer.draw(box);
}

void SFMLRenderer::DrawItemShadowBox(int x1, int y1, int x2, int y2, int type)
{
    // Item shadow boxes are similar but may have different styling
    DrawShadowBox(x1, y1, x2, y2, type);
}

void SFMLRenderer::BeginTextBatch()
{
    // No-op for SFML - we don't need a DC
}

void SFMLRenderer::EndTextBatch()
{
    // No-op for SFML
}

void SFMLRenderer::DrawText(int x, int y, const char* text, uint32_t color)
{
    if (!text || !m_fontLoaded || !m_texturesCreated)
        return;

    // Extract RGB from COLORREF (Windows color format: 0x00BBGGRR)
    uint8_t r = static_cast<uint8_t>(color & 0xFF);
    uint8_t g = static_cast<uint8_t>((color >> 8) & 0xFF);
    uint8_t b = static_cast<uint8_t>((color >> 16) & 0xFF);

    sf::Text sfText(m_font, text, 12);  // Default font size
    sfText.setPosition({static_cast<float>(x), static_cast<float>(y)});
    sfText.setFillColor(sf::Color(r, g, b));

    m_backBuffer.draw(sfText);
}

void SFMLRenderer::DrawTextRect(RECT* rect, const char* text, uint32_t color)
{
    if (!rect || !text) {
        printf("DrawTextRect: rect or text is null\n");
        return;
    }
    if (!m_fontLoaded) {
        printf("DrawTextRect: font not loaded!\n");
        return;
    }
    if (!m_texturesCreated) {
        printf("DrawTextRect: textures not created yet!\n");
        return;
    }

    // Extract RGB from COLORREF (Windows color format: 0x00BBGGRR)
    uint8_t r = static_cast<uint8_t>(color & 0xFF);
    uint8_t g = static_cast<uint8_t>((color >> 8) & 0xFF);
    uint8_t b = static_cast<uint8_t>((color >> 16) & 0xFF);

    // Debug: log the draw call (only first few to avoid spam)
    static int drawCount = 0;
    if (drawCount < 10) {
        char debugMsg[512];
        sprintf_s(debugMsg, "DrawTextRect: '%s' at rect(%d,%d,%d,%d) color RGB(%d,%d,%d)\n",
            text, rect->left, rect->top, rect->right, rect->bottom, r, g, b);
        printf(debugMsg);
        drawCount++;
    }

    sf::Text sfText(m_font, text, 12);
    sf::FloatRect bounds = sfText.getLocalBounds();

    // Center horizontally within rect (matching DDraw's DT_CENTER behavior)
    // Round to integer pixel for sharp text rendering
    float rectWidth = static_cast<float>(rect->right - rect->left);
    float centerX = rect->left + (rectWidth / 2.0f);
    float drawX = centerX - (bounds.size.x / 2.0f) - bounds.position.x;
    int pixelX = static_cast<int>(drawX + 0.5f);  // Round to nearest pixel

    sfText.setPosition({static_cast<float>(pixelX), static_cast<float>(rect->top)});
    sfText.setFillColor(sf::Color(r, g, b));

    m_backBuffer.draw(sfText);
}

ITexture* SFMLRenderer::CreateTexture(uint16_t width, uint16_t height)
{
    return new SFMLTexture(width, height);
}

void SFMLRenderer::DestroyTexture(ITexture* texture)
{
    delete texture;
}

uint16_t* SFMLRenderer::LockBackBuffer(int* pitch)
{
    if (m_backBufferLocked)
    {
        if (pitch)
            *pitch = m_width;
        return m_lockedBuffer.data();
    }

    m_backBufferLocked = true;

    // Resize locked buffer if needed
    size_t bufferSize = static_cast<size_t>(m_width) * m_height;
    if (m_lockedBuffer.size() != bufferSize)
    {
        m_lockedBuffer.resize(bufferSize);
    }

    // Copy current back buffer to locked buffer (convert RGBA to RGB565)
    m_lockedImage = m_backBuffer.getTexture().copyToImage();

    for (int y = 0; y < m_height; y++)
    {
        for (int x = 0; x < m_width; x++)
        {
            sf::Color pixel = m_lockedImage.getPixel({static_cast<unsigned int>(x), static_cast<unsigned int>(y)});
            m_lockedBuffer[y * m_width + x] = PixelConversion::MakeRGB565(pixel.r, pixel.g, pixel.b);
        }
    }

    if (pitch)
        *pitch = m_width;

    return m_lockedBuffer.data();
}

void SFMLRenderer::UnlockBackBuffer()
{
    if (!m_backBufferLocked)
        return;

    // Convert RGB565 buffer back to RGBA and update the back buffer
    sf::Image newImage;
    newImage.resize({static_cast<unsigned int>(m_width), static_cast<unsigned int>(m_height)});

    for (int y = 0; y < m_height; y++)
    {
        for (int x = 0; x < m_width; x++)
        {
            uint16_t pixel = m_lockedBuffer[y * m_width + x];
            uint8_t r, g, b;
            PixelConversion::ExtractRGB565(pixel, r, g, b);
            newImage.setPixel({static_cast<unsigned int>(x), static_cast<unsigned int>(y)}, sf::Color(r, g, b, 255));
        }
    }

    // Update the back buffer texture
    sf::Texture tempTex;
    if (tempTex.loadFromImage(newImage))
    {
        m_backBuffer.clear();
        sf::Sprite sprite(tempTex);
        m_backBuffer.draw(sprite);
    }

    m_backBufferLocked = false;
}

void SFMLRenderer::SetClipArea(int x, int y, int w, int h)
{
    SetRect(&m_clipArea, x, y, x + w, y + h);

    // Set SFML viewport/scissor
    // Note: SFML doesn't have direct scissor support, but we store it for manual clipping
}

RECT SFMLRenderer::GetClipArea() const
{
    return m_clipArea;
}

int SFMLRenderer::GetPixelFormat() const
{
    // Return RGB565 for compatibility with existing code
    return PIXELFORMAT_RGB565;
}

bool SFMLRenderer::Screenshot(const char* filename)
{
    if (!filename)
        return false;

    sf::Image screenshot = m_backBuffer.getTexture().copyToImage();
    return screenshot.saveToFile(filename);
}

int SFMLRenderer::GetWidth() const
{
    return m_width;
}

int SFMLRenderer::GetHeight() const
{
    return m_height;
}

int SFMLRenderer::GetWidthMid() const
{
    return m_width / 2;
}

int SFMLRenderer::GetHeightMid() const
{
    return m_height / 2;
}

ITexture* SFMLRenderer::GetBackgroundSurface()
{
    if (!m_pdbgsWrapper)
    {
        // PDBGS is larger than visible area for smooth tile scrolling
        m_pdbgsWrapper = new SFMLTexture(static_cast<uint16_t>(PDBGS_WIDTH), static_cast<uint16_t>(PDBGS_HEIGHT));
    }
    return m_pdbgsWrapper;
}

uint32_t SFMLRenderer::GetColorKey(ITexture* texture, uint16_t colorKey)
{
    // For SFML, just return the color key as-is
    return colorKey;
}

uint32_t SFMLRenderer::GetColorKeyRGB(ITexture* texture, uint8_t r, uint8_t g, uint8_t b)
{
    return PixelConversion::MakeRGB565(r, g, b);
}

void SFMLRenderer::SetColorKey(ITexture* texture, uint16_t colorKey)
{
    if (texture)
    {
        texture->SetColorKey(colorKey);
    }
}

void SFMLRenderer::SetColorKeyRGB(ITexture* texture, uint8_t r, uint8_t g, uint8_t b)
{
    if (texture)
    {
        texture->SetColorKeyRGB(r, g, b);
    }
}

// Transparency table getters (return dummy tables)
const long (*SFMLRenderer::GetTransTableRB100() const)[64] { return s_dummyTransTable; }
const long (*SFMLRenderer::GetTransTableG100() const)[64] { return s_dummyTransTable; }
const long (*SFMLRenderer::GetTransTableRB70() const)[64] { return s_dummyTransTable; }
const long (*SFMLRenderer::GetTransTableG70() const)[64] { return s_dummyTransTable; }
const long (*SFMLRenderer::GetTransTableRB50() const)[64] { return s_dummyTransTable; }
const long (*SFMLRenderer::GetTransTableG50() const)[64] { return s_dummyTransTable; }
const long (*SFMLRenderer::GetTransTableRB25() const)[64] { return s_dummyTransTable; }
const long (*SFMLRenderer::GetTransTableG25() const)[64] { return s_dummyTransTable; }
const long (*SFMLRenderer::GetTransTableRB2() const)[64] { return s_dummyTransTable; }
const long (*SFMLRenderer::GetTransTableG2() const)[64] { return s_dummyTransTable; }
const long (*SFMLRenderer::GetTransTableFadeRB() const)[64] { return s_dummyTransTable; }
const long (*SFMLRenderer::GetTransTableFadeG() const)[64] { return s_dummyTransTable; }

// Add table getters (return dummy tables)
const int (*SFMLRenderer::GetAddTable31() const)[510] { return s_dummyAddTable; }
const int (*SFMLRenderer::GetAddTable63() const)[510] { return s_dummyAddTable; }
const int (*SFMLRenderer::GetAddTransTable31() const)[64] { return s_dummyAddTransTable; }
const int (*SFMLRenderer::GetAddTransTable63() const)[64] { return s_dummyAddTransTable; }

char SFMLRenderer::GetSpriteAlphaDegree() const
{
    return m_spriteAlphaDegree;
}

void SFMLRenderer::SetSpriteAlphaDegree(char degree)
{
    m_spriteAlphaDegree = degree;
}

void SFMLRenderer::ColorTransferRGB(uint32_t rgb, int* outR, int* outG, int* outB)
{
    // Extract RGB from COLORREF format (0x00BBGGRR) - keep full 8-bit values
    // SFML uses RGBA8888 natively so no conversion needed
    if (outR) *outR = static_cast<int>(rgb & 0xFF);
    if (outG) *outG = static_cast<int>((rgb >> 8) & 0xFF);
    if (outB) *outB = static_cast<int>((rgb >> 16) & 0xFF);
}

HDC SFMLRenderer::GetTextDC()
{
    // SFML doesn't use GDI, return nullptr
    // Code that needs HDC for text measurement should use GetTextLength instead
    return nullptr;
}

int SFMLRenderer::GetTextLength(const char* text, int maxWidth)
{
    if (!text || !m_fontLoaded)
        return 0;

    // Measure text using SFML font
    int len = static_cast<int>(strlen(text));
    sf::Text sfText(m_font, "", 12);

    for (int i = len; i > 0; i--)
    {
        std::string substr(text, i);
        sfText.setString(substr);
        sf::FloatRect bounds = sfText.getLocalBounds();

        if (bounds.size.x <= static_cast<float>(maxWidth))
            return i;
    }

    return 0;
}

void SFMLRenderer::BltBackBufferFromPDBGS(RECT* srcRect)
{
    if (!srcRect)
        return;

    // Must call display() before using a RenderTexture as a texture source
    m_pdbgs.display();

    // Clamp source rect to valid PDBGS bounds (672x512) to prevent sampling outside texture
    int srcLeft = srcRect->left;
    int srcTop = srcRect->top;
    int srcRight = srcRect->right;
    int srcBottom = srcRect->bottom;

    // Clamp to PDBGS bounds
    if (srcLeft < 0) srcLeft = 0;
    if (srcTop < 0) srcTop = 0;
    if (srcRight > PDBGS_WIDTH) srcRight = PDBGS_WIDTH;
    if (srcBottom > PDBGS_HEIGHT) srcBottom = PDBGS_HEIGHT;

    // Calculate clamped dimensions
    int width = srcRight - srcLeft;
    int height = srcBottom - srcTop;

    // Skip if nothing to copy
    if (width <= 0 || height <= 0)
        return;

    // Copy from PDBGS to back buffer at (0, 0)
    sf::IntRect intRect(
        {srcLeft, srcTop},
        {width, height}
    );

    sf::Sprite sprite(m_pdbgs.getTexture(), intRect);
    sprite.setPosition({0.0f, 0.0f});
    m_backBuffer.draw(sprite);
}

void* SFMLRenderer::GetBackBufferNative()
{
    return &m_backBuffer;
}

void* SFMLRenderer::GetPDBGSNative()
{
    return &m_pdbgs;
}

void* SFMLRenderer::GetNativeRenderer()
{
    // Return this renderer itself for SFML
    // Legacy code expecting DXC_ddraw* should check renderer type first
    return this;
}

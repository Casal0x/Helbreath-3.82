// SFMLRenderer.cpp: SFML renderer implementation
//
// Part of SFMLEngine static library
//////////////////////////////////////////////////////////////////////

#include "SFMLRenderer.h"
#include "SFMLWindow.h"
#include "RendererFactory.h"
#include "PixelConversion.h"
#include "RenderConstants.h"
#include "ITextRenderer.h"
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/ConvexShape.hpp>
#include <SFML/OpenGL.hpp>
#include <cmath>
#include <cstring>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#include <dxgi.h>
#pragma comment(lib, "dxgi.lib")
#endif

// Static dummy tables
long SFMLRenderer::s_dummyTransTable[64][64] = {};
int SFMLRenderer::s_dummyAddTable[64][510] = {};
int SFMLRenderer::s_dummyAddTransTable[510][64] = {};

SFMLRenderer::SFMLRenderer()
    : m_pRenderWindow(nullptr)
    , m_texturesCreated(false)
    , m_backBufferLocked(false)
    , m_width(640)  // Default, updated in CreateRenderTextures
    , m_height(480) // Default, updated in CreateRenderTextures
    , m_fullscreen(false)
    , m_bFullscreenStretch(false)
    , m_iFpsLimit(0)
    , m_bVSync(false)
    , m_bSkipFrame(false)
    , m_lastPresentTime(std::chrono::steady_clock::now())
    , m_targetFrameDuration(std::chrono::steady_clock::duration::zero())
    , m_fps(0)
    , m_framesThisSecond(0)
    , m_deltaTime(0.0)
    , m_fpsAccumulator(0.0)
    , m_lastPresentedFrameTime(std::chrono::steady_clock::now())
    , m_spriteAlphaDegree(1)
{
    m_clipArea = GameRectangle(0, 0, m_width, m_height);
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

#ifdef _WIN32
// Enumerate GPUs and log their VRAM information
// Returns the index of the GPU with the most dedicated VRAM
static void LogGPUInfo()
{
    IDXGIFactory* pFactory = nullptr;
    HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pFactory);
    if (FAILED(hr) || !pFactory)
    {
        printf("[GPU] Failed to create DXGI factory\n");
        return;
    }

    printf("[GPU] Enumerating available graphics adapters:\n");

    IDXGIAdapter* pAdapter = nullptr;
    UINT adapterIndex = 0;
    UINT bestAdapterIndex = 0;
    SIZE_T maxDedicatedVRAM = 0;

    while (pFactory->EnumAdapters(adapterIndex, &pAdapter) != DXGI_ERROR_NOT_FOUND)
    {
        DXGI_ADAPTER_DESC desc;
        if (SUCCEEDED(pAdapter->GetDesc(&desc)))
        {
            // Convert wide string to narrow for printf
            char adapterName[128];
            WideCharToMultiByte(CP_ACP, 0, desc.Description, -1, adapterName, sizeof(adapterName), nullptr, nullptr);

            SIZE_T dedicatedVRAM_MB = desc.DedicatedVideoMemory / (1024 * 1024);
            SIZE_T sharedVRAM_MB = desc.SharedSystemMemory / (1024 * 1024);

            printf("[GPU]   Adapter %u: %s\n", adapterIndex, adapterName);
            printf("[GPU]     Dedicated VRAM: %zu MB\n", dedicatedVRAM_MB);
            printf("[GPU]     Shared Memory:  %zu MB\n", sharedVRAM_MB);

            if (desc.DedicatedVideoMemory > maxDedicatedVRAM)
            {
                maxDedicatedVRAM = desc.DedicatedVideoMemory;
                bestAdapterIndex = adapterIndex;
            }
        }
        pAdapter->Release();
        adapterIndex++;
    }

    if (adapterIndex > 0 && maxDedicatedVRAM > 0)
    {
        printf("[GPU] Adapter with most VRAM: Adapter %u (%zu MB)\n", bestAdapterIndex, maxDedicatedVRAM / (1024 * 1024));
        printf("[GPU] Note: NvOptimusEnablement and AmdPowerXpressRequestHighPerformance exports are set to prefer discrete GPU\n");
    }

    pFactory->Release();
}
#endif

bool SFMLRenderer::Init(NativeWindowHandle hWnd)
{
    // Log GPU information for debugging/verification
#ifdef _WIN32
    LogGPUInfo();
#endif

    return true;
}

bool SFMLRenderer::CreateRenderTextures()
{
    if (m_texturesCreated)
        return true;

    // Update dimensions from ResolutionConfig (initialized before renderer creation)
    m_width = RENDER_LOGICAL_WIDTH();
    m_height = RENDER_LOGICAL_HEIGHT();
    m_clipArea = GameRectangle(0, 0, m_width, m_height);

    // Create back buffer render texture
    if (!m_backBuffer.resize({static_cast<unsigned int>(m_width), static_cast<unsigned int>(m_height)}))
    {
        printf("[ERROR] SFMLRenderer::CreateRenderTextures - Failed to create back buffer\n");
        return false;
    }

    // Disable texture smoothing for pixel-perfect scaling (matches DDraw sharpness)
    // This uses nearest-neighbor filtering instead of bilinear
    m_backBuffer.setSmooth(false);

    // Disable texture repeating to prevent edge artifacts
    m_backBuffer.setRepeated(false);

    // Set explicit views to ensure 1:1 pixel mapping and prevent edge artifacts
    // The view must match exactly the surface dimensions
    sf::View backBufferView(sf::FloatRect({0.f, 0.f}, {static_cast<float>(m_width), static_cast<float>(m_height)}));
    m_backBuffer.setView(backBufferView);

    // Clear buffer - use transparent for proper alpha compositing
    m_backBuffer.clear(sf::Color::Transparent);
    m_backBuffer.display();

    m_texturesCreated = true;
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
}

void SFMLRenderer::SetFullscreen(bool fullscreen)
{
    m_fullscreen = fullscreen;
}

bool SFMLRenderer::IsFullscreen() const
{
    return m_fullscreen;
}

void SFMLRenderer::SetFullscreenStretch(bool stretch)
{
    m_bFullscreenStretch = stretch;
}

bool SFMLRenderer::IsFullscreenStretch() const
{
    return m_bFullscreenStretch;
}

void SFMLRenderer::SetFramerateLimit(int limit)
{
    m_iFpsLimit = limit;
    if (limit > 0)
        m_targetFrameDuration = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
            std::chrono::microseconds(1000000 / limit));
    else
        m_targetFrameDuration = std::chrono::steady_clock::duration::zero();
}

int SFMLRenderer::GetFramerateLimit() const
{
    return m_iFpsLimit;
}

void SFMLRenderer::SetVSyncMode(bool enabled)
{
    m_bVSync = enabled;
}

void SFMLRenderer::ChangeDisplayMode(NativeWindowHandle hWnd)
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

    // Engine-owned frame limiting: skip this frame if not enough time has elapsed
    // Unlimited (0) always renders; VSync uses monitor refresh rate as the target
    if (m_iFpsLimit > 0)
    {
        auto now = std::chrono::steady_clock::now();
        if ((now - m_lastPresentTime) < m_targetFrameDuration)
        {
            m_bSkipFrame = true;
            // Sleep 1ms to avoid spinning the CPU (accurate with timeBeginPeriod(1))
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            return;
        }
    }
    m_bSkipFrame = false;

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
    if (m_bSkipFrame)
        return;

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

        if (m_fullscreen && !m_bFullscreenStretch)
        {
            // Fullscreen letterbox: uniform scale to maintain aspect ratio
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
            // Windowed or fullscreen stretch: fill entire window
            backBufferSprite.setScale({scaleX, scaleY});
            backBufferSprite.setPosition({0.0f, 0.0f});
        }

        // Enable bilinear filtering for smooth upscaling at any resolution
        // This is applied per-draw, so it doesn't affect the back buffer during rendering
        const_cast<sf::Texture&>(m_backBuffer.getTexture()).setSmooth(true);

        m_pRenderWindow->draw(backBufferSprite, sf::RenderStates(sf::BlendAlpha));

        // Restore nearest-neighbor for internal rendering
        const_cast<sf::Texture&>(m_backBuffer.getTexture()).setSmooth(false);

        m_pRenderWindow->display();

        // Track frame metrics at the actual point of present
        auto now = std::chrono::steady_clock::now();

        if (m_iFpsLimit > 0)
        {
            // Advance deadline by exact target duration to prevent timing drift
            // (snapping to 'now' would accumulate overshoot from sleep granularity)
            m_lastPresentTime += m_targetFrameDuration;

            // If we've fallen behind by more than one frame, snap to avoid catch-up burst
            if ((now - m_lastPresentTime) >= m_targetFrameDuration)
                m_lastPresentTime = now;
        }

        auto elapsed = std::chrono::duration<double>(now - m_lastPresentedFrameTime);
        m_deltaTime = elapsed.count();
        m_lastPresentedFrameTime = now;

        m_framesThisSecond++;
        m_fpsAccumulator += m_deltaTime;
        if (m_fpsAccumulator >= 1.0)
        {
            m_fps = m_framesThisSecond;
            m_framesThisSecond = 0;
            m_fpsAccumulator -= 1.0;
        }
    }
}

bool SFMLRenderer::EndFrameCheckLostSurface()
{
    if (m_bSkipFrame)
        return false;
    EndFrame();
    // SFML doesn't have surface loss like DirectDraw
    return false;
}

bool SFMLRenderer::WasFramePresented() const
{
    return !m_bSkipFrame;
}

uint32_t SFMLRenderer::GetFPS() const
{
    return m_fps;
}

double SFMLRenderer::GetDeltaTime() const
{
    return m_deltaTime;
}

double SFMLRenderer::GetDeltaTimeMS() const
{
    return m_deltaTime * 1000.0;
}

void SFMLRenderer::DrawPixel(int x, int y, const Color& color)
{
    if (x < m_clipArea.Left() || x >= m_clipArea.Right() ||
        y < m_clipArea.Top() || y >= m_clipArea.Bottom())
        return;

    sf::RectangleShape pixel({1.0f, 1.0f});
    pixel.setPosition({static_cast<float>(x), static_cast<float>(y)});
    pixel.setFillColor(sf::Color(color.r, color.g, color.b, color.a));
    m_backBuffer.draw(pixel);
}

void SFMLRenderer::DrawLine(int x0, int y0, int x1, int y1, const Color& color, BlendMode blend)
{
    if ((x0 == x1) && (y0 == y1)) return;

    sf::Color lineColor(color.r, color.g, color.b, color.a);

    sf::VertexArray line(sf::PrimitiveType::Lines, 2);
    line[0].position = sf::Vector2f(static_cast<float>(x0), static_cast<float>(y0));
    line[0].color = lineColor;
    line[1].position = sf::Vector2f(static_cast<float>(x1), static_cast<float>(y1));
    line[1].color = lineColor;

    m_backBuffer.draw(line, (blend == BlendMode::Additive) ? sf::BlendAdd : sf::BlendAlpha);
}

void SFMLRenderer::DrawRectFilled(int x, int y, int w, int h, const Color& color)
{
    if (color.a == 0 || w <= 0 || h <= 0) return;

    sf::RectangleShape rect({static_cast<float>(w), static_cast<float>(h)});
    rect.setPosition({static_cast<float>(x), static_cast<float>(y)});
    rect.setFillColor(sf::Color(color.r, color.g, color.b, color.a));
    m_backBuffer.draw(rect);
}

void SFMLRenderer::DrawRectOutline(int x, int y, int w, int h, const Color& color, int thickness)
{
    if (color.a == 0 || w <= 0 || h <= 0 || thickness <= 0) return;

    sf::Color c(color.r, color.g, color.b, color.a);
    float fx = static_cast<float>(x);
    float fy = static_cast<float>(y);
    float fw = static_cast<float>(w);
    float fh = static_cast<float>(h);
    float ft = static_cast<float>(thickness);

    // Top edge
    sf::RectangleShape top({fw, ft});
    top.setPosition({fx, fy});
    top.setFillColor(c);
    m_backBuffer.draw(top);

    // Bottom edge
    sf::RectangleShape bottom({fw, ft});
    bottom.setPosition({fx, fy + fh - ft});
    bottom.setFillColor(c);
    m_backBuffer.draw(bottom);

    // Left edge (between top and bottom)
    sf::RectangleShape left({ft, fh - 2.0f * ft});
    left.setPosition({fx, fy + ft});
    left.setFillColor(c);
    m_backBuffer.draw(left);

    // Right edge (between top and bottom)
    sf::RectangleShape right({ft, fh - 2.0f * ft});
    right.setPosition({fx + fw - ft, fy + ft});
    right.setFillColor(c);
    m_backBuffer.draw(right);
}

// Helper: generate rounded rect corner points into an array, deduplicating coincident points.
// Returns the number of unique points written.
static int GenerateRoundedRectPoints(sf::Vector2f* out, int maxVerts,
                                     float fx, float fy, float fw, float fh, float fr)
{
    constexpr int kSegments = 8;
    constexpr float kPi = 3.14159265f;
    constexpr float kHalfPi = kPi * 0.5f;
    constexpr float kEpsilon = 0.01f;

    sf::Vector2f pts[kSegments * 4];
    int idx = 0;
    float cornerAngles[4] = { kPi, kHalfPi, 0.0f, -kHalfPi };
    float centerX[4] = { fx + fr, fx + fw - fr, fx + fw - fr, fx + fr };
    float centerY[4] = { fy + fr, fy + fr, fy + fh - fr, fy + fh - fr };

    for (int c = 0; c < 4; ++c)
    {
        for (int i = 0; i < kSegments; ++i)
        {
            float angle = cornerAngles[c] - kHalfPi * i / (kSegments - 1);
            pts[idx++] = { centerX[c] + fr * std::cos(angle),
                           centerY[c] - fr * std::sin(angle) };
        }
    }

    // Deduplicate adjacent coincident points
    int count = 0;
    for (int i = 0; i < idx && count < maxVerts; ++i)
    {
        if (count == 0 ||
            std::abs(pts[i].x - out[count - 1].x) > kEpsilon ||
            std::abs(pts[i].y - out[count - 1].y) > kEpsilon)
        {
            out[count++] = pts[i];
        }
    }
    // Check wrap-around (last vs first)
    if (count > 1 &&
        std::abs(out[count - 1].x - out[0].x) <= kEpsilon &&
        std::abs(out[count - 1].y - out[0].y) <= kEpsilon)
    {
        --count;
    }
    return count;
}

void SFMLRenderer::DrawRoundedRectFilled(int x, int y, int w, int h, int radius, const Color& color)
{
    if (color.a == 0 || w <= 0 || h <= 0) return;

    float fx = static_cast<float>(x);
    float fy = static_cast<float>(y);
    float fw = static_cast<float>(w);
    float fh = static_cast<float>(h);

    float maxRadius = (std::min)(fw, fh) * 0.5f;
    float fr = (std::min)(static_cast<float>(radius), maxRadius);

    if (fr <= 0.0f)
    {
        DrawRectFilled(x, y, w, h, color);
        return;
    }

    sf::Vector2f pts[32];
    int count = GenerateRoundedRectPoints(pts, 32, fx, fy, fw, fh, fr);
    if (count < 3) { DrawRectFilled(x, y, w, h, color); return; }

    // Use TriangleFan — avoids ConvexShape's edge-normal computation entirely
    sf::VertexArray fan(sf::PrimitiveType::TriangleFan, count + 2);
    sf::Color c(color.r, color.g, color.b, color.a);

    // Center point
    fan[0].position = { fx + fw * 0.5f, fy + fh * 0.5f };
    fan[0].color = c;
    for (int i = 0; i < count; ++i)
    {
        fan[i + 1].position = pts[i];
        fan[i + 1].color = c;
    }
    // Close the fan back to the first perimeter point
    fan[count + 1].position = pts[0];
    fan[count + 1].color = c;

    m_backBuffer.draw(fan);
}

void SFMLRenderer::DrawRoundedRectOutline(int x, int y, int w, int h, int radius,
                                          const Color& color, int thickness)
{
    if (color.a == 0 || w <= 0 || h <= 0 || thickness <= 0) return;

    float fx = static_cast<float>(x);
    float fy = static_cast<float>(y);
    float fw = static_cast<float>(w);
    float fh = static_cast<float>(h);

    float maxRadius = (std::min)(fw, fh) * 0.5f;
    float fr = (std::min)(static_cast<float>(radius), maxRadius);

    if (fr <= 0.0f)
    {
        DrawRectOutline(x, y, w, h, color, thickness);
        return;
    }

    float ft = static_cast<float>(thickness);

    // Inner rect dimensions (inset by thickness on all sides)
    float ifx = fx + ft;
    float ify = fy + ft;
    float ifw = fw - 2.0f * ft;
    float ifh = fh - 2.0f * ft;
    float ifr = (std::max)(0.0f, fr - ft);

    // If inner rect is degenerate, fall back to filled
    if (ifw <= 0.0f || ifh <= 0.0f)
    {
        DrawRoundedRectFilled(x, y, w, h, radius, color);
        return;
    }

    // Generate both perimeters with identical point counts using same angles
    constexpr int kSegments = 8;
    constexpr float kPi = 3.14159265f;
    constexpr float kHalfPi = kPi * 0.5f;
    constexpr int totalPts = kSegments * 4;

    sf::Color sfColor(color.r, color.g, color.b, color.a);

    float cornerAngles[4] = { kPi, kHalfPi, 0.0f, -kHalfPi };

    // Outer corner centers
    float oCX[4] = { fx + fr, fx + fw - fr, fx + fw - fr, fx + fr };
    float oCY[4] = { fy + fr, fy + fr, fy + fh - fr, fy + fh - fr };

    // Inner corner centers
    float iCX[4] = { ifx + ifr, ifx + ifw - ifr, ifx + ifw - ifr, ifx + ifr };
    float iCY[4] = { ify + ifr, ify + ifr, ify + ifh - ifr, ify + ifh - ifr };

    // TriangleStrip: for each perimeter point, emit outer then inner
    sf::VertexArray strip(sf::PrimitiveType::TriangleStrip, (totalPts + 1) * 2);

    for (int corner = 0; corner < 4; ++corner)
    {
        for (int i = 0; i < kSegments; ++i)
        {
            int pi = corner * kSegments + i;
            float angle = cornerAngles[corner] - kHalfPi * i / (kSegments - 1);
            float ca = std::cos(angle);
            float sa = std::sin(angle);

            strip[pi * 2].position = { oCX[corner] + fr * ca, oCY[corner] - fr * sa };
            strip[pi * 2].color = sfColor;

            strip[pi * 2 + 1].position = { iCX[corner] + ifr * ca, iCY[corner] - ifr * sa };
            strip[pi * 2 + 1].color = sfColor;
        }
    }

    // Close the strip back to the first point pair
    strip[totalPts * 2].position = strip[0].position;
    strip[totalPts * 2].color = sfColor;
    strip[totalPts * 2 + 1].position = strip[1].position;
    strip[totalPts * 2 + 1].color = sfColor;

    m_backBuffer.draw(strip);
}

void SFMLRenderer::BeginTextBatch()
{
    // Delegate to TextLib for consistency
    TextLib::ITextRenderer* pTextRenderer = TextLib::GetTextRenderer();
    if (pTextRenderer)
        pTextRenderer->BeginBatch();
}

void SFMLRenderer::EndTextBatch()
{
    // Delegate to TextLib for consistency
    TextLib::ITextRenderer* pTextRenderer = TextLib::GetTextRenderer();
    if (pTextRenderer)
        pTextRenderer->EndBatch();
}

void SFMLRenderer::DrawText(int x, int y, const char* text, const Color& color)
{
    if (!text || !m_texturesCreated)
        return;

    // Delegate to TextLib - single point of font handling
    TextLib::ITextRenderer* pTextRenderer = TextLib::GetTextRenderer();
    if (pTextRenderer)
        pTextRenderer->DrawText(x, y, text, color);
}

void SFMLRenderer::DrawTextRect(const GameRectangle& rect, const char* text, const Color& color)
{
    if (!text || !m_texturesCreated)
        return;

    // Delegate to TextLib - single point of font handling
    TextLib::ITextRenderer* pTextRenderer = TextLib::GetTextRenderer();
    if (pTextRenderer)
    {
        pTextRenderer->DrawTextAligned(rect.x, rect.y, rect.width, rect.height, text, color,
                                        TextLib::Align::TopCenter);
    }
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
    m_clipArea = GameRectangle(x, y, w, h);

    // Set SFML viewport/scissor
    // Note: SFML doesn't have direct scissor support, but we store it for manual clipping
}

GameRectangle SFMLRenderer::GetClipArea() const
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

void SFMLRenderer::ResizeBackBuffer(int width, int height)
{
    // The back buffer always stays at 640x480 (logical resolution)
    // Window scaling is handled in EndFrame() when presenting to the window
    // This is intentionally a no-op for SFML
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

int SFMLRenderer::GetTextLength(const char* text, int maxWidth)
{
    if (!text)
        return 0;

    // Delegate to TextLib - single point of font handling
    TextLib::ITextRenderer* pTextRenderer = TextLib::GetTextRenderer();
    if (pTextRenderer)
        return pTextRenderer->GetFittingCharCount(text, maxWidth);

    return 0;
}

int SFMLRenderer::GetTextWidth(const char* text)
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

void* SFMLRenderer::GetBackBufferNative()
{
    return &m_backBuffer;
}

void* SFMLRenderer::GetNativeRenderer()
{
    // Return this renderer itself for SFML
    // Legacy code expecting DXC_ddraw* should check renderer type first
    return this;
}

#include "GameClientApp.h"
#include "Core/Config/ProjectManifest.h"
#include "Core/Logging/Log.h"
#include "Core/Math/Matrix.h"
#include "Core/Math/Vector.h"
#include "Game/Interaction/RigState.h"
#include "Game/Interaction/Inventory.h"
#include "Game/Interaction/ResourceItem.h"
#include "Game/Gameplay/SolarSystem/DevSolarSystem.h"
#include "Game/Voxel/VoxelType.h"
#include <chrono>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <string>

#ifdef _WIN32
#if !defined(_WIN32_WINNT) || _WIN32_WINNT < 0x0A00
#  undef  _WIN32_WINNT
#  define _WIN32_WINNT 0x0A00
#endif
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX    // prevent min/max macros from conflicting with std::min/std::max
#define NODRAWTEXT  // prevent DrawText being macro-expanded to DrawTextA/DrawTextW
#include <windows.h>
#include <windowsx.h>
#endif

namespace NF::Game {

// ---------------------------------------------------------------------------
// Colour palette (0xRRGGBBAA)
// ---------------------------------------------------------------------------
static constexpr uint32_t kBgColor      = 0x0D0D0FFF; // near-black sky
static constexpr uint32_t kHudBg        = 0x1A1A1ECC; // semi-transparent HUD strip
static constexpr uint32_t kHPColor      = 0x22CC44FF; // green health
static constexpr uint32_t kENColor      = 0x2277FFFF; // blue energy
static constexpr uint32_t kBarBg        = 0x2A2A2AFF; // bar background
static constexpr uint32_t kTextColor    = 0xCCCCCCFF;
static constexpr uint32_t kTitleColor   = 0xFFFFFFFF;
static constexpr uint32_t kSepColor     = 0x444444FF;
static constexpr uint32_t kItemColor    = 0xA87A3BFF; // amber inventory
static constexpr uint32_t kCrosshairCol = 0xCCCCCC99; // translucent crosshair
static constexpr uint32_t kMenuBg       = 0x0A0A0ADD; // dark semi-transparent overlay
static constexpr uint32_t kBtnBg        = 0x3C3C3CFF;
static constexpr uint32_t kBtnHover     = 0x505053FF;
static constexpr uint32_t kFlashColor   = 0xFFDD44FF; // mine feedback flash

#ifdef _WIN32

// ---------------------------------------------------------------------------
// Win32 window procedure
// ---------------------------------------------------------------------------

static LRESULT CALLBACK GameClientWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }

    GameClientApp* app = reinterpret_cast<GameClientApp*>(
        GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (app)
        app->DispatchOsEvent(msg,
            static_cast<uintptr_t>(wParam),
            static_cast<intptr_t>(lParam));

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// ---------------------------------------------------------------------------
// DispatchOsEvent
// ---------------------------------------------------------------------------

void GameClientApp::DispatchOsEvent(unsigned msg, uintptr_t wParam, intptr_t lParam) noexcept
{
    const WPARAM wp = static_cast<WPARAM>(wParam);
    const LPARAM lp = static_cast<LPARAM>(lParam);

    switch (msg)
    {
    case WM_LBUTTONDOWN:
        m_MouseX = static_cast<float>(GET_X_LPARAM(lp));
        m_MouseY = static_cast<float>(GET_Y_LPARAM(lp));
        m_LeftDown        = true;
        m_LeftJustPressed = true;
        SetCapture(static_cast<HWND>(m_Hwnd));
        break;

    case WM_LBUTTONUP:
        m_LeftDown = false;
        ReleaseCapture();
        break;

    case WM_RBUTTONDOWN:
        m_RightDown = true;
        SetCapture(static_cast<HWND>(m_Hwnd));
        break;

    case WM_RBUTTONUP:
        m_RightDown = false;
        ReleaseCapture();
        break;

    case WM_MOUSEMOVE:
    {
        const float newX = static_cast<float>(GET_X_LPARAM(lp));
        const float newY = static_cast<float>(GET_Y_LPARAM(lp));
        if (m_MouseTracking) {
            m_MouseDeltaX += newX - m_PrevMouseX;
            m_MouseDeltaY += newY - m_PrevMouseY;
        }
        m_PrevMouseX = newX;
        m_PrevMouseY = newY;
        m_MouseX = newX;
        m_MouseY = newY;
        m_MouseTracking = true;
        break;
    }

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    {
        const unsigned vk = static_cast<unsigned>(wParam) & 0xFFu;
        if (!m_Keys[vk])
            m_KeysJustPressed[vk] = true;
        m_Keys[vk] = true;
        break;
    }

    case WM_KEYUP:
    case WM_SYSKEYUP:
    {
        const unsigned vk = static_cast<unsigned>(wParam) & 0xFFu;
        m_Keys[vk] = false;
        break;
    }

    case WM_DPICHANGED:
    {
        m_DpiScale = static_cast<float>(HIWORD(wp)) / 96.f;
        m_UIRenderer.SetDpiScale(m_DpiScale);
        const RECT* r = reinterpret_cast<const RECT*>(lp);
        SetWindowPos(static_cast<HWND>(m_Hwnd), nullptr,
                     r->left, r->top,
                     r->right - r->left, r->bottom - r->top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        break;
    }

    default:
        break;
    }
}

#endif // _WIN32

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------

bool GameClientApp::Init()
{
    NF::Logger::Log(NF::LogLevel::Info, "GameClient", "[1/4] Creating platform window");

#ifdef _WIN32
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    HINSTANCE hInst = GetModuleHandleW(nullptr);

    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = GameClientWndProc;
    wc.hInstance     = hInst;
    wc.hCursor       = LoadCursorW(nullptr, reinterpret_cast<LPCWSTR>(IDC_ARROW));
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = L"NovaForgeGame";

    if (!RegisterClassExW(&wc)) {
        NF::Logger::Log(NF::LogLevel::Error, "GameClient", "Failed to register window class");
        return false;
    }

    HWND hwnd = CreateWindowExW(
        0, L"NovaForgeGame", L"NovaForge",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720,
        nullptr, nullptr, hInst, nullptr);

    if (!hwnd) {
        NF::Logger::Log(NF::LogLevel::Error, "GameClient", "Failed to create window");
        UnregisterClassW(L"NovaForgeGame", hInst);
        return false;
    }

    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    {
        UINT dpi = GetDpiForWindow(hwnd);
        m_DpiScale = static_cast<float>(dpi) / 96.f;
    }

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);
    m_Hwnd = hwnd;
#endif

    NF::Logger::Log(NF::LogLevel::Info, "GameClient", "[2/4] Initialising render device");
    m_RenderDevice = std::make_unique<NF::RenderDevice>();

#ifdef NF_HAS_OPENGL
    const NF::GraphicsAPI api = NF::GraphicsAPI::OpenGL;
#else
    const NF::GraphicsAPI api = NF::GraphicsAPI::Null;
#endif

    if (!m_RenderDevice->Init(api, m_Hwnd)) {
        NF::Logger::Log(NF::LogLevel::Error, "GameClient", "RenderDevice init failed");
        return false;
    }
    m_RenderDevice->Resize(m_ClientWidth, m_ClientHeight);

    NF::Logger::Log(NF::LogLevel::Info, "GameClient", "[3/4] Initialising UI renderer");
    m_UIRenderer.Init();
    m_UIRenderer.SetViewportSize(static_cast<float>(m_ClientWidth),
                                  static_cast<float>(m_ClientHeight));
    m_UIRenderer.SetDpiScale(m_DpiScale);

    NF::Logger::Log(NF::LogLevel::Info, "GameClient", "[4/4] Initialising game orchestrator");
    if (!m_Orchestrator.Init(m_RenderDevice.get())) {
        NF::Logger::Log(NF::LogLevel::Error, "GameClient", "Orchestrator init failed");
        return false;
    }

    // Read dev flags from project manifest to gate dev-only features.
    {
        NF::ProjectManifest manifest;
        manifest.LoadFromFile("Config/novaforge.project.json");
        m_DevSolarMapEnabled = manifest.DevSolarMapEnabled;
        if (m_DevSolarMapEnabled)
            NF::Logger::Log(NF::LogLevel::Info, "GameClient",
                            "Dev: solar map overlay enabled (M key)");
    }

    // Phase 4: initialise the forward renderer and chunk mesh cache.
    m_ForwardRenderer.Init(m_RenderDevice.get());
    m_MeshCache.Init(&m_ForwardRenderer);
    m_MeshCache.RebuildDirty(m_Orchestrator.GetGameWorld().GetChunkMap());

    m_Running = true;
    NF::Logger::Log(NF::LogLevel::Info, "GameClient", "GameClientApp::Init complete");
    return true;
}

// ---------------------------------------------------------------------------
// Run
// ---------------------------------------------------------------------------

void GameClientApp::Run()
{
    NF::Logger::Log(NF::LogLevel::Info, "GameClient", "Entering game loop");

    using Clock = std::chrono::steady_clock;
    auto lastTime = Clock::now();

#ifdef _WIN32
    MSG msg{};
    while (m_Running)
    {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) {
                m_Running = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        // Detect window resize
        {
            RECT rc{};
            if (GetClientRect(static_cast<HWND>(m_Hwnd), &rc))
            {
                int newW = rc.right  - rc.left;
                int newH = rc.bottom - rc.top;
                if (newW != m_ClientWidth || newH != m_ClientHeight)
                {
                    m_ClientWidth  = newW;
                    m_ClientHeight = newH;
                    m_RenderDevice->Resize(m_ClientWidth, m_ClientHeight);
                }
            }
        }

        auto now = Clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;
        TickFrame(dt);
        FlushFrameInput();
    }
#else
    // Headless loop for CI / non-Windows platforms — run 120 frames
    // (approximately 2 seconds at 60 FPS) to exercise all game systems
    // then exit so automated tests can validate correctness.
    // Skip main menu in headless mode so we actually exercise gameplay.
    m_State = GameAppState::Playing;
    constexpr int kHeadlessFrames = 120;
    for (int frame = 0; frame < kHeadlessFrames && m_Running; ++frame)
    {
        auto now = Clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;
        TickFrame(dt);
    }
    m_Running = false;
#endif
}

// ---------------------------------------------------------------------------
// Shutdown
// ---------------------------------------------------------------------------

void GameClientApp::Shutdown()
{
    NF::Logger::Log(NF::LogLevel::Info, "GameClient", "GameClientApp::Shutdown");
    m_MeshCache.Shutdown();
    m_ForwardRenderer.Shutdown();
    m_Orchestrator.Shutdown();
    m_UIRenderer.Shutdown();
    m_RenderDevice->Shutdown();

#ifdef _WIN32
    if (m_Hwnd && IsWindow(static_cast<HWND>(m_Hwnd)))
        DestroyWindow(static_cast<HWND>(m_Hwnd));
    m_Hwnd = nullptr;
    UnregisterClassW(L"NovaForgeGame", GetModuleHandleW(nullptr));
#endif
}

// ---------------------------------------------------------------------------
// TickFrame
// ---------------------------------------------------------------------------

void GameClientApp::TickFrame(float dt)
{
    m_RenderDevice->BeginFrame();
    m_RenderDevice->Clear(0.10f, 0.10f, 0.12f, 1.f);

    // --- Handle Escape key for state transitions ---
    if (m_KeysJustPressed[0x1B]) { // VK_ESCAPE = 0x1B
        switch (m_State) {
        case GameAppState::MainMenu:
            // Escape on main menu = exit
            m_Running = false;
            break;
        case GameAppState::Playing:
            m_State = GameAppState::Paused;
            break;
        case GameAppState::Paused:
            m_State = GameAppState::Playing;
            break;
        }
    }

    // --- Update UI renderer viewport ---
    m_UIRenderer.SetViewportSize(static_cast<float>(m_ClientWidth),
                                  static_cast<float>(m_ClientHeight));
    m_UIRenderer.BeginFrame();

    switch (m_State) {
    case GameAppState::MainMenu:
        DrawMainMenu();
        break;

    case GameAppState::Playing:
    {
        // --- Phase 5: read input and feed to PlayerMovement ---
        {
            auto& pm = m_Orchestrator.GetPlayerMovement();
            float forward = 0.f, right = 0.f;
            if (m_Keys[0x57]) forward += 1.f; // W
            if (m_Keys[0x53]) forward -= 1.f; // S
            if (m_Keys[0x44]) right   += 1.f; // D
            if (m_Keys[0x41]) right   -= 1.f; // A

            const bool jump   = m_KeysJustPressed[0x20]; // Space
            const bool sprint = m_Keys[0x10];             // Shift

            pm.SetMoveInput(forward, right, jump, sprint);

            if (m_RightDown && (m_MouseDeltaX != 0.f || m_MouseDeltaY != 0.f))
                pm.ApplyMouseLook(m_MouseDeltaX, m_MouseDeltaY);
        }

        m_Orchestrator.Tick(dt);

        // Handle mining on left click
        HandleMining();

        // M key toggles solar map overlay (dev-only: gated by config flag).
        if (m_DevSolarMapEnabled && m_KeysJustPressed[0x4D]) // VK_M
            m_ShowSolarMap = !m_ShowSolarMap;

        // Update mine flash timer
        if (m_MineFlashTimer > 0.f)
            m_MineFlashTimer -= dt;

        // ---- Phase 4: render voxel chunk meshes ----
        m_MeshCache.RebuildDirty(m_Orchestrator.GetGameWorld().GetChunkMap());
        {
            NF::Matrix4x4 view = GetViewMatrix();
            NF::Matrix4x4 proj = GetProjectionMatrix();
            m_ForwardRenderer.BeginScene(view, proj);
            m_MeshCache.SetCameraPosition(m_Orchestrator.GetPlayerMovement().GetEyePosition());
            m_MeshCache.Render();
            m_ForwardRenderer.EndScene();
        }

        // ---- 2-D overlays ----
        DrawCrosshair();
        DrawHUD();
        if (m_ShowSolarMap)
            DrawSolarMap();
        break;
    }

    case GameAppState::Paused:
    {
        // Render world behind the pause overlay (frozen)
        m_MeshCache.RebuildDirty(m_Orchestrator.GetGameWorld().GetChunkMap());
        {
            NF::Matrix4x4 view = GetViewMatrix();
            NF::Matrix4x4 proj = GetProjectionMatrix();
            m_ForwardRenderer.BeginScene(view, proj);
            m_MeshCache.SetCameraPosition(m_Orchestrator.GetPlayerMovement().GetEyePosition());
            m_MeshCache.Render();
            m_ForwardRenderer.EndScene();
        }

        DrawCrosshair();
        DrawHUD();
        DrawPauseMenu();
        break;
    }
    }

    m_UIRenderer.EndFrame();
    m_RenderDevice->EndFrame();
}

// ---------------------------------------------------------------------------
// DrawHUD
// ---------------------------------------------------------------------------

void GameClientApp::DrawHUD()
{
    const float dpi   = m_UIRenderer.GetDpiScale();
    const float lineH = 18.f * dpi;
    const float barH  = 14.f * dpi;
    const float padX  = 8.f  * dpi;
    const float hudW  = 260.f * dpi;
    const float hudX  = padX;
    const float hudY  = static_cast<float>(m_ClientHeight) - (lineH * 7.f + 12.f * dpi);
    const float scale = 2.f;

    // HUD background panel
    m_UIRenderer.DrawRect({hudX - 4.f * dpi, hudY - 4.f * dpi,
                            hudW + 8.f * dpi, lineH * 7.f + 16.f * dpi}, kHudBg);

    const NF::Game::RigState&   rig = m_Orchestrator.GetInteractionLoop().GetRig();
    const NF::Game::Inventory&  inv = m_Orchestrator.GetInteractionLoop().GetInventory();

    float cy = hudY;

    // Title
    m_UIRenderer.DrawText("NovaForge", hudX, cy, kTitleColor, scale);
    cy += lineH;
    m_UIRenderer.DrawRect({hudX, cy, hudW, 1.f}, kSepColor);
    cy += 4.f * dpi;

    // Rig name
    m_UIRenderer.DrawText(("Rig: " + rig.GetName()).c_str(), hudX, cy, kTextColor, scale);
    cy += lineH;

    // Health bar
    {
        const float hp = rig.GetHealth();
        const float frac = std::max(0.f, std::min(1.f, hp / NF::Game::RigState::kMaxHealth));
        m_UIRenderer.DrawRect({hudX, cy, hudW, barH}, kBarBg);
        if (frac > 0.f)
            m_UIRenderer.DrawRect({hudX, cy, hudW * frac, barH}, kHPColor);
        const std::string hpLabel = "HP " + std::to_string(static_cast<int>(hp))
                                  + " / " + std::to_string(static_cast<int>(NF::Game::RigState::kMaxHealth));
        m_UIRenderer.DrawText(hpLabel.c_str(), hudX + 4.f * dpi, cy + 2.f * dpi, kTextColor, scale);
        cy += barH + 3.f * dpi;
    }

    // Energy bar
    {
        const float en = rig.GetEnergy();
        const float frac = std::max(0.f, std::min(1.f, en / NF::Game::RigState::kMaxEnergy));
        m_UIRenderer.DrawRect({hudX, cy, hudW, barH}, kBarBg);
        if (frac > 0.f)
            m_UIRenderer.DrawRect({hudX, cy, hudW * frac, barH}, kENColor);
        const std::string enLabel = "EN " + std::to_string(static_cast<int>(en))
                                  + " / " + std::to_string(static_cast<int>(NF::Game::RigState::kMaxEnergy));
        m_UIRenderer.DrawText(enLabel.c_str(), hudX + 4.f * dpi, cy + 2.f * dpi, kTextColor, scale);
        cy += barH + 3.f * dpi;
    }

    // Inventory — show total item count across all slots
    {
        uint32_t total = 0;
        for (int i = 0; i < NF::Game::Inventory::kMaxSlots; ++i) {
            const auto& slot = inv.GetSlot(i);
            if (!slot.IsEmpty()) total += slot.count;
        }
        const std::string invLabel = "Inventory: " + std::to_string(total) + " items";
        m_UIRenderer.DrawText(invLabel.c_str(), hudX, cy, kItemColor, scale);
        cy += lineH;
    }

    // Position info (Phase 5)
    {
        const auto& pm = m_Orchestrator.GetPlayerMovement();
        const auto& pos = pm.GetPosition();
        const std::string posLabel = "Pos: " + std::to_string(static_cast<int>(pos.X))
                                   + ", " + std::to_string(static_cast<int>(pos.Y))
                                   + ", " + std::to_string(static_cast<int>(pos.Z))
                                   + (pm.IsGrounded() ? " [GND]" : " [AIR]");
        m_UIRenderer.DrawText(posLabel.c_str(), hudX, cy, kTextColor, scale);
        cy += lineH;
    }

    // Mining feedback flash
    if (m_MineFlashTimer > 0.f) {
        const float alpha = std::min(1.f, m_MineFlashTimer / 0.4f);
        const uint32_t flashAlpha = static_cast<uint32_t>(alpha * 255.f);
        const uint32_t flashCol = (kFlashColor & 0xFFFFFF00u) | flashAlpha;
        m_UIRenderer.DrawText("MINED!", hudX, cy, flashCol, scale);
    }
}

// ---------------------------------------------------------------------------
// DrawCrosshair — simple + shaped crosshair at screen centre
// ---------------------------------------------------------------------------

void GameClientApp::DrawCrosshair()
{
    const float cx = static_cast<float>(m_ClientWidth)  * 0.5f;
    const float cy = static_cast<float>(m_ClientHeight) * 0.5f;
    const float dpi = m_UIRenderer.GetDpiScale();
    const float len = 10.f * dpi;
    const float th  = 2.f * dpi;

    // Horizontal line
    m_UIRenderer.DrawRect({cx - len, cy - th * 0.5f, len * 2.f, th}, kCrosshairCol);
    // Vertical line
    m_UIRenderer.DrawRect({cx - th * 0.5f, cy - len, th, len * 2.f}, kCrosshairCol);
}

// ---------------------------------------------------------------------------
// DrawSolarMap — full-screen 2D orbital map overlay (toggle with M key)
// ---------------------------------------------------------------------------

void GameClientApp::DrawSolarMap()
{
    using NF::Game::Gameplay::CelestialBodyType;

    const float w   = static_cast<float>(m_ClientWidth);
    const float h   = static_cast<float>(m_ClientHeight);
    const float dpi = m_UIRenderer.GetDpiScale();

    // Semi-transparent full-screen background.
    m_UIRenderer.DrawRect({0.f, 0.f, w, h}, 0x0A0A0ADD);

    const auto& sys = m_Orchestrator.GetSolarSystem();

    // Title + close hint
    m_UIRenderer.DrawText("SOLAR MAP", w * 0.5f - 40.f * dpi, 18.f * dpi, kTitleColor, 2.f);
    m_UIRenderer.DrawText("[M] Close", w - 90.f * dpi, 20.f * dpi, kSepColor, 1.5f);
    const std::string seedStr = "Seed: " + std::to_string(sys.GetSeed())
                              + "  Bodies: " + std::to_string(sys.BodyCount());
    m_UIRenderer.DrawText(seedStr, 20.f * dpi, 20.f * dpi, kSepColor, 1.5f);

    if (sys.BodyCount() == 0) {
        m_UIRenderer.DrawText("No solar system data loaded.", w * 0.5f - 80.f * dpi,
                              h * 0.5f, kTextColor, 2.f);
        return;
    }

    const float cx = w * 0.5f;
    const float cy = h * 0.5f + 10.f * dpi; // slightly below centre to clear title

    // Scale: fit the widest planet orbit into 80% of the shorter screen dimension.
    float maxOrbit = 1.f;
    for (const auto& body : sys.GetBodies()) {
        if (body.type == CelestialBodyType::Planet)
            maxOrbit = std::max(maxOrbit, body.orbitRadius);
    }
    const float scale = std::min(w, h) * 0.38f / maxOrbit;

    // Orbit rings (approximated with dots).
    for (const auto& body : sys.GetBodies()) {
        if (body.type != CelestialBodyType::Planet) continue;
        const float r = body.orbitRadius * scale;
        constexpr int kSeg = 72;
        for (int s = 0; s < kSeg; ++s) {
            const float a  = static_cast<float>(s) * 6.2832f / static_cast<float>(kSeg);
            const float px = cx + std::cos(a) * r;
            const float py = cy + std::sin(a) * r;
            m_UIRenderer.DrawRect({px - dpi, py - dpi, 2.f * dpi, 2.f * dpi}, 0x333355FF);
        }
    }

    // Bodies
    for (const auto& body : sys.GetBodies()) {
        float bx, by;

        if (body.type == CelestialBodyType::Star) {
            bx = cx;
            by = cy;
        } else if (body.type == CelestialBodyType::Moon) {
            const auto* parent = sys.FindBody(body.parentId);
            if (!parent) continue;
            const float pr = parent->orbitRadius * scale;
            const float pa = parent->orbitAngle;
            const float parentX = cx + std::cos(pa) * pr;
            const float parentY = cy + std::sin(pa) * pr;
            bx = parentX + body.FlatX() * scale * 0.3f;
            by = parentY + body.FlatY() * scale * 0.3f;
        } else {
            bx = cx + body.FlatX() * scale;
            by = cy + body.FlatY() * scale;
        }

        float drawR = 4.f * dpi;
        if (body.type == CelestialBodyType::Star)   drawR = 12.f * dpi;
        else if (body.type == CelestialBodyType::Moon) drawR = 2.f * dpi;

        m_UIRenderer.DrawRect({bx - drawR, by - drawR, drawR * 2.f, drawR * 2.f}, body.color);

        // Body label (skip moon labels to reduce clutter)
        if (body.type != CelestialBodyType::Moon) {
            m_UIRenderer.DrawText(body.name.c_str(),
                                  bx + drawR + 3.f * dpi,
                                  by - 6.f * dpi,
                                  kTextColor, 1.5f);
        }

        // Deposit count badge for planets
        if (body.type == CelestialBodyType::Planet && !body.deposits.empty()) {
            const std::string depStr = std::to_string(body.deposits.size()) + "d";
            m_UIRenderer.DrawText(depStr.c_str(),
                                  bx + drawR + 3.f * dpi,
                                  by + 4.f * dpi,
                                  kItemColor, 1.f);
        }
    }

    // Controls hint at bottom
    m_UIRenderer.DrawText("Orbital map | M = toggle | Travel not yet available",
                          20.f * dpi, h - 26.f * dpi, kSepColor, 1.5f);
}

// ---------------------------------------------------------------------------
// DrawMainMenu — title screen with Play and Exit buttons
// ---------------------------------------------------------------------------

void GameClientApp::DrawMainMenu()
{
    const float w = static_cast<float>(m_ClientWidth);
    const float h = static_cast<float>(m_ClientHeight);
    const float dpi = m_UIRenderer.GetDpiScale();
    const float scale = 2.f;

    // Full-screen dark background
    m_UIRenderer.DrawRect({0.f, 0.f, w, h}, kMenuBg);

    // Title
    const float titleX = w * 0.5f - 80.f * dpi;
    const float titleY = h * 0.3f;
    m_UIRenderer.DrawText("NovaForge", titleX, titleY, kTitleColor, 3.f);
    m_UIRenderer.DrawText("Development Build", titleX + 10.f * dpi, titleY + 30.f * dpi, kSepColor, scale);

    // Buttons
    const float btnW = 160.f * dpi;
    const float btnH = 32.f  * dpi;
    const float btnX = w * 0.5f - btnW * 0.5f;
    const float gap  = 12.f * dpi;

    // Play button
    {
        const float by = h * 0.5f;
        const bool hov = m_MouseX >= btnX && m_MouseX < btnX + btnW &&
                         m_MouseY >= by   && m_MouseY < by + btnH;
        m_UIRenderer.DrawRect({btnX, by, btnW, btnH}, hov ? kBtnHover : kBtnBg);
        m_UIRenderer.DrawOutlineRect({btnX, by, btnW, btnH}, kSepColor);
        m_UIRenderer.DrawText("> Play", btnX + 48.f * dpi, by + 8.f * dpi, kTitleColor, scale);
        if (hov && m_LeftJustPressed) {
            m_State = GameAppState::Playing;
            NF::Logger::Log(NF::LogLevel::Info, "GameClient", "Starting game...");
        }
    }

    // Exit button
    {
        const float by = h * 0.5f + btnH + gap;
        const bool hov = m_MouseX >= btnX && m_MouseX < btnX + btnW &&
                         m_MouseY >= by   && m_MouseY < by + btnH;
        m_UIRenderer.DrawRect({btnX, by, btnW, btnH}, hov ? kBtnHover : kBtnBg);
        m_UIRenderer.DrawOutlineRect({btnX, by, btnW, btnH}, kSepColor);
        m_UIRenderer.DrawText("Exit", btnX + 54.f * dpi, by + 8.f * dpi, kTextColor, scale);
        if (hov && m_LeftJustPressed) {
            m_Running = false;
        }
    }

    // Controls hint
    m_UIRenderer.DrawText("WASD: Move | Space: Jump | Shift: Sprint | RMB: Look | LMB: Mine | Esc: Pause",
                          20.f * dpi, h - 30.f * dpi, kSepColor, 1.5f);
}

// ---------------------------------------------------------------------------
// DrawPauseMenu — semi-transparent overlay with Resume and Exit buttons
// ---------------------------------------------------------------------------

void GameClientApp::DrawPauseMenu()
{
    const float w = static_cast<float>(m_ClientWidth);
    const float h = static_cast<float>(m_ClientHeight);
    const float dpi = m_UIRenderer.GetDpiScale();
    const float scale = 2.f;

    // Semi-transparent overlay
    m_UIRenderer.DrawRect({0.f, 0.f, w, h}, 0x00000099);

    // "PAUSED" title
    const float titleX = w * 0.5f - 50.f * dpi;
    const float titleY = h * 0.35f;
    m_UIRenderer.DrawText("PAUSED", titleX, titleY, kTitleColor, 3.f);

    const float btnW = 160.f * dpi;
    const float btnH = 32.f  * dpi;
    const float btnX = w * 0.5f - btnW * 0.5f;
    const float gap  = 12.f * dpi;

    // Resume button
    {
        const float by = h * 0.48f;
        const bool hov = m_MouseX >= btnX && m_MouseX < btnX + btnW &&
                         m_MouseY >= by   && m_MouseY < by + btnH;
        m_UIRenderer.DrawRect({btnX, by, btnW, btnH}, hov ? kBtnHover : kBtnBg);
        m_UIRenderer.DrawOutlineRect({btnX, by, btnW, btnH}, kSepColor);
        m_UIRenderer.DrawText("Resume", btnX + 44.f * dpi, by + 8.f * dpi, kTitleColor, scale);
        if (hov && m_LeftJustPressed) {
            m_State = GameAppState::Playing;
        }
    }

    // Main Menu button
    {
        const float by = h * 0.48f + btnH + gap;
        const bool hov = m_MouseX >= btnX && m_MouseX < btnX + btnW &&
                         m_MouseY >= by   && m_MouseY < by + btnH;
        m_UIRenderer.DrawRect({btnX, by, btnW, btnH}, hov ? kBtnHover : kBtnBg);
        m_UIRenderer.DrawOutlineRect({btnX, by, btnW, btnH}, kSepColor);
        m_UIRenderer.DrawText("Main Menu", btnX + 34.f * dpi, by + 8.f * dpi, kTextColor, scale);
        if (hov && m_LeftJustPressed) {
            m_State = GameAppState::MainMenu;
        }
    }

    // Exit button
    {
        const float by = h * 0.48f + (btnH + gap) * 2.f;
        const bool hov = m_MouseX >= btnX && m_MouseX < btnX + btnW &&
                         m_MouseY >= by   && m_MouseY < by + btnH;
        m_UIRenderer.DrawRect({btnX, by, btnW, btnH}, hov ? kBtnHover : kBtnBg);
        m_UIRenderer.DrawOutlineRect({btnX, by, btnW, btnH}, kSepColor);
        m_UIRenderer.DrawText("Exit Game", btnX + 38.f * dpi, by + 8.f * dpi, kTextColor, scale);
        if (hov && m_LeftJustPressed) {
            m_Running = false;
        }
    }
}

// ---------------------------------------------------------------------------
// HandleMining — left-click raycast mining with HUD feedback
// ---------------------------------------------------------------------------

void GameClientApp::HandleMining()
{
    if (!m_LeftJustPressed) return;

    auto& pm = m_Orchestrator.GetPlayerMovement();
    const NF::Vector3 eye = pm.GetEyePosition();
    const NF::Vector3 dir = pm.GetViewDirection();

    auto hit = m_Orchestrator.GetGameWorld().GetChunkMap().RaycastVoxel(
        eye.X, eye.Y, eye.Z,
        dir.X, dir.Y, dir.Z,
        8.f); // mining reach = 8 voxels

    if (hit.hit) {
        auto& loop = m_Orchestrator.GetInteractionLoop();
        loop.Mine(hit.x, hit.y, hit.z);
        m_MineFlashTimer = 0.4f; // show "mined!" flash for 0.4s
    }
}

// ---------------------------------------------------------------------------
// FlushFrameInput
// ---------------------------------------------------------------------------

void GameClientApp::FlushFrameInput() noexcept
{
    for (int i = 0; i < 256; ++i)
        m_KeysJustPressed[i] = false;
    m_LeftJustPressed = false;
    m_MouseDeltaX = 0.f;
    m_MouseDeltaY = 0.f;
}

// ---------------------------------------------------------------------------
// Camera
// ---------------------------------------------------------------------------

NF::Matrix4x4 GameClientApp::GetViewMatrix() const noexcept
{
    const auto& pm = m_Orchestrator.GetPlayerMovement();
    const NF::Vector3 eye = pm.GetEyePosition();
    const NF::Vector3 viewDir = pm.GetViewDirection();

    // Target is eye + viewDir.
    const NF::Vector3 target{eye.X + viewDir.X, eye.Y + viewDir.Y, eye.Z + viewDir.Z};

    NF::Vector3 forward = viewDir;
    NF::Vector3 right   = NF::Vector3{0.f, 1.f, 0.f}.Cross(forward).Normalized();
    NF::Vector3 up      = forward.Cross(right);

    NF::Matrix4x4 v = NF::Matrix4x4::Identity();
    v.M[0][0] = right.X;    v.M[1][0] = right.Y;    v.M[2][0] = right.Z;
    v.M[0][1] = up.X;       v.M[1][1] = up.Y;       v.M[2][1] = up.Z;
    v.M[0][2] = -forward.X; v.M[1][2] = -forward.Y; v.M[2][2] = -forward.Z;
    v.M[3][0] = -right.Dot(eye);
    v.M[3][1] = -up.Dot(eye);
    v.M[3][2] =  forward.Dot(eye);
    return v;
}

NF::Matrix4x4 GameClientApp::GetProjectionMatrix() const noexcept
{
    const float aspect = (m_ClientHeight > 0)
        ? static_cast<float>(m_ClientWidth) / static_cast<float>(m_ClientHeight) : 1.f;
    const float fovY  = 0.7854f; // ~45 degrees
    const float nearZ = 0.1f, farZ = 1000.f;
    const float f     = 1.f / std::tan(fovY * 0.5f);

    NF::Matrix4x4 p{};
    p.M[0][0] = f / aspect;
    p.M[1][1] = f;
    p.M[2][2] = (farZ + nearZ) / (nearZ - farZ);
    p.M[2][3] = -1.f;
    p.M[3][2] = (2.f * farZ * nearZ) / (nearZ - farZ);
    return p;
}

} // namespace NF::Game

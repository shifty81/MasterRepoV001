#include "Editor/Application/EditorApp.h"
#include "Core/Config/ProjectManifest.h"
#include "Core/Logging/Log.h"
#include "Game/Voxel/VoxelType.h"
#include "Game/Voxel/Chunk.h"
#include "Editor/Commands/VoxelEditCommands.h"
#include <chrono>

#ifdef _WIN32
// Require Windows 10 minimum for per-monitor V2 DPI awareness and GetDpiForWindow.
// Raise _WIN32_WINNT to 0x0A00 if a lower version was already defined.
#if !defined(_WIN32_WINNT) || _WIN32_WINNT < 0x0A00
#  undef  _WIN32_WINNT
#  define _WIN32_WINNT 0x0A00
#endif
#define WIN32_LEAN_AND_MEAN
#define NODRAWTEXT  // prevent DrawText being macro-expanded to DrawTextA/DrawTextW
#include <windows.h>
#include <windowsx.h>  // GET_X_LPARAM, GET_Y_LPARAM
#endif

namespace NF::Editor {

#ifdef _WIN32
// ---------------------------------------------------------------------------
// Win32 window procedure
// ---------------------------------------------------------------------------

static LRESULT CALLBACK EditorWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_DESTROY)
    {
        PostQuitMessage(0);
        return 0;
    }

    // Route all other messages through the EditorApp stored in user data.
    // GWLP_USERDATA is set after CreateWindowExW returns, so it may be null
    // for messages fired during window creation — that is safe.
    EditorApp* app = reinterpret_cast<EditorApp*>(
        GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    if (app)
    {
        // WM_DPICHANGED: reposition to the OS-suggested rect and return 0 so
        // DefWindowProc does not override our placement.
        if (msg == WM_DPICHANGED)
        {
            app->DispatchOsEvent(msg,
                static_cast<uintptr_t>(wParam),
                static_cast<intptr_t>(lParam));
            return 0;
        }
        app->DispatchOsEvent(msg,
            static_cast<uintptr_t>(wParam),
            static_cast<intptr_t>(lParam));
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// ---------------------------------------------------------------------------
// DispatchOsEvent — updates m_Input and m_DpiScale from raw Win32 messages
// ---------------------------------------------------------------------------

void EditorApp::DispatchOsEvent(unsigned msg, uintptr_t wParam, intptr_t lParam) noexcept
{
    const WPARAM wp = static_cast<WPARAM>(wParam);
    const LPARAM lp = static_cast<LPARAM>(lParam);

    switch (msg)
    {
    case WM_MOUSEMOVE:
    {
        const float newX = static_cast<float>(GET_X_LPARAM(lp));
        const float newY = static_cast<float>(GET_Y_LPARAM(lp));
        m_Input.mouseDeltaX = newX - m_Input.mouseX;
        m_Input.mouseDeltaY = newY - m_Input.mouseY;
        m_Input.mouseX = newX;
        m_Input.mouseY = newY;
        break;
    }

    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
        m_Input.mouseX          = static_cast<float>(GET_X_LPARAM(lp));
        m_Input.mouseY          = static_cast<float>(GET_Y_LPARAM(lp));
        m_Input.leftDown        = true;
        m_Input.leftJustPressed = true;
        SetCapture(static_cast<HWND>(m_Hwnd));
        break;

    case WM_LBUTTONUP:
        m_Input.mouseX           = static_cast<float>(GET_X_LPARAM(lp));
        m_Input.mouseY           = static_cast<float>(GET_Y_LPARAM(lp));
        m_Input.leftDown         = false;
        m_Input.leftJustReleased = true;
        ReleaseCapture();
        break;

    case WM_RBUTTONDOWN:
        m_Input.mouseX           = static_cast<float>(GET_X_LPARAM(lp));
        m_Input.mouseY           = static_cast<float>(GET_Y_LPARAM(lp));
        m_Input.rightDown        = true;
        m_Input.rightJustPressed = true;
        SetCapture(static_cast<HWND>(m_Hwnd));
        break;

    case WM_RBUTTONUP:
        m_Input.mouseX  = static_cast<float>(GET_X_LPARAM(lp));
        m_Input.mouseY  = static_cast<float>(GET_Y_LPARAM(lp));
        m_Input.rightDown = false;
        ReleaseCapture();
        break;

    case WM_MBUTTONDOWN:
        m_Input.mouseX            = static_cast<float>(GET_X_LPARAM(lp));
        m_Input.mouseY            = static_cast<float>(GET_Y_LPARAM(lp));
        m_Input.middleDown        = true;
        m_Input.middleJustPressed = true;
        SetCapture(static_cast<HWND>(m_Hwnd));
        break;

    case WM_MBUTTONUP:
        m_Input.mouseX      = static_cast<float>(GET_X_LPARAM(lp));
        m_Input.mouseY      = static_cast<float>(GET_Y_LPARAM(lp));
        m_Input.middleDown  = false;
        ReleaseCapture();
        break;

    case WM_MOUSEWHEEL:
        m_Input.wheelDelta +=
            static_cast<float>(GET_WHEEL_DELTA_WPARAM(wp)) / static_cast<float>(WHEEL_DELTA);
        break;

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    {
        const unsigned vk = static_cast<unsigned>(wParam) & 0xFFu;
        if (!m_Input.keysDown[vk])
            m_Input.keysJustPressed[vk] = true;
        m_Input.keysDown[vk] = true;
        break;
    }

    case WM_KEYUP:
    case WM_SYSKEYUP:
    {
        const unsigned vk = static_cast<unsigned>(wParam) & 0xFFu;
        m_Input.keysDown[vk] = false;
        break;
    }

    case WM_DPICHANGED:
    {
        m_DpiScale = static_cast<float>(HIWORD(wp)) / 96.f;
        m_UIRenderer.SetDpiScale(m_DpiScale);
        // Reposition window to the DPI-suggested rect.
        const RECT* suggested = reinterpret_cast<const RECT*>(lp);
        SetWindowPos(static_cast<HWND>(m_Hwnd), nullptr,
                     suggested->left, suggested->top,
                     suggested->right  - suggested->left,
                     suggested->bottom - suggested->top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        break;
    }

    default:
        break;
    }
}
#endif

bool EditorApp::Init() {
    Logger::Log(LogLevel::Info, "Editor", "[1/6] EditorApp — loading project manifest");

    // Step 1 — load project manifest
    ProjectManifest manifest;
    {
        const std::string manifestPath = "Config/novaforge.project.json";
        if (manifest.LoadFromFile(manifestPath))
        {
            manifest.LogSummary();
        }
        else
        {
            Logger::Log(LogLevel::Warning, "Editor",
                        "Project manifest not found; using defaults");
        }
    }

    Logger::Log(LogLevel::Info, "Editor", "[2/6] EditorApp — creating platform window");

#ifdef _WIN32
    // Enable per-monitor V2 DPI awareness before any window is created so the
    // OS sends WM_DPICHANGED and reports correct DPI via GetDpiForWindow.
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    HINSTANCE hInstance = GetModuleHandleW(nullptr);

    WNDCLASSEXW wc{};
    wc.cbSize        = sizeof(wc);
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = EditorWndProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursorW(nullptr, reinterpret_cast<LPCWSTR>(IDC_ARROW));
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wc.lpszClassName = L"NovaForgeEditor";

    if (!RegisterClassExW(&wc))
    {
        Logger::Log(LogLevel::Error, "Editor", "Failed to register window class");
        return false;
    }

    HWND hwnd = CreateWindowExW(
        0,
        L"NovaForgeEditor",
        L"NovaForge Editor",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1280, 720,
        nullptr, nullptr, hInstance, nullptr);

    if (!hwnd)
    {
        Logger::Log(LogLevel::Error, "Editor", "Failed to create window");
        UnregisterClassW(L"NovaForgeEditor", hInstance);
        return false;
    }

    // Store 'this' in the window's user data so EditorWndProc can route
    // input events back to us without a global variable.
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    // Query the monitor DPI now that the window exists.
    {
        UINT dpi = GetDpiForWindow(hwnd);
        m_DpiScale = static_cast<float>(dpi) / 96.f;
    }

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);
    m_Hwnd = hwnd;
#endif

    Logger::Log(LogLevel::Info, "Editor", "[3/6] EditorApp — initialising render device");
    m_RenderDevice = std::make_unique<RenderDevice>();

    // Use OpenGL when available, passing the native window handle so the
    // device can create a rendering context.  Falls back to Null if OpenGL
    // was not compiled in.
#ifdef NF_HAS_OPENGL
    const GraphicsAPI selectedAPI = GraphicsAPI::OpenGL;
#else
    const GraphicsAPI selectedAPI = GraphicsAPI::Null;
#endif
    if (!m_RenderDevice->Init(selectedAPI, m_Hwnd)) {
        Logger::Log(LogLevel::Error, "Editor", "RenderDevice init failed");
        return false;
    }

    Logger::Log(LogLevel::Info, "Editor", "[4/6] EditorApp — loading dev world");
    // Load the dev world via GameWorld (Phase 1: wired editor load path)
    {
        const std::string contentRoot =
            manifest.IsValid() ? manifest.ContentRoot : "Content";
        m_GameWorld.Initialize(contentRoot);
    }
    m_Level.Load(manifest.IsValid() ? manifest.DefaultWorld : "DevWorld");

    Logger::Log(LogLevel::Info, "Editor", "[5/6] EditorApp — setting up viewport");
    // Viewport
    m_Viewport.Init(m_RenderDevice.get());
    m_Viewport.Resize(m_ClientWidth, m_ClientHeight);

    // Point the orbit camera at the actual terrain generated around the spawn.
    // Spawn defaults to (128, 32, 128); terrain fills y=0..12 in that area.
    {
        const auto& sp = m_GameWorld.GetSpawnPoint();
        // Terrain surface is roughly at y=10 (see GameWorld::Initialize terrain loop).
        // Place the orbit pivot at the surface so the camera looks down at it.
        m_Viewport.SetCameraTarget({sp.Position.X, 10.f, sp.Position.Z});
        // Zoom out to show all 9 chunks (3×32 = 96 unit span).
        m_Viewport.SetCameraZoom(80.f);
        // Tilt ~35° down for a clear overview.
        m_Viewport.SetCameraPitch(0.6f);
        Logger::Log(LogLevel::Info, "Editor",
            "Viewport camera centred on spawn (" + std::to_string(sp.Position.X)
            + ", " + std::to_string(sp.Position.Z) + ")");
    }

    // Set initial OpenGL viewport to match the window client area.
    m_RenderDevice->Resize(m_ClientWidth, m_ClientHeight);

    // Initialise the custom UI renderer
    m_UIRenderer.Init();
    m_UIRenderer.SetViewportSize(static_cast<float>(m_ClientWidth),
                                  static_cast<float>(m_ClientHeight));
    m_UIRenderer.SetDpiScale(m_DpiScale);

    // Initialise the forward renderer and chunk mesh cache (Phase 4).
    m_ForwardRenderer.Init(m_RenderDevice.get());
    m_MeshCache.Init(&m_ForwardRenderer);
    m_MeshCache.RebuildDirty(m_GameWorld.GetChunkMap());

    Logger::Log(LogLevel::Info, "Editor", "[6/6] EditorApp — wiring panels and layout");
    // Wire UIRenderer to all subsystems
    m_DockingSystem.SetUIRenderer(&m_UIRenderer);
    m_SceneOutliner.SetUIRenderer(&m_UIRenderer);
    m_Inspector.SetUIRenderer(&m_UIRenderer);
    m_ContentBrowser.SetUIRenderer(&m_UIRenderer);
    m_ConsolePanel.SetUIRenderer(&m_UIRenderer);
    m_Viewport.SetUIRenderer(&m_UIRenderer);
    m_VoxelInspector.SetUIRenderer(&m_UIRenderer);
    m_HUDPanel.SetUIRenderer(&m_UIRenderer);

    // Wire WorldDebugPanel
    m_WorldDebugPanel.SetUIRenderer(&m_UIRenderer);
    m_WorldDebugPanel.SetOverlay(&m_GameWorld.GetDebugOverlay());

    // Wire input state to interactive panels
    m_SceneOutliner.SetInputState(&m_Input);
    m_ConsolePanel.SetInputState(&m_Input);
    m_ContentBrowser.SetInputState(&m_Input);
    m_Viewport.SetInputState(&m_Input);
    m_VoxelInspector.SetInputState(&m_Input);
    m_HUDPanel.SetInputState(&m_Input);
    m_Inspector.SetInputState(&m_Input);

    // Wire voxel layer
    m_VoxelInspector.SetChunkMap(&m_GameWorld.GetChunkMap());

    // Wire Phase 3 interaction loop
    m_InteractionLoop.Init(&m_GameWorld.GetVoxelEditApi());
    m_HUDPanel.SetInteractionLoop(&m_InteractionLoop);

    // Wire toolbar
    m_Toolbar.SetUIRenderer(&m_UIRenderer);
    m_Toolbar.SetInteractionLoop(&m_InteractionLoop);
    m_Toolbar.SetInputState(&m_Input);
    m_Toolbar.SetToolContext(&m_ToolContext);

    // Wire PropertyInspectorSystem to Inspector so it renders the property grid
    m_Inspector.SetPropertyInspectorSystem(&m_PropertyInspectorSystem);
    m_Inspector.SetOnPropertyEdited([this]() {
        ApplyPropertyEditsToWorld();
    });

    // ---- Command registry and hotkeys ----
    RegisterEditorCommands();
    m_HotkeyMap.BuildDefaultBindings();

    // Wire command registry to toolbar so menu items and Undo/Redo can execute
    m_Toolbar.SetCommandRegistry(&m_CommandRegistry);

    // Panels
    m_SceneOutliner.SetWorld(&m_Level.GetWorld());
    m_SceneOutliner.SetOnSelectionChanged([this](EntityId id) {
        nf::SelectionHandle handle;
        handle.kind  = nf::SelectionKind::WorldObject;
        handle.id    = static_cast<uint64_t>(id);
        handle.label = "Entity " + std::to_string(id);
        m_Selection.Select(handle);
        m_Inspector.SetSelectedEntity(id, &m_Level.GetWorld());
        BuildPropertySetForSelection(handle);
        UpdateViewportHighlight();
    });

    // Use manifest content root if available, else default
    m_ContentBrowser.SetRootPath(
        manifest.IsValid() ? manifest.ContentRoot : "Content");

    // Register panels with the docking system — callbacks now forward bounds
    m_DockingSystem.RegisterPanel("SceneOutliner",
        [this](float x, float y, float w, float h) {
            m_SceneOutliner.Draw(x, y, w, h);
        });
    m_DockingSystem.RegisterPanel("Viewport",
        [this](float x, float y, float w, float h) {
            m_Viewport.Draw(x, y, w, h);
        });
    m_DockingSystem.RegisterPanel("Inspector",
        [this](float x, float y, float w, float h) {
            m_Inspector.Draw(x, y, w, h);
        });
    m_DockingSystem.RegisterPanel("ContentBrowser",
        [this](float x, float y, float w, float h) {
            m_ContentBrowser.Draw(x, y, w, h);
        });
    m_DockingSystem.RegisterPanel("Console",
        [this](float x, float y, float w, float h) {
            m_ConsolePanel.Draw(x, y, w, h);
        });
    m_DockingSystem.RegisterPanel("VoxelInspector",
        [this](float x, float y, float w, float h) {
            m_VoxelInspector.Draw(x, y, w, h);
        });
    m_DockingSystem.RegisterPanel("HUD",
        [this](float x, float y, float w, float h) {
            m_HUDPanel.Draw(x, y, w, h);
        });
    m_DockingSystem.RegisterPanel("WorldDebug",
        [this](float x, float y, float w, float h) {
            m_WorldDebugPanel.Draw(x, y, w, h);
        });

    // Default layout:
    //   SceneOutliner (20%) | Viewport+Console (56%) | Inspector+ContentBrowser+VoxelInspector (24%)
    m_DockingSystem.SetRootSplit("SceneOutliner", "Viewport", 0.20f);
    m_DockingSystem.SplitPanel("Viewport",  "Inspector",      SplitAxis::Horizontal, 0.70f);
    m_DockingSystem.SplitPanel("Inspector", "ContentBrowser",  SplitAxis::Vertical,  0.40f);
    m_DockingSystem.SplitPanel("ContentBrowser", "VoxelInspector", SplitAxis::Vertical, 0.50f);
    m_DockingSystem.SplitPanel("VoxelInspector", "HUD",            SplitAxis::Vertical, 0.50f);
    m_DockingSystem.SplitPanel("HUD",           "WorldDebug",     SplitAxis::Vertical, 0.50f);
    m_DockingSystem.SplitPanel("Viewport",  "Console",         SplitAxis::Vertical,  0.75f);

    // The Viewport panel sits directly over the OpenGL 3-D render target.
    // Skip drawing an opaque 2-D background for it so the scene is visible.
    m_DockingSystem.SetPanelTransparent("Viewport");

    m_Running = true;
    Logger::Log(LogLevel::Info, "Editor", "EditorApp::Init complete -- editor-first boot to DevWorld");
    return true;
}

// ---------------------------------------------------------------------------
// RegisterEditorCommands
// ---------------------------------------------------------------------------

void EditorApp::RegisterEditorCommands()
{
    using namespace nf;

    // File.NewWorld
    m_CommandRegistry.Register(nf::EditorCommand{
        "File.NewWorld",
        "New World",
        nullptr,
        [this](EditorCommandContext&) {
            Logger::Log(LogLevel::Info, "Editor",
                        "File.NewWorld: resetting to a blank dev world");
            m_GameWorld.Shutdown();
            m_GameWorld.Initialize("Content");
            m_Level.Unload();
            m_Level.Load("DevWorld");
            m_SceneOutliner.SetWorld(&m_Level.GetWorld());
            m_MeshCache.RebuildDirty(m_GameWorld.GetChunkMap());
            m_Selection.Clear();
            m_ToolContext.worldDirty = false;
            Logger::Log(LogLevel::Info, "Editor", "New world ready");
        }
    });

    // File.Exit
    m_CommandRegistry.Register(nf::EditorCommand{
        "File.Exit",
        "Exit",
        nullptr,
        [this](EditorCommandContext&) {
            Logger::Log(LogLevel::Info, "Editor", "File.Exit command triggered");
            m_Running = false;
        }
    });

    // World.SaveDevWorld
    m_CommandRegistry.Register(nf::EditorCommand{
        "World.SaveDevWorld",
        "Save World",
        nullptr,
        [this](EditorCommandContext&) {
            Logger::Log(LogLevel::Info, "Editor", "Saving dev world...");
            const bool entityOk = m_GameWorld.SaveWorld("Content/Worlds/DevWorld.nfsv");
            const bool chunkOk  = m_GameWorld.SaveChunks("Content/Worlds/DevWorld.nfck");
            if (entityOk && chunkOk) {
                m_ToolContext.worldDirty = false;
                Logger::Log(LogLevel::Info, "Editor", "World saved (entities + chunks)");
            } else {
                Logger::Log(LogLevel::Warning, "Editor",
                            "World save incomplete — entities: " + std::string(entityOk ? "ok" : "FAILED")
                            + ", chunks: " + std::string(chunkOk ? "ok" : "FAILED"));
            }
        }
    });

    // World.ReloadDevWorld
    m_CommandRegistry.Register(nf::EditorCommand{
        "World.ReloadDevWorld",
        "Reload World",
        nullptr,
        [this](EditorCommandContext&) {
            Logger::Log(LogLevel::Info, "Editor", "Reloading dev world...");
            m_GameWorld.Shutdown();
            m_GameWorld.Initialize("Content");

            // If a saved chunk file exists, load it over the generated terrain.
            if (m_GameWorld.LoadChunks("Content/Worlds/DevWorld.nfck")) {
                Logger::Log(LogLevel::Info, "Editor",
                            "Loaded saved chunk data (" + std::to_string(m_GameWorld.GetLoadedChunkCount())
                            + " chunks)");
            }

            m_Level.Unload();
            m_Level.Load("DevWorld");
            m_SceneOutliner.SetWorld(&m_Level.GetWorld());
            m_MeshCache.RebuildDirty(m_GameWorld.GetChunkMap());
            m_Selection.Clear();
            m_ToolContext.worldDirty = false;
            m_CommandHistory = CommandHistory{}; // Clear undo/redo stack
            Logger::Log(LogLevel::Info, "Editor", "World reloaded");
        }
    });

    // Tools.SelectMode
    m_CommandRegistry.Register(nf::EditorCommand{
        "Tools.SelectMode", "Select Mode", nullptr,
        [this](EditorCommandContext&) {
            m_ToolContext.activeMode = EditorToolMode::Select;
            Logger::Log(LogLevel::Info, "Editor", "Tool mode: Select");
        }
    });

    // Tools.VoxelInspectMode
    m_CommandRegistry.Register(nf::EditorCommand{
        "Tools.VoxelInspectMode", "Voxel Inspect", nullptr,
        [this](EditorCommandContext&) {
            m_ToolContext.activeMode = EditorToolMode::VoxelInspect;
            Logger::Log(LogLevel::Info, "Editor", "Tool mode: VoxelInspect");
        }
    });

    // Tools.VoxelAddMode
    m_CommandRegistry.Register(nf::EditorCommand{
        "Tools.VoxelAddMode", "Voxel Add", nullptr,
        [this](EditorCommandContext&) {
            m_ToolContext.activeMode = EditorToolMode::VoxelAdd;
            Logger::Log(LogLevel::Info, "Editor", "Tool mode: VoxelAdd");
        }
    });

    // Tools.VoxelRemoveMode
    m_CommandRegistry.Register(nf::EditorCommand{
        "Tools.VoxelRemoveMode", "Voxel Remove", nullptr,
        [this](EditorCommandContext&) {
            m_ToolContext.activeMode = EditorToolMode::VoxelRemove;
            Logger::Log(LogLevel::Info, "Editor", "Tool mode: VoxelRemove");
        }
    });

    // Edit.Undo
    m_CommandRegistry.Register(nf::EditorCommand{
        "Edit.Undo", "Undo",
        [this](const EditorCommandContext&) { return m_CommandHistory.CanUndo(); },
        [this](EditorCommandContext&) {
            m_CommandHistory.Undo();
            Logger::Log(LogLevel::Info, "Editor", "Undo");
        }
    });

    // Edit.Redo
    m_CommandRegistry.Register(nf::EditorCommand{
        "Edit.Redo", "Redo",
        [this](const EditorCommandContext&) { return m_CommandHistory.CanRedo(); },
        [this](EditorCommandContext&) {
            m_CommandHistory.Redo();
            Logger::Log(LogLevel::Info, "Editor", "Redo");
        }
    });

    Logger::Log(LogLevel::Info, "Editor",
                "Registered " + std::to_string(m_CommandRegistry.GetRegisteredCommandIds().size())
                + " editor commands");
}

// ---------------------------------------------------------------------------
// ProcessHotkeys
// ---------------------------------------------------------------------------

void EditorApp::ProcessHotkeys()
{
#ifdef _WIN32
    // Check modifier states.
    const bool ctrl  = m_Input.keysDown[VK_CONTROL];
    const bool shift = m_Input.keysDown[VK_SHIFT];
    const bool alt   = m_Input.keysDown[VK_MENU];

    // Scan all just-pressed keys.
    for (int vk = 0; vk < 256; ++vk) {
        if (!m_Input.keysJustPressed[vk]) continue;

        // Skip modifier keys themselves.
        if (vk == VK_CONTROL || vk == VK_SHIFT || vk == VK_MENU ||
            vk == VK_LCONTROL || vk == VK_RCONTROL ||
            vk == VK_LSHIFT || vk == VK_RSHIFT ||
            vk == VK_LMENU || vk == VK_RMENU)
            continue;

        // Convert VK to a string key name for hotkey lookup.
        std::string keyName;
        if (vk >= 'A' && vk <= 'Z') {
            keyName = static_cast<char>(vk);
        } else if (vk >= '0' && vk <= '9') {
            keyName = static_cast<char>(vk);
        } else if (vk == VK_OEM_3) {
            keyName = "`";
        } else if (vk == VK_F1) { keyName = "F1"; }
        else if (vk == VK_F2)   { keyName = "F2"; }
        else if (vk == VK_ESCAPE) { keyName = "Escape"; }

        if (keyName.empty()) continue;

        nf::HotkeyChord chord{ctrl, shift, alt, keyName};
        if (const std::string* cmdId = m_HotkeyMap.Find(chord)) {
            m_CommandRegistry.Execute(*cmdId);
        }
    }
#endif
}

// ---------------------------------------------------------------------------
// UpdateStatusBar
// ---------------------------------------------------------------------------

void EditorApp::UpdateStatusBar()
{
    nf::StatusBarState state;
    state.activeWorld     = m_GameWorld.IsReady() ? "DevWorld" : "No World";
    state.activeTool      = ToolModeName(m_ToolContext.activeMode);
    state.selectionSummary = m_Selection.GetSelectionLabel();
    state.worldReadiness  = m_GameWorld.IsReady() ? "Ready" : "Not Ready";
    state.dirty           = m_ToolContext.worldDirty;
    state.loadedChunkCount = m_GameWorld.GetLoadedChunkCount();
    m_StatusBar.SetState(std::move(state));
}

// ---------------------------------------------------------------------------
// DrawStatusBar
// ---------------------------------------------------------------------------

void EditorApp::DrawStatusBar(float x, float y, float w, float h)
{
    static constexpr uint32_t kStatusBg   = 0x1E1E2EFF;
    static constexpr uint32_t kStatusText = 0xA0A0B0FF;

    m_UIRenderer.DrawRect({x, y, w, h}, kStatusBg);
    m_UIRenderer.DrawRect({x, y, w, 1.f}, 0x444444FF); // top separator

    const float dpi = m_UIRenderer.GetDpiScale();
    std::string status = m_StatusBar.BuildDisplayString();
    m_UIRenderer.DrawText(status, x + 8.f * dpi, y + 3.f * dpi, kStatusText, 1.5f);
}

// ---------------------------------------------------------------------------
// HandleViewportInteraction
// ---------------------------------------------------------------------------

void EditorApp::HandleViewportInteraction()
{
    if (!m_Input.leftJustPressed) return;
    if (!m_Viewport.IsMouseInside()) return;
    // Don't interact when right or middle button is held (camera navigation).
    if (m_Input.rightDown || m_Input.middleDown) return;

    Vector3 origin, direction;
    if (!m_Viewport.PickRay(m_Input.mouseX, m_Input.mouseY, origin, direction))
        return;

    // Raycast into the chunk map.
    auto hit = m_GameWorld.GetChunkMap().RaycastVoxel(
        origin.X, origin.Y, origin.Z,
        direction.X, direction.Y, direction.Z,
        64.f);

    switch (m_ToolContext.activeMode) {
    case nf::EditorToolMode::Select:
    case nf::EditorToolMode::VoxelInspect:
    {
        if (hit.hit) {
            nf::SelectionHandle handle;
            handle.kind  = nf::SelectionKind::Voxel;
            handle.id    = nf::PackVoxelCoord(hit.x, hit.y, hit.z);
            handle.label = "Voxel (" + std::to_string(hit.x) + ", "
                         + std::to_string(hit.y) + ", "
                         + std::to_string(hit.z) + ")";
            m_Selection.Select(handle);
            SyncInspectorToSelection();
            Logger::Log(LogLevel::Debug, "Editor",
                        "Picked voxel at " + handle.label);
        } else {
            m_Selection.Clear();
            SyncInspectorToSelection();
        }
        break;
    }
    case nf::EditorToolMode::VoxelAdd:
    {
        if (hit.hit) {
            // Place a voxel at the adjacent (air) position using an undoable command.
            auto cmd = std::make_shared<VoxelPlaceCommand>(
                m_GameWorld.GetVoxelEditApi(),
                hit.prevX, hit.prevY, hit.prevZ,
                static_cast<NF::Game::VoxelId>(m_ToolContext.selectedVoxelType));
            m_CommandHistory.Push(cmd);
            m_ToolContext.worldDirty = true;
            Logger::Log(LogLevel::Debug, "Editor",
                        "Placed voxel at (" + std::to_string(hit.prevX)
                        + ", " + std::to_string(hit.prevY)
                        + ", " + std::to_string(hit.prevZ) + ")");
        }
        break;
    }
    case nf::EditorToolMode::VoxelRemove:
    {
        if (hit.hit) {
            auto cmd = std::make_shared<VoxelRemoveCommand>(
                m_GameWorld.GetVoxelEditApi(),
                hit.x, hit.y, hit.z);
            m_CommandHistory.Push(cmd);
            m_ToolContext.worldDirty = true;
            Logger::Log(LogLevel::Debug, "Editor",
                        "Removed voxel at (" + std::to_string(hit.x)
                        + ", " + std::to_string(hit.y)
                        + ", " + std::to_string(hit.z) + ")");
        }
        break;
    }
    }
}

// ---------------------------------------------------------------------------
// SyncInspectorToSelection
// ---------------------------------------------------------------------------

void EditorApp::SyncInspectorToSelection()
{
    if (!m_Selection.HasSelection()) {
        m_Inspector.SetSelectedEntity(NullEntity, nullptr);
        m_PropertyInspectorSystem.Clear();
        UpdateViewportHighlight();
        return;
    }

    const auto& handle = m_Selection.GetSelection();
    switch (handle.kind) {
    case nf::SelectionKind::WorldObject:
        m_Inspector.SetSelectedEntity(
            static_cast<EntityId>(handle.id), &m_Level.GetWorld());
        break;
    case nf::SelectionKind::Voxel:
        // Decode packed voxel world position.
        m_Inspector.SetSelectedVoxel(handle, m_GameWorld);
        break;
    default:
        m_Inspector.SetSelectedEntity(NullEntity, nullptr);
        break;
    }

    BuildPropertySetForSelection(handle);
    UpdateViewportHighlight();
}

// ---------------------------------------------------------------------------
// BuildPropertySetForSelection
// ---------------------------------------------------------------------------

void EditorApp::BuildPropertySetForSelection(const nf::SelectionHandle& handle)
{
    nf::PropertySet ps;
    ps.title = "Selection";

    ps.entries.push_back({
        "Label", nf::PropertyType::String, nf::PropertyWidgetHint::TextField,
        handle.label, {}, false, false
    });
    ps.entries.push_back({
        "Id", nf::PropertyType::Int, nf::PropertyWidgetHint::ReadOnlyLabel,
        static_cast<int>(handle.id & 0x7FFFFFFF), {}, true, false
    });

    switch (handle.kind) {
    case nf::SelectionKind::WorldObject:
        ps.entries.push_back({
            "Kind", nf::PropertyType::String, nf::PropertyWidgetHint::ReadOnlyLabel,
            std::string("WorldObject"), {}, true, false
        });
        break;
    case nf::SelectionKind::Chunk:
        ps.entries.push_back({
            "Kind", nf::PropertyType::String, nf::PropertyWidgetHint::ReadOnlyLabel,
            std::string("Chunk"), {}, true, false
        });
        ps.entries.push_back({
            "Visible", nf::PropertyType::Bool, nf::PropertyWidgetHint::Checkbox,
            true, {}, false, false
        });
        break;
    case nf::SelectionKind::Voxel: {
        ps.entries.push_back({
            "Kind", nf::PropertyType::String, nf::PropertyWidgetHint::ReadOnlyLabel,
            std::string("Voxel"), {}, true, false
        });
        const int32_t vx = nf::UnpackVoxelCoord(handle.id, 0);
        const int32_t vy = nf::UnpackVoxelCoord(handle.id, nf::kVoxelCoordBits);
        const int32_t vz = nf::UnpackVoxelCoord(handle.id, nf::kVoxelCoordBits * 2);
        ps.entries.push_back({"X", nf::PropertyType::Int, nf::PropertyWidgetHint::ReadOnlyLabel,
            static_cast<int>(vx), {}, true, false});
        ps.entries.push_back({"Y", nf::PropertyType::Int, nf::PropertyWidgetHint::ReadOnlyLabel,
            static_cast<int>(vy), {}, true, false});
        ps.entries.push_back({"Z", nf::PropertyType::Int, nf::PropertyWidgetHint::ReadOnlyLabel,
            static_cast<int>(vz), {}, true, false});
        const int typeId = static_cast<int>(
            m_GameWorld.GetVoxelEditApi().GetVoxel(vx, vy, vz));
        ps.entries.push_back({
            "Type", nf::PropertyType::Int, nf::PropertyWidgetHint::NumericField,
            typeId, {}, false, false
        });
        break;
    }
    case nf::SelectionKind::Asset:
        ps.entries.push_back({
            "Kind", nf::PropertyType::String, nf::PropertyWidgetHint::ReadOnlyLabel,
            std::string("Asset"), {}, true, false
        });
        break;
    default:
        break;
    }

    m_PropertyInspectorSystem.SetPropertySet(std::move(ps));
}

// ---------------------------------------------------------------------------
// UpdateViewportHighlight
// ---------------------------------------------------------------------------

void EditorApp::UpdateViewportHighlight()
{
    ViewportHighlightState state;
    if (m_Selection.HasSelection()) {
        const auto& h    = m_Selection.GetSelection();
        state.highlightLabel = h.label;
        switch (h.kind) {
        case nf::SelectionKind::WorldObject:
            state.selectedWorldObjectId = h.id;
            break;
        case nf::SelectionKind::Chunk:
            state.selectedChunkId = h.id;
            break;
        case nf::SelectionKind::Voxel:
            state.selectedVoxelId = h.id;
            break;
        default:
            break;
        }
    }
    m_Viewport.SetHighlightState(std::move(state));
}

// ---------------------------------------------------------------------------
// RebuildWorldOutliner
// ---------------------------------------------------------------------------

void EditorApp::RebuildWorldOutliner()
{
    const auto& chunkMap = m_GameWorld.GetChunkMap();
    const auto  coords   = chunkMap.GetLoadedCoords();

    std::vector<RuntimeChunkMetadata> chunks;
    chunks.reserve(coords.size());
    for (const auto& coord : coords) {
        RuntimeChunkMetadata meta;
        // Build a stable chunk ID from the coordinate fields.
        meta.id = (static_cast<std::uint64_t>(static_cast<std::uint32_t>(coord.X)))
                | (static_cast<std::uint64_t>(static_cast<std::uint32_t>(coord.Y)) << 20u)
                | (static_cast<std::uint64_t>(static_cast<std::uint32_t>(coord.Z)) << 40u);
        meta.coordX = coord.X;
        meta.coordY = coord.Y;
        meta.coordZ = coord.Z;
        meta.label  = "Chunk (" + std::to_string(coord.X) + ","
                    + std::to_string(coord.Y) + ","
                    + std::to_string(coord.Z) + ")";
        const auto* chunk = chunkMap.GetChunk(coord);
        meta.loaded = (chunk != nullptr);
        meta.dirty  = (chunk != nullptr && chunk->IsDirty());
        chunks.push_back(std::move(meta));
    }

    m_SceneOutliner.SetChunkData(
        m_GameWorld.IsReady() ? "DevWorld" : "No World",
        std::move(chunks));
}

// ---------------------------------------------------------------------------
// ApplyPropertyEditsToWorld
// ---------------------------------------------------------------------------

void EditorApp::ApplyPropertyEditsToWorld()
{
    if (!m_Selection.HasSelection()) return;
    const auto& handle = m_Selection.GetSelection();

    if (handle.kind == nf::SelectionKind::Voxel) {
        // Check if the "Type" property was edited.
        const auto& ps = m_PropertyInspectorSystem.GetPropertySet();
        for (const auto& entry : ps.entries) {
            if (entry.name == "Type" && entry.dirty && std::holds_alternative<int>(entry.value)) {
                const int32_t vx = nf::UnpackVoxelCoord(handle.id, 0);
                const int32_t vy = nf::UnpackVoxelCoord(handle.id, nf::kVoxelCoordBits);
                const int32_t vz = nf::UnpackVoxelCoord(handle.id, nf::kVoxelCoordBits * 2);

                const auto oldType = m_GameWorld.GetVoxelEditApi().GetVoxel(vx, vy, vz);
                const auto newType = static_cast<NF::Game::VoxelId>(
                    std::max(0, std::min(255, std::get<int>(entry.value))));

                if (newType != oldType) {
                    auto cmd = std::make_shared<VoxelTypeEditCommand>(
                        m_GameWorld.GetVoxelEditApi(), vx, vy, vz, oldType, newType);
                    m_CommandHistory.Push(cmd);
                    m_ToolContext.worldDirty = true;

                    // Refresh the inspector to show updated type name
                    SyncInspectorToSelection();
                    Logger::Log(LogLevel::Info, "Editor",
                                "Inspector edit: voxel type " + std::to_string(oldType)
                                + " -> " + std::to_string(newType)
                                + " at (" + std::to_string(vx) + ", "
                                + std::to_string(vy) + ", " + std::to_string(vz) + ")");
                }
                break;
            }
        }
    }

    // Mark world dirty for any property edit
    m_ToolContext.worldDirty = true;
}

// ---------------------------------------------------------------------------
// ToolModeName
// ---------------------------------------------------------------------------

const char* EditorApp::ToolModeName(nf::EditorToolMode mode) noexcept
{
    switch (mode) {
    case nf::EditorToolMode::Select:       return "Select";
    case nf::EditorToolMode::VoxelInspect: return "VoxelInspect";
    case nf::EditorToolMode::VoxelAdd:     return "VoxelAdd";
    case nf::EditorToolMode::VoxelRemove:  return "VoxelRemove";
    }
    return "Unknown";
}

// ---------------------------------------------------------------------------
// TickFrame
// ---------------------------------------------------------------------------

void EditorApp::TickFrame(float dt)
{
    m_RenderDevice->BeginFrame();
    m_RenderDevice->Clear(0.18f, 0.18f, 0.18f, 1.f);
    m_Level.Update(dt);

    // Process hotkeys before anything else.
    ProcessHotkeys();

    // Compute docking layout FIRST so we know viewport bounds before the 3D pass.
    const float toolbarH  = m_Toolbar.GetHeight() * m_DpiScale;
    const float statusH   = 22.f * m_DpiScale;
    const float dockY     = toolbarH;
    const float dockW     = static_cast<float>(m_ClientWidth);
    const float dockH     = static_cast<float>(m_ClientHeight) - toolbarH - statusH;

    m_DockingSystem.BuildLayout(0.f, dockY, dockW, dockH);

    // Query viewport panel bounds for 3D scene rendering.
    float vpX = 0.f, vpY = 0.f, vpW = 0.f, vpH = 0.f;
    const bool hasViewport = m_DockingSystem.GetPanelRect("Viewport", vpX, vpY, vpW, vpH);

    // ---- Render 3D scene into viewport region ----
    m_MeshCache.RebuildDirty(m_GameWorld.GetChunkMap());
    if (hasViewport && vpW > 0.f && vpH > 0.f) {
        // OpenGL uses a bottom-left origin for glViewport/glScissor while the
        // UI coordinate system has Y=0 at the top of the window. Flip Y so
        // that the GL viewport region matches the UI panel position.
        const int glX = static_cast<int>(vpX);
        const int glY = m_ClientHeight - static_cast<int>(vpY + vpH);
        const int glW = static_cast<int>(vpW);
        const int glH = static_cast<int>(vpH);

        m_RenderDevice->SetViewport(glX, glY, glW, glH);
        m_RenderDevice->EnableScissor(true);
        m_RenderDevice->SetScissorRect(glX, glY, glW, glH);
        m_RenderDevice->Clear(0.12f, 0.12f, 0.14f, 1.f);

        Matrix4x4 view = m_Viewport.GetViewMatrix();
        Matrix4x4 proj = m_Viewport.GetProjectionMatrix();
        m_ForwardRenderer.BeginScene(view, proj);
        m_MeshCache.SetCameraPosition(m_Viewport.GetCameraEye());
        m_MeshCache.Render();
        m_ForwardRenderer.EndScene();

        // Restore full-window viewport for UI pass.
        m_RenderDevice->EnableScissor(false);
        m_RenderDevice->SetViewport(0, 0, m_ClientWidth, m_ClientHeight);

        m_Viewport.SetSceneRendered(true);
    } else {
        // No viewport panel visible — render full scene as fallback.
        Matrix4x4 view = m_Viewport.GetViewMatrix();
        Matrix4x4 proj = m_Viewport.GetProjectionMatrix();
        m_ForwardRenderer.BeginScene(view, proj);
        m_MeshCache.SetCameraPosition(m_Viewport.GetCameraEye());
        m_MeshCache.Render();
        m_ForwardRenderer.EndScene();

        m_Viewport.SetSceneRendered(false);
    }

    // ---- Handle viewport interaction (pick / tool) ----
    HandleViewportInteraction();

    // ---- Update editor state ----
    UpdateStatusBar();
    RebuildWorldOutliner();

    // Update the debug overlay with latest world data.
    m_GameWorld.GetDebugOverlay().Update(
        m_GameWorld.GetConfig(),
        m_GameWorld.IsReady() ? &m_Level.GetWorld() : nullptr,
        m_GameWorld.GetPlayerEntity());

    // Begin UI rendering pass (2-D overlay on top of the 3-D scene)
    m_UIRenderer.SetViewportSize(static_cast<float>(m_ClientWidth),
                                  static_cast<float>(m_ClientHeight));
    m_UIRenderer.BeginFrame();

    m_DockingSystem.Update(dt);
    m_SceneOutliner.Update(dt);
    m_Inspector.Update(dt);
    m_ContentBrowser.Update(dt);
    m_ConsolePanel.Update(dt);
    m_Viewport.Update(dt);
    m_VoxelInspector.Update(dt);
    m_HUDPanel.Update(dt);
    m_WorldDebugPanel.Update(dt);
    if (m_Toolbar.IsPieActive())
        m_InteractionLoop.Tick(dt);

    // Toolbar strip at the top
    m_Toolbar.Draw(0.f, 0.f, static_cast<float>(m_ClientWidth), toolbarH);

    // Docking panels fill the area between toolbar and status bar.
    m_DockingSystem.Draw(0.f, dockY, dockW, dockH);

    // Status bar at the bottom
    DrawStatusBar(0.f, static_cast<float>(m_ClientHeight) - statusH,
                  static_cast<float>(m_ClientWidth), statusH);

    // Menu drop-downs drawn last so they render on top of all panels.
    m_Toolbar.DrawDropdown();

    // Flush all batched UI draw calls to the GPU
    m_UIRenderer.EndFrame();

    m_RenderDevice->EndFrame();
}

void EditorApp::Run() {
    Logger::Log(LogLevel::Info, "Editor", "EditorApp::Run – entering editor loop");

    using Clock = std::chrono::steady_clock;
    auto lastTime = Clock::now();

#ifdef _WIN32
    MSG msg{};
    while (m_Running)
    {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                m_Running = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        // Propagate window resize to the viewport
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
                    m_Viewport.Resize(m_ClientWidth, m_ClientHeight);
                }
            }
        }

        auto now = Clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;
        TickFrame(dt);

        // Clear edge-triggered input flags after all systems have read them.
        m_Input.FlushFrameEvents();
    }
#else
    while (m_Running) {
        auto now = Clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;
        TickFrame(dt);
    }
#endif
}

void EditorApp::Shutdown() {
    Logger::Log(LogLevel::Info, "Editor", "EditorApp::Shutdown");
    m_MeshCache.Shutdown();
    m_ForwardRenderer.Shutdown();
    m_GameWorld.Shutdown();
    m_Level.Unload();
    m_UIRenderer.Shutdown();
    m_RenderDevice->Shutdown();

#ifdef _WIN32
    if (m_Hwnd && IsWindow(static_cast<HWND>(m_Hwnd)))
        DestroyWindow(static_cast<HWND>(m_Hwnd));
    m_Hwnd = nullptr;
    UnregisterClassW(L"NovaForgeEditor", GetModuleHandleW(nullptr));
#endif
}

} // namespace NF::Editor

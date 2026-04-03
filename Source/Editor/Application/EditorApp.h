#pragma once
#include "Renderer/RHI/RenderDevice.h"
#include "Renderer/Pipeline/ForwardRenderer.h"
#include "UI/Rendering/UIRenderer.h"
#include "Engine/ECS/World.h"
#include "Engine/World/Level.h"
#include "Game/World/GameWorld.h"
#include "Game/Interaction/InteractionLoop.h"
#include "Game/Voxel/ChunkMeshCache.h"
#include "Editor/Application/EditorInputState.h"
#include "Editor/Panels/DockingSystem.h"
#include "Editor/Panels/SceneOutliner.h"
#include "Editor/Panels/Inspector.h"
#include "Editor/Panels/ContentBrowser.h"
#include "Editor/Panels/ConsolePanel.h"
#include "Editor/Panels/VoxelInspector.h"
#include "Editor/Panels/HUDPanel.h"
#include "Editor/Panels/EditorToolbar.h"
#include "Editor/Viewport/EditorViewport.h"
#include "Editor/Selection/SelectionService.h"
#include "Editor/Commands/EditorCommandRegistry.h"
#include "Editor/Commands/EditorHotkeyMap.h"
#include "Editor/Tools/EditorToolContext.h"
#include "Editor/Panels/StatusBarPanel.h"
#include "Editor/Commands/EditorCommand.h"
#include <cstdint>
#include <memory>
#include <string>

namespace NF::Editor {

/// @brief Top-level editor application; owns the device, world and level.
class EditorApp {
public:
    /// @brief Initialise all editor subsystems.
    /// @return True on success.
    bool Init();

    /// @brief Run the blocking editor loop.
    void Run();

    /// @brief Tear down all editor subsystems.
    void Shutdown();

    /// @brief Return a pointer to the render device.
    [[nodiscard]] RenderDevice* GetRenderDevice() noexcept { return m_RenderDevice.get(); }

    /// @brief Return a pointer to the active ECS world.
    [[nodiscard]] World* GetWorld() noexcept { return &m_Level.GetWorld(); }

#ifdef _WIN32
    /// @brief Route a Win32 OS event into editor input state.
    /// Called by the Win32 window procedure on the message-dispatch thread.
    void DispatchOsEvent(unsigned msg, uintptr_t wParam, intptr_t lParam) noexcept;
#endif

private:
    std::unique_ptr<RenderDevice> m_RenderDevice;
    ForwardRenderer               m_ForwardRenderer;
    UIRenderer                    m_UIRenderer;
    NF::Game::GameWorld           m_GameWorld;
    NF::Game::InteractionLoop     m_InteractionLoop;
    NF::Game::ChunkMeshCache      m_MeshCache;
    Level                         m_Level;
    bool                          m_Running{false};
    void*                         m_Hwnd{nullptr};
    int                           m_ClientWidth{1280};
    int                           m_ClientHeight{720};

    EditorInputState              m_Input;          ///< Per-frame OS input state.
    float                         m_DpiScale{1.f};  ///< Monitor DPI / 96.

    /// @brief Advance all panels and issue one frame of rendering.
    /// @param dt Elapsed seconds since the previous frame.
    void TickFrame(float dt);

    DockingSystem  m_DockingSystem;
    SceneOutliner  m_SceneOutliner;
    Inspector      m_Inspector;
    ContentBrowser m_ContentBrowser;
    ConsolePanel   m_ConsolePanel;
    EditorViewport m_Viewport;
    VoxelInspector m_VoxelInspector;
    HUDPanel       m_HUDPanel;
    EditorToolbar  m_Toolbar;

    // ---- Editor state systems (reset integration) ----
    nf::SelectionService       m_Selection;
    nf::EditorToolContext      m_ToolContext;
    nf::EditorCommandRegistry  m_CommandRegistry;
    nf::EditorHotkeyMap        m_HotkeyMap;
    nf::StatusBarPanel         m_StatusBar;
    CommandHistory             m_CommandHistory;

    /// @brief Register all editor commands with the command registry.
    void RegisterEditorCommands();

    /// @brief Process hotkey presses against the hotkey map.
    void ProcessHotkeys();

    /// @brief Update the status bar from current editor state.
    void UpdateStatusBar();

    /// @brief Handle viewport left-click for selection and tool actions.
    void HandleViewportInteraction();

    /// @brief Draw the status bar at the bottom of the window.
    void DrawStatusBar(float x, float y, float w, float h);

    /// @brief Apply the current selection to the inspector panel.
    void SyncInspectorToSelection();

    /// @brief Active tool mode display name.
    [[nodiscard]] static const char* ToolModeName(nf::EditorToolMode mode) noexcept;
};

} // namespace NF::Editor

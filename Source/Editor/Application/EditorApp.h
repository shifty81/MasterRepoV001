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
#include "Editor/Viewport/TransformGizmo.h"
#include "Editor/Selection/SelectionService.h"
#include "Editor/Inspector/PropertyInspectorSystem.h"
#include "Editor/Commands/EditorCommandRegistry.h"
#include "Editor/Commands/EditorHotkeyMap.h"
#include "Editor/Tools/EditorToolContext.h"
#include "Editor/Panels/StatusBarPanel.h"
#include "Editor/Panels/WorldDebugPanel.h"
#include "Editor/Commands/EditorCommand.h"
#include "Editor/Application/EditorWorldSession.h"
#include "Editor/Application/EditorModeManager.h"
#include "Editor/Application/ContextToolShelf.h"
#include "Editor/Panels/PreferencesPanel.h"
#include "Editor/Panels/MaterialEditorPanel.h"
#include "Editor/Panels/LiveProfilerBackend.h"
#include "Editor/Panels/LiveProfilerPanel.h"
#include "Editor/Panels/SolarSystemPanel.h"
#include "Editor/Panels/EconomyPanel.h"
#include "Editor/Panels/InventoryPanel.h"
#include "Game/Character/PlayerCharacterRenderer.h"
#include "Game/Movement/PlayerMovement.h"
#include "Game/Gameplay/SolarSystem/DevSolarSystem.h"
#include "Game/Gameplay/PCG/PCGItemGen.h"
#include "Game/Gameplay/Exploration/ExplorationSystem.h"
#include "Game/Gameplay/Economy/ResourceRegistry.h"
#include "Game/Gameplay/Station/StationServices.h"
#include "Game/Gameplay/Salvage/SalvageSystem.h"
#include "Game/Gameplay/Storage/StorageSystem.h"
#include "Game/Gameplay/Inventory/InventorySystem.h"
#include "Game/Gameplay/Fleet/ShipRenderer.h"
#include "Game/Components/PositionComponent.h"
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

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

    // ---- Editor state systems ----
    nf::SelectionService        m_Selection;
    nf::PropertyInspectorSystem m_PropertyInspectorSystem;
    nf::EditorToolContext       m_ToolContext;
    nf::EditorCommandRegistry   m_CommandRegistry;
    nf::EditorHotkeyMap         m_HotkeyMap;
    nf::StatusBarPanel          m_StatusBar;
    CommandHistory              m_CommandHistory;
    WorldDebugPanel             m_WorldDebugPanel;
    EditorWorldSession          m_WorldSession;
    EditorModeManager           m_ModeManager;
    ContextToolShelf            m_ContextShelf;
    PreferencesPanel            m_PreferencesPanel;
    MaterialEditorPanel         m_MaterialEditor;
    TransformGizmo              m_TransformGizmo;
    LiveProfilerBackend         m_ProfilerBackend;
    LiveProfilerPanel           m_ProfilerPanel;
    SolarSystemPanel            m_SolarSystemPanel;
    EconomyPanel                m_EconomyPanel;
    InventoryPanel              m_InventoryPanel;
    NF::Game::PlayerCharacterRenderer m_CharacterRenderer;
    NF::Game::Gameplay::ShipRenderer   m_ShipRenderer;
    NF::Game::Gameplay::DevSolarSystem m_DevSolarSystem;
    NF::Game::Gameplay::PCGItemGen     m_PCGItemGen;
    NF::Game::Gameplay::ExplorationSystem m_ExplorationSystem;
    NF::Game::Gameplay::ResourceRegistry  m_Resources;
    NF::Game::Gameplay::StationServices   m_EditorStation{"Homebase"};
    NF::Game::Gameplay::SalvageSystem     m_EditorSalvage;
    NF::Game::Gameplay::StorageSystem     m_EditorStorage;
    NF::Game::Gameplay::InventorySystem   m_EditorInventorySys;

    // ---- PIE (Play-In-Editor) state ----
    NF::Game::PlayerMovement m_PiePlayer;   ///< FPS player — always active; noclip in edit, physics in PIE.
    bool  m_WasPiePlaying{false};           ///< Tracks PIE state transitions.
    bool  m_FpsCursorHidden{false};         ///< True when cursor is hidden for FPS look.

    // ---- Gizmo drag state ----
    bool        m_GizmoWasActive{false};    ///< Gizmo drag active on previous frame.
    NF::Vector3 m_GizmoDragStartPos{};      ///< Entity position at the start of the gizmo drag.

    // ---- World picker overlay ----
    bool                     m_WorldPickerOpen{false};   ///< True when the world picker overlay is shown.
    std::vector<std::string> m_WorldPickerWorlds;        ///< World names populated when picker opens.

    /// @brief Read keyboard/mouse input and forward to PIE PlayerMovement.
    void HandlePieInput(float dt);

    /// @brief Compute first-person view matrix from PIE player state.
    [[nodiscard]] Matrix4x4 GetPieViewMatrix() const noexcept;

    /// @brief Compute projection matrix for PIE first-person camera.
    [[nodiscard]] Matrix4x4 GetPieProjectionMatrix() const noexcept;

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

    /// @brief Build a PropertySet from the current selection and push it
    ///        to m_PropertyInspectorSystem.
    void BuildPropertySetForSelection(const nf::SelectionHandle& handle);

    /// @brief Sync the viewport highlight overlay to the current selection.
    void UpdateViewportHighlight();

    /// @brief Rebuild SceneOutliner chunk tree from the live GameWorld.
    void RebuildWorldOutliner();

    /// @brief Apply property edits from the inspector back into world state.
    void ApplyPropertyEditsToWorld();

    /// @brief Active tool mode display name.
    [[nodiscard]] static const char* ToolModeName(nf::EditorToolMode mode) noexcept;

    /// @brief Regenerate the dev solar system from the current world seed.
    void RegenerateSolarSystem();

    /// @brief Draw the world-picker modal overlay (populated by File.OpenWorld).
    void DrawWorldPickerOverlay();
};

} // namespace NF::Editor

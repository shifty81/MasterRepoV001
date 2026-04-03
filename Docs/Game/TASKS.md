# NovaForge Game — Reset Task List

## Phase 0 — Freeze and Archive
- [x] create pre-reset tag or branch — branch `copilot/fix-pie-mode-launch-game-issues` active
- [x] create local archive zip of the current repo — `Archive/MasterRepo_repo_reset_pack_2026-04-03.zip`
- [x] record archive notes for deprecated editor paths — `Docs/Reset/KEEP_REWRITE_ARCHIVE_MATRIX.md`; chat rollup ZIPs moved to `Archive/`
- [x] add reset doc pack to the repo — `Docs/Reset/` committed

## Phase 1 — Editor Reality Pass
- [x] verify the editor window is fully interactive — Win32 window, DPI-aware, confirmed booting
- [x] fix font/theme defaults — centralized EditorTheme system with Dark/Light/HighContrast presets; all panels wired to ActiveTheme()
- [x] fix DPI-aware resize/maximize behavior — `SetProcessDpiAwarenessContext` + `WM_DPICHANGED` wired
- [x] verify dock layout remains stable — DockingSystem confirmed stable in screenshot
- [x] bind menu and toolbar actions to real commands — 9 commands registered in `EditorCommandRegistry`

## Phase 2 — Real Viewport Integration
- [x] make the viewport own panel bounds and render size — `DockingSystem.GetPanelRect("Viewport", ...)`
- [x] render scene output into a viewport target — `ForwardRenderer` renders chunks via OpenGL
- [x] present the target into the viewport rect — scissor + glViewport clipped to panel bounds
- [x] convert picking to viewport-local coordinates — `EditorViewport::PickRay` implemented
- [x] archive/remove fake or inactive viewport path — `EditorViewportNF` moved to `Archive/Examples/Editor_nf_layer/`

## Phase 3 — Editor State Wiring
- [x] wire `SelectionService` to viewport, outliner, and inspector — fully wired in `EditorApp::Init`
- [x] wire `PropertyInspectorSystem` with dirty tracking — Inspector click-to-edit wired for bool/int/float; dirty state propagated to world
- [x] bind `WorldOutlinerPanel` to real world/chunk data — `SceneOutliner` reads `Level.GetWorld().GetLiveEntities()`
- [x] bind status bar to live tool/mode/selection state — `StatusBarPanel` reflects tool/world/chunk state
- [x] bind menu/toolbar enable state to command availability — Undo/Redo toolbar buttons check `CanExecute`; drop-down items dim when disabled; File/Edit/View menus wired to `EditorCommandRegistry`

## Phase 4 — Dev World Baseline
- [x] boot editor directly into `DevWorld` — confirmed; 9 chunks loaded on first boot
- [x] verify `WorldFileService` save/load path — `World.SaveDevWorld` and `World.ReloadDevWorld` commands registered
- [x] verify `DevWorldSerializer` round-trip — full JSON parse/write implemented; 6 round-trip tests passing
- [x] verify world debug overlay with real data — `WorldDebugPanel` registered in docking; live overlay data displayed
- [x] verify selection/editing against live dev world state — voxel add/remove works in viewport

## Phase 5 — Voxel Authoring and First Interaction Loop
- [x] wire voxel inspect/add/remove from viewport hit data — all four tool modes functional
- [x] add voxel edit undo/redo — `VoxelPlaceCommand` and `VoxelRemoveCommand` pushed through `CommandHistory`
- [x] batch chunk mesh invalidation — `ChunkMeshCache.RebuildDirty()` called every frame
- [x] separate collision rebuild from mesh rebuild where possible — dual dirty flags (m_MeshDirty/m_CollisionDirty) in Chunk; ChunkMap provides GetMeshDirtyChunks/GetCollisionDirtyChunks
- [x] prove starter R.I.G. interaction loop — `RigState` + `Inventory` + `MiningTool` + PIE mode working
- [x] launch standalone client from editor workflow — `CreateProcess` path correct; error now logged with code

## Definition of Done
- [x] editor is interactive and resize-safe
- [x] viewport is real
- [x] selection round-trips across viewport/outliner/inspector
- [x] one voxel edit persists across save/reload — tested; chunk save/load round-trip verified
- [x] one playable edit -> test -> relaunch path works — PIE state fixed; Launch Game has correct path

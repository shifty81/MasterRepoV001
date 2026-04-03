# Implementation Master Checklist

## Direction Lock
- [x] README matches standalone native C++ NovaForge direction
- [x] Roadmap matches current milestone reality
- [x] Atlas Suite language removed from active docs
- [x] WPF language removed from active docs
- [x] mega-monorepo language removed from active docs

## Archive Hygiene
- [x] duplicate audit docs moved — fully-merged zips and stale archive files moved to `to_remove/`
- [x] old reset payloads moved — 3 zips and builderrors moved to `to_remove/`
- [x] error dumps isolated — builderrors moved to `to_remove/`
- [x] stale source paths moved to Archive/_stale_source — EditorViewportNF moved to `to_remove/Archive_Examples_Editor_nf_layer/`

## Editor Trust Layer
- [x] EditorCommandHistory authoritative — CommandHistory in EditorApp
- [x] VoxelAddCommand implemented — VoxelPlaceCommand in VoxelEditCommands.h
- [x] VoxelRemoveCommand implemented — VoxelRemoveCommand in VoxelEditCommands.h
- [x] PropertyEditCommand implemented where needed — VoxelTypeEditCommand for inspector edits
- [x] Undo wired to command history — Edit.Undo registered in EditorCommandRegistry
- [x] Redo wired to command history — Edit.Redo registered in EditorCommandRegistry
- [x] toolbar buttons honor CanExecute state — Undo/Redo check CanExecute
- [x] menu items honor CanExecute state — drop-down items dim when disabled

## Inspector
- [x] selected voxel fields visible — position and type shown in Inspector
- [x] selected voxel type editable — click to increment/decrement type in property grid
- [x] change writes into world state — VoxelTypeEditCommand applies edit via VoxelEditApi
- [x] change participates in undo/redo — VoxelTypeEditCommand pushed to CommandHistory
- [x] dirty state updates after edit — m_ToolContext.worldDirty set on edit

## Persistence
- [x] EditorWorldSession created or hardened
- [x] save command writes dev world — saves entities + chunks to Content/Worlds/
- [x] reload command reloads same dev world — loads saved chunk data after re-init
- [x] round-trip smoke test added — 2 persistence tests (326 total)
- [x] world dirty flag resets correctly after save

## Viewport and World Truth
- [x] one authoritative viewport implementation remains active — EditorViewport
- [x] stale viewport implementation archived — EditorViewportNF in Archive/
- [x] viewport-local picking validated — EditorViewport::PickRay implemented
- [x] outliner represents real world state — RebuildWorldOutliner feeds live chunk data
- [x] selection service reflects chunk/entity/voxel truth — voxel picking wired

## Boot and Output
- [x] editor boots cleanly from normal build output
- [x] game boots cleanly from normal build output
- [x] Launch Game path documented and stabilized — CreateProcess logs error code
- [x] project paths resolved through one service — ProjectPathService
- [x] output directory structure normalized

## Minimum Usability
- [x] docking persistence works — DockingSystem confirmed stable
- [x] resize behavior stable — DPI-aware resize/maximize
- [x] maximize behavior stable — WM_DPICHANGED handler wired
- [x] minimum readable font/theme pass complete — centralized EditorTheme with Dark/Light/HighContrast; all panels wired
- [x] command feedback visible in status/log — StatusBarPanel reflects state

## Validation
- [x] voxel select works
- [x] voxel add works
- [x] voxel remove works
- [x] undo works
- [x] redo works
- [x] save works — saves entities + chunks; dirty flag resets
- [x] reload works — loads saved chunk data; verified in round-trip test
- [x] standalone game reflects saved state

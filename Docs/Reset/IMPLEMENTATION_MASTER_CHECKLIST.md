# Implementation Master Checklist

## Direction Lock
- [x] README matches standalone native C++ NovaForge direction
- [x] Roadmap matches current milestone reality
- [x] Atlas Suite language removed from active docs
- [x] WPF language removed from active docs
- [x] mega-monorepo language removed from active docs

## Archive Hygiene
- [ ] duplicate audit docs moved to Archive/_superseded_docs
- [ ] old reset payloads moved to Archive/_old_reset_payloads
- [ ] error dumps isolated
- [x] stale source paths moved to Archive/_stale_source — EditorViewportNF moved to Archive/Examples/

## Editor Trust Layer
- [x] EditorCommandHistory authoritative — CommandHistory in EditorApp
- [x] VoxelAddCommand implemented — VoxelPlaceCommand in VoxelEditCommands.h
- [x] VoxelRemoveCommand implemented — VoxelRemoveCommand in VoxelEditCommands.h
- [ ] PropertyEditCommand implemented where needed
- [x] Undo wired to command history — Edit.Undo registered in EditorCommandRegistry
- [x] Redo wired to command history — Edit.Redo registered in EditorCommandRegistry
- [x] toolbar buttons honor CanExecute state — Undo/Redo check CanExecute
- [x] menu items honor CanExecute state — drop-down items dim when disabled

## Inspector
- [x] selected voxel fields visible — position and type shown in Inspector
- [ ] selected voxel type editable
- [ ] change writes into world state
- [ ] change participates in undo/redo
- [x] dirty state updates after edit — m_ToolContext.worldDirty set on edit

## Persistence
- [ ] EditorWorldSession created or hardened
- [x] save command writes dev world — World.SaveDevWorld registered
- [x] reload command reloads same dev world — World.ReloadDevWorld registered
- [ ] round-trip smoke test added
- [ ] world dirty flag resets correctly after save

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
- [ ] output directory structure normalized

## Minimum Usability
- [x] docking persistence works — DockingSystem confirmed stable
- [x] resize behavior stable — DPI-aware resize/maximize
- [x] maximize behavior stable — WM_DPICHANGED handler wired
- [ ] minimum readable font/theme pass complete — stb_easy_font functional, no bitmap font yet
- [x] command feedback visible in status/log — StatusBarPanel reflects state

## Validation
- [x] voxel select works
- [x] voxel add works
- [x] voxel remove works
- [x] undo works
- [x] redo works
- [ ] save works — registered but round-trip untested
- [ ] reload works — registered but round-trip untested
- [ ] standalone game reflects saved state

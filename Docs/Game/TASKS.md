# NovaForge Game — Reset Task List

## Phase 0 — Freeze and Archive
- [ ] create pre-reset tag or branch
- [ ] create local archive zip of the current repo
- [ ] record archive notes for deprecated editor paths
- [x] add reset doc pack to the repo

## Phase 1 — Editor Reality Pass
- [ ] verify the editor window is fully interactive
- [ ] fix font/theme defaults
- [ ] fix DPI-aware resize/maximize behavior
- [ ] verify dock layout remains stable
- [ ] bind menu and toolbar actions to real commands

## Phase 2 — Real Viewport Integration
- [ ] make the viewport own panel bounds and render size
- [ ] render scene output into a viewport target
- [ ] present the target into the viewport rect
- [ ] convert picking to viewport-local coordinates
- [ ] archive/remove fake or inactive viewport path

## Phase 3 — Editor State Wiring
- [ ] wire `SelectionService` to viewport, outliner, and inspector
- [ ] wire `PropertyInspectorSystem` with dirty tracking
- [ ] bind `WorldOutlinerPanel` to real world/chunk data
- [ ] bind status bar to live tool/mode/selection state
- [ ] bind menu/toolbar enable state to command availability

## Phase 4 — Dev World Baseline
- [ ] boot editor directly into `DevWorld`
- [ ] verify `WorldFileService` save/load path
- [ ] verify `DevWorldSerializer` round-trip
- [ ] verify world debug overlay with real data
- [ ] verify selection/editing against live dev world state

## Phase 5 — Voxel Authoring and First Interaction Loop
- [ ] wire voxel inspect/add/remove from viewport hit data
- [ ] add voxel edit undo/redo
- [ ] batch chunk mesh invalidation
- [ ] separate collision rebuild from mesh rebuild where possible
- [ ] prove starter R.I.G. interaction loop
- [ ] launch standalone client from editor workflow

## Definition of Done
- [ ] editor is interactive and resize-safe
- [ ] viewport is real
- [ ] selection round-trips across viewport/outliner/inspector
- [ ] one voxel edit persists across save/reload
- [ ] one playable edit -> test -> relaunch path works

# Roadmap Reset — 2026-04-03

## Reset Principle
The repo is not measured by how many panels exist.
It is measured by whether the editor can author and test NovaForge through a real viewport and a real world state.

## Phase A — Repo Freeze and Audit
Goal:
- freeze the current state before major rewrite

Deliverables:
- archive zip
- archive branch/tag
- reset doc pack committed
- conflicting docs marked obsolete or replaced

## Phase B — Editor Reality Pass
Goal:
- correct the editor foundation before adding more systems

Deliverables:
- clickable shell
- DPI-safe windowing
- readable font/theme defaults
- real command routing
- stable dock layout

## Phase C — Real Viewport Integration
Goal:
- make the viewport authoritative

Deliverables:
- render target ownership
- real scene presentation in the center panel
- viewport-local picking
- camera movement with visible output
- fake viewport path removed or archived

## Phase D — Editor State Integration
Goal:
- bind shell and editor state together

Deliverables:
- selection service fully wired
- outliner live data source
- property inspector dirty tracking
- status bar mode/state reporting
- toolbar/menu enable state from real command availability

## Phase E — Dev World Authoring Baseline
Goal:
- make one test world the truth source for early development

Deliverables:
- direct boot to dev world from editor
- save/load through world file services
- world debug overlay
- outliner and inspector reflect dev world state

## Phase F — Voxel Authoring and Interaction
Goal:
- edit the world through the editor and prove a minimal loop

Deliverables:
- voxel inspect/add/remove from viewport
- undo/redo for voxel edits
- chunk rebuild batching
- first interaction loop using starter R.I.G. and basic mining/build/repair

## Phase G — Client Launch and Workflow Hardening
Goal:
- complete the edit -> test -> relaunch path

Deliverables:
- play-in-editor path
- launch standalone client path
- smoke tests for boot, edit, save, reload, play, and relaunch
- docs and task lists updated to actual state

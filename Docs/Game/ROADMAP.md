# NovaForge Game — Reset Roadmap

## Repo Identity
NovaForge is a standalone repo for the game, engine, and game-specific editor.
The editor exists to build NovaForge.
The editor does not ship with the game.
This repo does not own Atlas Suite or generic workspace tooling.

## Reset Order

### Phase 0 — Freeze and Archive
Goal: preserve the current repo before destructive refactor.

Deliverables:
- pre-reset archive zip
- pre-reset tag or branch
- archive notes for deprecated editor paths
- reset doc pack committed into the repo

### Phase 1 — Editor Reality Pass
Goal: make the editor shell usable before adding more systems.

Deliverables:
- clickable editor window
- readable theme and font defaults
- DPI-aware resize and maximize behavior
- stable dock layout
- real command routing for menu and toolbar actions

### Phase 2 — Real Viewport Integration
Goal: make the center panel authoritative.

Deliverables:
- viewport owns panel bounds and render size
- real render target presented into viewport rect
- camera motion changes real rendered output
- viewport-local pick-ray generation
- fake or inactive viewport path archived or removed

### Phase 3 — Editor State Wiring
Goal: synchronize viewport, outliner, inspector, and command state.

Deliverables:
- selection service fully wired
- world outliner backed by real world state
- property inspector with dirty tracking
- status bar bound to real mode/state
- toolbar/menu enable state derived from command availability

### Phase 4 — Dev World Baseline
Goal: use one stable world as the early source of truth.

Deliverables:
- editor boots directly into the dev world
- save/load through world file services
- world debug overlay
- selection and property editing against real dev world entities/chunks/voxels

### Phase 5 — Voxel Authoring and First Interaction Loop
Goal: prove real authoring and minimal gameplay through the editor.

Deliverables:
- voxel inspect/add/remove in viewport
- undo/redo command path for edits
- chunk rebuild batching and refresh
- starter R.I.G. + minimal mining/build/repair loop
- play-in-editor and standalone relaunch path

## Deferred Until After Phase 5
- low-poly wrapper implementation
- fleet gameplay
- broad galaxy simulation
- advanced multiplayer expansion
- non-critical editor luxury panels
- Atlas-linked workspace features

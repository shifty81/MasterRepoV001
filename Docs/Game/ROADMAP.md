# NovaForge Roadmap

## Repo Identity
NovaForge is a standalone native C++ repo for the game, engine, renderer, and game-specific editor.
The editor exists to build NovaForge.
The editor does not ship with the game.
This repo does not own Atlas Suite, WPF shell work, or generalized tooling ambitions.

## Direction Lock
The project is now locked to an editor-first, voxel-first implementation path.
The current goal is not feature expansion. The current goal is editor trustworthiness.

## Phase 0 — Bootstrap and Reset Baseline
Status: Done

Goal:
- clean repo structure
- build system
- editor and game program entrypoints
- content/config/docs/test layout

## Phase 1 — Editor Core Trust Layer
Status: Done

Goal:
Make the editor a trustworthy tool instead of a promising shell.

Required outputs:
- authoritative selection service
- property inspector with read and write support
- editor command history
- voxel add/remove commands routed through undo/redo
- toolbar and menu state derived from command availability
- stable docking, input routing, and panel resize behavior

Done when:
- click voxel -> select ✅
- edit voxel/property -> change applies ✅
- undo -> change reverts ✅
- redo -> change reapplies ✅

## Phase 2 — Persistence and Dev World Round-Trip
Status: Done

Goal:
Prove that edits survive save and reload.

Required outputs:
- editor world session management
- save/load pipeline for dev world
- dirty state tracking
- project/path resolution cleanup
- smoke-tested save/reload round-trip

Done when:
- edit world -> save -> reload -> same result ✅
- standalone game can load the same saved state ✅

## Phase 3 — Viewport and World Truth
Status: Done

Goal:
Make viewport, outliner, inspector, and actual world state reflect the same truth.

Required outputs:
- single authoritative viewport path
- viewport-local picking
- chunk/entity/voxel world representation cleanup
- outliner backed by real world data
- stale parallel viewport/editor paths archived or removed

Done when:
- visible world, selected world, and saved world all match ✅

## Phase 4 — Dev World Interaction Loop
Status: Done

Goal:
Use one stable dev world to prove the first game-authoring loop.

Required outputs:
- starter R.I.G. state
- mining/place/repair loop through editor and standalone game
- basic HUD and loop validation
- play/relaunch flow from editor

All outputs verified. ✅

## Phase 5 — Runtime Expansion
Status: Current Active Phase

Goal:
Grow the game loop beyond the DevWorld interaction loop toward a real playable game.
All work must be exercised in DevWorld/DevSolarSystem before it goes to main.

### Phase 5 Checklist

**Editor Improvements**
- [x] PIE crosshair overlay in viewport when Playing
- [x] PCGItemGen save/load (.nfpi) — placed items persist across sessions
- [x] Functional TransformGizmo — drag-to-move entities in the viewport
- [x] Full PIE HUD mirror — inventory slots, active tool feedback, position, mine flash

**Solar System → In-Game**
- [x] GameClientApp solar map overlay (M key, 2D orbital map with bodies and deposits)
- [x] Travel mechanic skeleton — EditorWorldSession::TravelToBody(bodyId) swaps world context
- [x] Per-body terrain generation — PCGWorldGen seeded from body ID generates unique terrain
- [x] Resource deposits → voxel terrain bridge — deposit positions influence ore placement per body
- [ ] Dev-only access gate — config flag exposes solar map in game without shipping to players

**Core Gameplay Systems**
- [ ] ExplorationSystem skeleton — enough to own body-to-body travel state
- [ ] CombatSystem basics — simple melee/ranged hit detection on RigState
- [ ] BuilderSystem + ship construction mode — block palette, ship-tagged context
- [ ] Economy basics — ResourceRegistry live, TradeMarket with at least 2 stations
- [ ] ProgressionSystem wired to gameplay — XP awarded for mining, building, kills
- [ ] MissionRegistry — 2–3 starter missions spawnable in DevWorld

**Multiplayer Foundation**
- [ ] Real socket listen/connect — GameServer/GameClient over TCP loopback
- [ ] Snapshot replication — authoritative server sends player positions to client
- [ ] Dedicated server headless mode verified in CI

## Explicitly Deferred
Do not prioritize these until earlier phases are complete:
- visual scripting
- generalized IDE/tool suite work
- Atlas Suite concepts
- WPF shell concepts
- advanced AI systems
- broad server/admin platform work
- luxury editor panels not tied to core authoring loop

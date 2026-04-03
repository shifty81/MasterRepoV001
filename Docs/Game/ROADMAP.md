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
Status: Current Active Phase

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
- click voxel -> select
- edit voxel/property -> change applies
- undo -> change reverts
- redo -> change reapplies

## Phase 2 — Persistence and Dev World Round-Trip
Status: Next

Goal:
Prove that edits survive save and reload.

Required outputs:
- editor world session management
- save/load pipeline for dev world
- dirty state tracking
- project/path resolution cleanup
- smoke-tested save/reload round-trip

Done when:
- edit world -> save -> reload -> same result
- standalone game can load the same saved state

## Phase 3 — Viewport and World Truth
Status: Next

Goal:
Make viewport, outliner, inspector, and actual world state reflect the same truth.

Required outputs:
- single authoritative viewport path
- viewport-local picking
- chunk/entity/voxel world representation cleanup
- outliner backed by real world data
- stale parallel viewport/editor paths archived or removed

Done when:
- visible world, selected world, and saved world all match

## Phase 4 — Dev World Interaction Loop
Status: Deferred until phases 1-3 pass

Goal:
Use one stable dev world to prove the first game-authoring loop.

Required outputs:
- starter R.I.G. state
- mining/place/repair loop through editor and standalone game
- basic HUD and loop validation
- play/relaunch flow from editor

## Phase 5 — Runtime Expansion
Status: Deferred

Goal:
Only after editor trust and persistence are solid.

Potential outputs later:
- broader gameplay systems
- low-poly wrapper layer
- fleet and galaxy systems
- expanded content pipeline
- broader multiplayer foundations

## Explicitly Deferred
Do not prioritize these until earlier phases are complete:
- visual scripting
- generalized IDE/tool suite work
- Atlas Suite concepts
- WPF shell concepts
- advanced AI systems
- broad server/admin platform work
- luxury editor panels not tied to core authoring loop

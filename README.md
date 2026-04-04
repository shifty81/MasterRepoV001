# NovaForge

> Standalone native C++ voxel game, engine, and game-specific editor in one repo.

NovaForge is a voxel-first first-person survival, construction, and exploration project built around a custom engine, a custom editor, and a standalone game client. The editor exists to build NovaForge. It does not ship with the game.

## Repo Direction Lock

This repository is locked to one direction:

- standalone repo
- native C++ only
- engine + renderer + game + game-specific editor
- editor-first workflow
- voxel-first world authority
- no WPF
- no Atlas Suite integration in this repo
- no generalized tool-suite or IDE expansion in this repo

## Current Status

Phases 1–4 complete ✅ — editor trust, persistence, viewport truth, and interaction loop all verified.

Current milestone:
**Phase 5 — Runtime Expansion** — polish and hardening before broader gameplay

All core editor systems are implemented and verified:
- authoritative viewport with OpenGL rendering and viewport-local picking
- voxel authoring (select, inspect, add, remove) with undo/redo
- inspector property editing with write-back to world state
- save/load round-trip verified (entity + chunk persistence)
- tabbed panel docking system (Inspector/VoxelInspector/Preferences tabs, Console/ContentBrowser tabs)
- full-width bottom dock, Unreal-like panel layout
- preferences panel with theme cycling (Dark/Light/HighContrast) and INI persistence
- clean standalone game launch from editor with saved-state loading
- collision-dirty and mesh-dirty flags separated for independent rebuild
- embedded 8×14 bitmap font with texture atlas (replaced stb_easy_font)
- normalized output directories (VS Debug/Release and CMake write to same path)
- 326 tests passing

## What Is Already Present

- modular C++ source layout across Core, Engine, Renderer, UI, Game, Editor, and Programs
- custom editor executable and standalone game executable entrypoints
- custom OpenGL viewport with orbit camera and viewport-local picking
- tabbed docking/panel editor layout with DPI-aware resizing
- voxel select, inspect, add, and remove with undo/redo
- selection and inspector systems with property write-back
- command routing with hotkey bindings
- mode tabs (Select/Voxels/Entities/World/Debug) with context tool shelf
- centralized EditorTheme system (Dark/Light/HighContrast)
- world persistence (entity header + chunk data)
- project manifest and content/config structure
- test suite with 326 passing tests across major systems
- roadmap, reset docs, and implementation planning packs

## Active Development

Next focus areas:
- expanded mode-specific context shelf controls
- additional panel polish and workflow improvements

## Active Repo Boundary

This repo owns:
- NovaForge engine runtime
- renderer
- voxel runtime
- game-specific editor
- standalone game client
- content, config, docs, and tests for NovaForge

This repo does not own:
- Atlas Suite
- WPF shell work
- generalized tooling platform
- AI broker platform
- custom IDE ambitions
- unrelated server/admin products

## Roadmap

See:
- [`Docs/Game/ROADMAP.md`](Docs/Game/ROADMAP.md)
- [`Docs/Game/AUDIT_2026-04-03.md`](Docs/Game/AUDIT_2026-04-03.md)
- [`Docs/Reset/COMPREHENSIVE_REPO_RESET_MASTER.md`](Docs/Reset/COMPREHENSIVE_REPO_RESET_MASTER.md)
- [`Docs/Reset/IMPLEMENTATION_MASTER_CHECKLIST.md`](Docs/Reset/IMPLEMENTATION_MASTER_CHECKLIST.md)

## License

GNU General Public License v3.0. See [LICENSE](LICENSE).

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

Current milestone:
**Editor trustworthiness and persistence**

That means the active work is:
- authoritative viewport/editor state
- voxel authoring safety
- undo/redo
- inspector write-back
- save/load round-trip validation
- clean standalone game launch

## What Is Already Present

- modular C++ source layout across Core, Engine, Renderer, UI, Game, Editor, and Programs
- custom editor executable and standalone game executable entrypoints
- custom OpenGL viewport path
- docking/panel editor layout
- voxel selection and add/remove interaction foundation
- selection and inspector systems
- command routing foundation
- project manifest and content/config structure
- test suite scaffold across major systems
- roadmap, reset docs, and implementation planning packs

## What Is Not Yet Trusted

The repo should not be treated as feature-complete.
These areas still require verification or completion:
- voxel undo/redo wiring
- inspector editing path
- save/load round-trip proof
- outliner truth against world/chunk state
- toolbar/menu command enable state
- stale parallel editor path cleanup
- output path and launch reliability across build workflows

## Immediate Build Order

1. editor command history completion
2. voxel edit commands
3. inspector write-back
4. save/load validation
5. outliner/world truth cleanup
6. single authoritative viewport path
7. executable output cleanup
8. minimum usability pass

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

## Definition of Done for Current Milestone

The current milestone is done only when:
- editor boots cleanly
- viewport interaction works reliably
- voxel edits can be undone and redone
- inspector edits write back into world state
- save then reload preserves world edits
- standalone game launches from the same repo state consistently

## License

GNU General Public License v3.0. See [LICENSE](LICENSE).

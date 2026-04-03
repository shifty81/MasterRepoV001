# Comprehensive Repo Reset Master

## Purpose
This document is the single integration brief for rebuilding or normalizing the repository from all currently available materials:
- uploaded repo reset zip
- archive zips already included in the repo
- prior planning and audit chats distilled into implementation form
- existing source tree, docs, and code stubs

Use this as the source-of-truth brief for any AI or human doing a repo reset.

## Non-Negotiable Direction
NovaForge is:
- standalone
- native C++
- game + engine + renderer + game-specific editor
- editor-first
- voxel-first
- custom UI
- not WPF
- not Atlas Suite
- not a generalized multi-product monorepo

## Real State of the Repo
The repo is beyond pure scaffolding.
It already contains substantial source across:
- Core
- Engine
- Renderer
- Physics
- Input
- Audio
- Animation
- Networking
- UI
- Game
- Editor
- Programs

The repo is still not trustworthy enough to treat as complete.
The gap is now system completion and consolidation, not blank-page architecture.

## Primary Reset Outcome
Rewrite and merge as necessary so the repo becomes:
1. structurally coherent
2. directionally locked
3. free of stale competing implementations
4. aligned with the active milestone
5. ready for editor-first implementation work

## Merge Rules
- Prefer rewrite over preserving broken or duplicate patterns.
- Prefer one authoritative implementation over multiple variants.
- Preserve useful historical material in Archive only.
- Do not let old docs or old branches override current direction.
- Do not add new major systems outside the active milestone.

## Keep
Keep active and authoritative:
- Source/Core
- Source/Engine
- Source/Renderer
- Source/UI
- Source/Game
- Source/Editor
- Source/Programs
- Config
- Content
- Tests
- current roadmap and reset docs once updated

## Remove from Active Direction
Remove from active messaging, front page, and milestone planning:
- Atlas Suite language
- WPF shell language
- generalized tool-host language
- custom IDE language
- AI broker platform language
- mega-monorepo language
- unrelated admin/server platform scope

## Archive
Archive out of active flow:
- duplicate audits
- superseded roadmap variants
- old reset payloads once merged
- old error dumps
- parallel unused editor panel/view implementations
- experimental code paths not used by current editor app

Suggested locations:
- Archive/_historical
- Archive/_superseded_docs
- Archive/_old_reset_payloads
- Archive/_stale_source

## Build Now
### A. Editor Trust
- complete command history wiring
- add reversible voxel edit commands
- add inspector write-back path
- derive command enable state for toolbar and menu
- clean panel/input state routing

### B. Persistence
- add or harden editor world session
- validate save/load
- add dirty tracking
- verify dev world round-trip

### C. World Truth
- keep one authoritative viewport path
- remove stale viewport alternatives
- fix outliner representation of chunk/entity/voxel state
- ensure selection, inspector, viewport, and persistence agree

### D. Boot Reliability
- stabilize editor boot
- stabilize standalone game boot
- normalize output paths
- normalize project/content/config resolution

### E. Minimum Usability
- panel resize behavior
- docking persistence
- multi-resolution behavior
- readable font/theme pass
- wire menu actions to real commands

## Defer
Do not implement yet:
- broad gameplay expansion
- visual scripting
- advanced content browser ambitions
- plugin system
- AI systems
- broader tool-suite extraction
- low-poly wrapper production workflow
- fleet/galaxy layers

## Required Repo Front Page Positioning
The README must describe current reality:
- standalone native C++ repo
- editor trustworthiness phase
- current gaps honestly stated
- roadmap linked to current implementation order
- no inflated completion claims

## Required Roadmap Positioning
The roadmap must show:
- bootstrap done
- editor trust active
- persistence next
- viewport/world truth next
- dev world interaction after trust and persistence
- everything else deferred

## Required Success Conditions
The repo reset is successful only when:
- repo boundary is unambiguous
- docs no longer conflict
- duplicate paths are archived
- active milestone is editor trustworthiness
- save/load and undo/redo are explicit priorities
- future work is gated by validated behavior

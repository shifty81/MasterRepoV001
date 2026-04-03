# MasterRepo Reset — Gap Matrix

## Confirmed Strengths
- repo already has a meaningful engine/editor/game split
- project manifest and dev world definition exist
- editor source has the right system names for commands, selection, outliner, inspector, and viewport
- game source already has world, voxel, interaction, and serialization scaffolds

## Critical Gaps

### 1. Editor usability gap
Symptoms reported in project chats indicate the editor window currently behaves like a mostly static shell.

Needed:
- clickable input path validation
- DPI/resolution handling
- font/theme cleanup
- stable panel interaction

### 2. Viewport reality gap
The viewport appears to expose counters or camera deltas without authoritative render presentation.

Needed:
- render target ownership
- viewport presentation path
- viewport-local picking
- removal of fake/inactive viewport path

### 3. State synchronization gap
Panels exist, but the loop between viewport, outliner, inspector, and toolbar state is not yet trustworthy.

Needed:
- real selection propagation
- real property editing with dirty tracking
- live world/outliner data binding
- command state binding

### 4. Workflow gap
Editor-first boot, play-in-editor, and standalone launch workflow need to be hardened into one path.

Needed:
- direct dev world boot
- save/load verification
- edit -> test -> relaunch smoke path

## Defer On Purpose
- low-poly wrapper visuals
- broad feature expansion
- non-critical editor panels
- advanced multiplayer/fleet/world scale systems

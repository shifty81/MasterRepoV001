# Editor System Reset Spec

## Core Decision
Stop treating the editor shell as the main deliverable.
The viewport must become real first.

## Problem Statement
The current editor symptoms reported across project chats are consistent with a shell-heavy build where:
- menus and toolbar actions exist visually but do not drive real editor state
- the viewport reports counters or camera deltas but does not present authoritative scene output
- UI scaling and font choices are not production-safe
- input routing is partial or disconnected
- the outliner/inspector/viewport loop is not fully synchronized

## Reset Rule
Every editor feature is now categorized as one of the following:
- **real**: backed by engine/editor state and visible behavior
- **bridge**: a shell control with a defined native/editor backend contract
- **archive**: old path retained only for reference
- **blocked**: not worth touching until the viewport path is real

## Mandatory Editor Targets

### 1. Real viewport composition
Required outcome:
- the center panel presents real render output
- resize changes real render target size
- camera movement changes what is rendered, not just counters
- viewport-local mouse coordinates are used for picking and tools

Required components:
- viewport render target ownership
- present path into editor viewport rect
- viewport-local projection and pick-ray generation
- disabled or removed fake/inactive viewport path

### 2. Real editor interaction loop
Required outcome:
- click in viewport selects something real
- selection is reflected in outliner and inspector
- tool mode changes affect viewport behavior
- play/stop/editor actions run real commands

Required components:
- `SelectionService`
- `EditorCommandRegistry`
- `EditorHotkeyMap`
- `EditorToolContext`
- `VoxelPickService`
- status feedback through `StatusBarPanel`

### 3. Real shell-to-editor state sync
Required outcome:
- outliner rows reflect runtime/editor scene data
- inspector reflects selected object/chunk/voxel data
- toolbar/menu enabled state reflects actual availability
- status bar reports actual mode, selection, and world state

Required components:
- world/outliner adapter
- property editing widgets with dirty tracking
- command enable/disable wiring
- change notification/events

### 4. Usable editor baseline
Required outcome:
- window scales correctly
- controls are clickable
- fonts are readable
- dark theme is coherent
- dock layout remains stable during resize/maximize

Required components:
- DPI-aware window sizing
- font/theme pass
- input hit-testing validation
- stable panel layout defaults

## Archive Rules
Archive, do not delete, any current panel path that is only decorative or redundant.
Move it to an archive namespace or docs record before replacing it.

Archive candidates include:
- fake shell-only panel behaviors
- duplicate panel classes with overlapping roles
- any viewport path that does not own or present authoritative scene output
- transitional bootstrap UX that forces command-prompt-style mode selection when editor-first boot is desired

## Hard Do-Not-Do List
Do not spend time on these before the viewport is real:
- extra menu polish
- broad plugin systems
- advanced material editing UI
- multi-world authoring breadth
- cosmetic shell animation
- secondary panels not tied to the current critical path

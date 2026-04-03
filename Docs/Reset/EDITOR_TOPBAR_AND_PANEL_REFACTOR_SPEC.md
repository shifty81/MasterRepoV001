# EDITOR_TOPBAR_AND_PANEL_REFACTOR_SPEC.md

## Purpose
Define a cleaner native editor layout that reduces UI clutter, improves
discoverability, and shifts the editor toward a more Unreal-like workflow.

## Layout Target

```
┌──────────────────────────────────────────────────────────────────┐
│  Menu Bar:  File  Edit  View  Tools  Build  Play  Window  Help  │  Band 1
├──────────────────────────────────────────────────────────────────┤
│  Toolbar:  Save Undo Redo │ Move Rot Scale │ Play Pause Stop    │  Band 2
├──────────────────────────────────────────────────────────────────┤
│  Mode Tabs:  Select │ Voxels │ Entities │ World │ Debug         │  Band 3
├──────────────────────────────────────────────────────────────────┤
│  Context Shelf: [mode-specific tools & settings]                │  Band 4
├────────┬────────────────────────────────────┬────────────────────┤
│ Scene  │                                    │  Details          │
│Outliner│           Viewport                 │  (tabbed:         │
│        │                                    │   Inspector /     │
│        │                                    │   Tool Settings / │
│        │                                    │   World Settings) │
├────────┴────────────────────────────────────┴────────────────────┤
│  Bottom Dock (tabbed): Content Browser │ Console │ Output       │
├──────────────────────────────────────────────────────────────────┤
│  Status Bar                                                     │
└──────────────────────────────────────────────────────────────────┘
```

## Implementation Phases

### Phase 1 — Structural Cleanup (this PR)
- Add mode tab strip below toolbar.
- Add context tool shelf below mode tabs (content changes with mode).
- Consolidate right-side panels into a single tabbed region.
- Consolidate bottom panels into a single tabbed region.
- Remove low-value permanent panels.

### Phase 2 — Mode Awareness (future)
- Full EditorModeManager driving context shelf content.
- Inspector responds to active mode.
- Shelf shows brush, material, entity, and debug controls per mode.

### Phase 3 — PIE Cleanup (future)
- Play controls in top band.
- Esc-to-return flow.
- Eject / pause / frame-step.

### Phase 4 — Refinement (future)
- Tooltips, iconography, layout persistence, workspace presets.

## Design Rules
1. Viewport is primary — supporting tools must not dominate.
2. Mode determines tool visibility — hide irrelevant controls.
3. Panels must earn permanent space — use tabs or drawers otherwise.
4. Do not duplicate controls across menu, toolbar, shelf, and inspector.
5. Icon-first, dark flat chrome, compact spacing.

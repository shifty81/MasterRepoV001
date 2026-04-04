# Edit-Everything System Matrix

**Status:** Active  
**Purpose:** Single canonical reference mapping every editable entity type to its editor infrastructure.  
Use this matrix before building any new editor panel, command type, or inspector to prevent duplication.

---

## Matrix

| Entity Type | Editor Panel | Edit Command Class | Inspector Panel | Undo Stack Owner | Saved-As Format | Phase |
|---|---|---|---|---|---|---|
| **Voxel** | `BlockAssetEditor` (docked viewport) | `VoxelPlaceCommand`, `VoxelRemoveCommand`, `VoxelFillCommand` | `VoxelInspector` | `BlockAssetEditor::m_CommandHistory` | `.nfblock` (binary voxel grid) | Phase 2 |
| **Chunk** | `EditorViewport` (main scene) | `ChunkEditCommand` (wraps batch of `VoxelPlaceCommand`) | `ChunkInspector` (future) | `EditorWorldSession::m_CommandHistory` | `.nfchunk` (binary chunk data) | Phase 2 |
| **World Object** | `EditorViewport` (main scene) | `PlaceObjectCommand`, `MoveObjectCommand`, `DeleteObjectCommand` | `Inspector` (generic) | `EditorWorldSession::m_CommandHistory` | `.nfobj` (JSON object descriptor) | Phase 1 |
| **Block Asset** | `ContentBrowser` + `BlockAssetEditor` | `RenameAssetCommand`, `SetLODFlagsCommand` | `BlockAssetInspector` | `ContentBrowser::m_CommandHistory` | `.nfblock` header fields | Phase 2 |
| **Ship Definition** | `ShipEditor` (future docked window) | `SetShipPropertyCommand` | `ShipInspector` | `ShipEditor::m_CommandHistory` | `Content/Data/Ships/*.json` | Phase 5+ |
| **Sector** | `GalaxyEditor` (future full-screen) | `SetSectorPropertyCommand`, `MoveSectorCommand` | `SectorInspector` | `GalaxyEditor::m_CommandHistory` | `Content/Data/Universe/*.json` | Phase 5+ |
| **Mission** | `MissionEditor` (future docked window) | `SetMissionPropertyCommand`, `AddObjectiveCommand` | `MissionInspector` | `MissionEditor::m_CommandHistory` | `Content/Data/Missions/*.json` | Phase 5+ |
| **Skill** | `SkillEditor` (future docked window) | `SetSkillPropertyCommand` | `SkillInspector` | `SkillEditor::m_CommandHistory` | `Content/Data/Skills/*.json` | Phase 5+ |
| **Module** | `ModuleEditor` (future docked window) | `SetModulePropertyCommand` | `ModuleInspector` | `ModuleEditor::m_CommandHistory` | `Content/Data/Modules/*.json` | Phase 5+ |
| **NPC** | `NPCEditor` (future docked window) | `SetNPCPropertyCommand`, `AssignBehaviorTreeCommand` | `NPCInspector` | `NPCEditor::m_CommandHistory` | `Content/Data/NPCs/*.json` | Phase 4+ |
| **Animation Clip** | `AnimationEditor` (future docked window) | `AddKeyframeCommand`, `RemoveKeyframeCommand`, `SetKeyframeValueCommand` | `AnimationInspector` | `AnimationEditor::m_CommandHistory` | `Content/Animation/*.nfanim` | Phase 4+ |
| **Graph Node** | `GraphEditor` (future docked window, uses `LayeredViewport`) | `AddNodeCommand`, `RemoveNodeCommand`, `ConnectPinsCommand`, `DisconnectPinsCommand` | `NodeInspector` | `GraphEditor::m_CommandHistory` | `Content/Graphs/*.nfgraph` | Phase 3 (NodeGraph module) |

---

## Column Definitions

| Column | Description |
|---|---|
| **Editor Panel** | The docked panel or full-screen window that provides the editing UI for this entity type. "Future" indicates planned but not yet built. |
| **Edit Command Class** | One or more `Command`-derived classes that encapsulate a single undoable edit. All commands implement `Execute()`, `Undo()`, and `GetDescription()`. |
| **Inspector Panel** | The right-dock panel that displays and edits properties of a selected entity of this type. |
| **Undo Stack Owner** | The object that owns the `CommandHistory` instance for this entity type's edits. Each editor session has its own independent history. |
| **Saved-As Format** | The file format and path where the edited data is persisted. |
| **Phase** | The NovaForge development phase in which this entity type's editor infrastructure is built. |

---

## Command Base Interface

All edit commands derive from:

```cpp
// Source/Editor/Commands/Command.h
namespace NF::Editor {

class Command {
public:
    virtual ~Command() = default;
    virtual void Execute() = 0;
    virtual void Undo() = 0;
    [[nodiscard]] virtual std::string GetDescription() const = 0;
};

} // namespace NF::Editor
```

`CommandHistory` owns a `std::deque<std::unique_ptr<Command>>` and implements Ctrl+Z / Ctrl+Y.

---

## Inspector Panel Registration

All inspector panels are registered in `EditorApp::Init` and activated by selection type:

```cpp
// Pattern: EditorApp sets active inspector based on what is selected
void EditorApp::OnSelectionChanged(const SelectionContext& ctx)
{
    if (ctx.type == SelectionType::WorldObject)
        m_RightDock.SetActiveTab("Inspector");
    else if (ctx.type == SelectionType::Voxel)
        m_RightDock.SetActiveTab("VoxelInspector");
    // … additional types as they are implemented
}
```

---

## Undo Stack Rules

1. **One stack per editor session.** The main editor viewport, BlockAssetEditor, ShipEditor, etc. each own independent `CommandHistory` instances.
2. **Never share stacks across sessions.** Cross-session undo is undefined behavior.
3. **Max depth:** 128 commands per stack (configurable via Preferences panel).
4. **Compound commands:** Use `CommandGroup` to batch multiple atomic commands into a single undoable unit (e.g., fill operation = one group of N `VoxelPlaceCommand`s).
5. **Dirty flag:** `CommandHistory::IsModified()` returns `true` whenever the current position differs from the last-saved position. Used to drive the toolbar Save button state.

---

## Persistence Format Rules

| Rule | Rationale |
|---|---|
| Binary formats (`.nfblock`, `.nfchunk`) for runtime-loaded assets | Fast load times; `AssetDatabase` manages versioning |
| JSON formats (`*.json`) for all data-driven content (ships, missions, etc.) | Human-readable; diff-friendly in git; validated by JSON Schema |
| `.nfworld` descriptor is always JSON | Easy to inspect and hand-edit during development |
| `.nfgraph` for graph assets (NodeGraph) | Binary bytecode for runtime; source form TBD |

---

## Phase Map

```
Phase 1 (DevWorld):         World Object editing — PlaceObjectCommand, MoveObjectCommand
Phase 2 (Voxel Runtime):    Voxel + Chunk + Block Asset editing — full BlockAssetEditor
Phase 3 (NodeGraph):        Graph Node editing — GraphEditor skeleton
Phase 4 (AI / Animation):   NPC + Animation Clip editing — AnimationEditor, NPCEditor
Phase 5+ (Full Game):       Ship, Sector, Mission, Skill, Module editing
```

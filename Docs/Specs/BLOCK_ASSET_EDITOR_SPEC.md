# BLOCK_ASSET_EDITOR_SPEC.md

**Status:** Spec — pre-implementation  
**Scope:** Standalone + docked voxel block asset editor operating at 32×32×32 resolution  
**Depends on:** `NF::Game::Chunk`, `NF::Game::VoxelEditApi`, `NF::Editor::CommandHistory`, `NF::Editor::EditorViewport`

---

## 1. Purpose

Define a voxel-first block asset editor that operates at 32×32×32 resolution with full
per-voxel manipulation, material assignment, and higher-level authoring tools.  
The editor produces `.nfblock` binary asset files consumed by the world voxel runtime
and referenced by placement, PCG, and LOD pipelines.

---

## 2. Core Data Model

### 2.1 `BlockAsset` (new — `Source/Game/Voxel/BlockAsset.h`)

```
BlockAsset
├── ID            : std::string        (e.g. "core.stone_wall_1")
├── Dimensions    : {uint8_t W, H, D}  (default 32 × 32 × 32; max 64)
├── VoxelGrid     : std::array<VoxelId, W*H*D>  (dense; same layout as Chunk::m_Voxels)
├── MaterialOverrides : std::unordered_map<VoxelId, MaterialOverride>
│   └── MaterialOverride : { uint32_t albedoRgba, float roughness, float emissive }
├── Tags          : std::vector<std::string>   (e.g. "wall", "structural", "damageable")
├── LODFlags      : uint8_t  (bit 0 = generate low-poly, bit 1 = billboard, bit 2 = skip-LOD)
└── Metadata      : std::string (free-form JSON blob for game-layer properties)
```

### 2.2 `Voxel` Representation

`BlockAsset` reuses the existing `NF::Game::VoxelId` (uint8_t).  
Extended per-voxel data not covered by `VoxelId` is stored in the material override table;
the per-cell array stays byte-sized to match `Chunk` layout so the same mesher code path runs
on both.

#### Flags (stored alongside material override, not in the cell array)

| Flag | Meaning |
|------|---------|
| `Damageable` | Block health pool applies; triggers damage mesh swap |
| `Emissive` | Block contributes to light bake pass |
| `Functional` | Block hosts a game interaction slot (door hinge, switch, etc.) |
| `SealBoundary` | Block counts as hermetic seal for interior pressure volumes |

---

## 3. File Format

**Path:** `Content/Assets/Blocks/<ID>.nfblock`  
**Encoding:** Little-endian binary

```
Header (16 bytes)
  magic   : uint32  = 0x4E464246 ('NFBK')
  version : uint8   = 1
  dimW    : uint8
  dimH    : uint8
  dimD    : uint8
  flags   : uint8   (LODFlags)
  _pad    : uint8[7]

VoxelGrid (dimW * dimH * dimD bytes, one VoxelId per voxel)

MaterialOverrideCount : uint16
MaterialOverride[N]
  voxelId   : uint8
  albedoR/G/B/A : uint8[4]
  roughness : float32
  emissive  : float32

TagCount   : uint16
Tags[N]    : uint16 length + utf8 bytes

MetadataLength : uint32
MetadataBytes  : utf8 JSON
```

The serializer lives in `Source/Game/Voxel/BlockAssetSerializer.h/cpp`.  
`AssetDatabase` registers `.nfblock` as `AssetType::VoxelBlock` and invokes this serializer.

---

## 4. Editor Architecture

### 4.1 Standalone Shell

`Source/Editor/StandaloneEditors/BlockAssetEditor/BlockAssetEditorApp.h/cpp`

- Follows the same pattern as `NF::Editor::EditorApp` (create window, init `UIRenderer`, enter
  tick loop).
- Launched from: double-click `.nfblock` in `ContentBrowser`; or
  File → New → Block Asset from within the main editor.
- Communicates back to the main editor via the Live-Link layer (Phase 0.2) to notify that a
  block asset has changed so the world viewport can invalidate affected chunks.

### 4.2 Docked Panel (also available inside main editor)

`Source/Editor/Panels/BlockAssetEditorPanel.h/cpp`

Registered as a tab in the bottom dock alongside `MaterialEditorPanel`.  
When opened with no asset selected shows "Open or create a .nfblock file."

### 4.3 Panel Layout

```
┌──────────────────────────────────────────────────┐
│ Toolbar: [New] [Open] [Save] | Tool: [◉Point] [□Cube r:_] [─Line] [⬛Fill] [🎨Paint] | Symmetry: [X] [Y] [Z] │
├───────────────┬──────────────────┬───────────────┤
│ Layer View    │  3D Preview      │ Palette       │
│ (Z-slice      │  EditorViewport  │ VoxelType     │
│  slider +     │  orbit camera    │ swatches +    │
│  2D 32×32     │  same as world   │ material      │
│  grid)        │  viewport        │ overrides     │
├───────────────┴──────────────────┴───────────────┤
│ Blueprint Tab (NodeGraphPanel — procedural rules) │
└──────────────────────────────────────────────────┘
```

**Layer View** — XY cross-section at a user-controlled Z slice (0–31).  
Each cell is a colored square representing the `VoxelType`; click to paint, right-click to erase.

**3D Preview** — reuses `EditorViewport` with orbit camera.  
Calls `VoxelMesher::RebuildMesh(blockAsset.VoxelGrid)` whenever a stroke completes.  
Does NOT use the world `ChunkMap`; it meshes the isolated `BlockAsset::VoxelGrid` directly.

**Palette** — all registered `VoxelType` entries as colored labeled swatches.  
Secondary panel: material override fields for the selected voxel type (albedo, roughness, emissive toggle).

---

## 5. Tool System

### 5.1 Tool Interface

```cpp
// Source/Editor/StandaloneEditors/BlockAssetEditor/BlockTool.h
class BlockTool {
public:
    virtual ~BlockTool() = default;
    virtual void OnStrokeBegin(BlockAssetEditorState& state, int x, int y, int z) = 0;
    virtual void OnStrokeContinue(BlockAssetEditorState& state, int x, int y, int z) = 0;
    virtual void OnStrokeEnd(BlockAssetEditorState& state) = 0;
};
```

`BlockAssetEditorState` holds: active `BlockAsset*`, active `VoxelId` (paint type), `CommandHistory&`.

### 5.2 Phase 1 Tools

| Tool | Behaviour |
|------|-----------|
| `PointTool` | Place or erase one voxel per cell click/drag; one `VoxelPlaceCommand` per voxel |
| `CubeBrushTool` | Place/erase a cubic radius around the cursor; emits one `BlockBrushStrokeCommand` per stroke |
| `LineTool` | Bresenham 3D line from stroke-begin to stroke-end position; emits one command |
| `FillTool` | 3D flood-fill (6-connected) from the clicked cell; emits one `BlockFillCommand` |
| `MaterialPaintTool` | Paints `VoxelId` without changing occupied state; emits `BlockMaterialPaintCommand` |

### 5.3 Phase 2 Tools

| Tool | Behaviour |
|------|-----------|
| `BooleanUnionTool` | Voxelises a primitive shape (box/sphere) into the grid |
| `BooleanSubtractTool` | Removes all voxels inside a primitive shape |
| `SymmetryModifier` | Mirrors every stroke across selected axes before dispatch to active tool |
| `PatternBrushTool` | Stamps a sub-block (e.g. a 4×4×4 pattern) at the cursor |
| `NoiseBrushTool` | Uses `PCGWorldGen` noise to probabilistically add/remove voxels in a radius |

---

## 6. Command Types

All block asset edits are `NF::Editor::EditorCommand` subclasses stored in `CommandHistory`.

| Command Class | Execute | Undo |
|---|---|---|
| `VoxelPlaceCommand` | Sets voxel at (x,y,z) to new type | Restores previous type |
| `VoxelRemoveCommand` | Sets voxel to Air | Restores previous type |
| `BlockBrushStrokeCommand` | Batch-applies a set of (x,y,z,newId,oldId) tuples | Reverses all tuples |
| `BlockFillCommand` | Stores pre-fill snapshot + new type; applies fill | Restores snapshot |
| `BlockMaterialPaintCommand` | Updates material override entry | Restores previous override |
| `BlockResizeCommand` | Crops or expands the grid | Restores previous dimensions + data |

**Determinism rule:** Every command captures the *complete* before-state needed to reverse it at
construction time, not at execution time.  Commands are re-executable (Redo must produce
identical results).

---

## 7. Data Flow

```
Mouse/Keyboard Input
        │
        ▼
  BlockTool::OnStroke*
        │  (builds command)
        ▼
  CommandHistory::Push(cmd)
        │  (calls cmd.Execute())
        ▼
  BlockAsset::VoxelGrid (mutated)
        │
        ▼
  VoxelMesher::RebuildMesh()   ← triggered by dirty flag on stroke end
        │
        ▼
  EditorViewport (3D preview re-rendered)
        │
        ▼  (on Save)
  BlockAssetSerializer::Write()
        │
        ▼
  Content/Assets/Blocks/<ID>.nfblock
        │
        ▼  (via Live-Link)
  Main Editor world viewport invalidates affected ChunkMeshCache entries
```

---

## 8. Integration Points

| System | How It Connects |
|---|---|
| `NF::Game::VoxelMesher` | Called directly on `BlockAsset::VoxelGrid`; no `ChunkMap` involved |
| `NF::Editor::CommandHistory` | Shared instance per open asset document |
| `NF::Editor::EditorViewport` | Reused as-is; `SetSceneRendered(true)` after mesh upload |
| `NF::Editor::ContentBrowser` | Registers `.nfblock` file handler; opens editor on double-click |
| `AssetDatabase` | Registers `AssetType::VoxelBlock`; provides UUID ↔ path mapping |
| Live-Link server | Sends `AssetReloaded` packet on save so main editor + game client refresh |
| `LowPolyDerivationPipeline` | Reads `.nfblock`, outputs `.nflowpoly`; triggered on save when `LODFlags::GenerateLowPoly` is set |

---

## 9. Future Extensions (out of scope for Phase 1)

- Animation tracks on functional voxels (door open/close keyframes)
- Per-face UV override for texture atlas mapping
- Multi-asset merge: combine two `.nfblock` files with offset and blend mode
- Blueprint procedural generation nodes (NodeGraphPanel sub-tab)
- Real-time physics preview (drop-test the block using `NF::Physics`)

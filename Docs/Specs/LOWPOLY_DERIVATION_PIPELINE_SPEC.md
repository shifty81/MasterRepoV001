# Low-Poly Derivation Pipeline — Technical Specification

**Status:** Draft  
**Applies to:** NovaForge Phase 2 (Voxel Runtime) and beyond  
**Owner:** Engine / Voxel team  

---

## 1. Purpose

This document specifies the offline pipeline that reads a `.nfblock` voxel asset and produces a `.nflowpoly` low-poly mesh. The pipeline runs inside the editor on Save (when `LODFlags::GenerateLowPoly` is set) and is never executed at game runtime.

---

## 2. Trigger Conditions

The pipeline is triggered when all of the following are true at asset save time:

| Condition | Value |
|---|---|
| Asset type | `BlockAsset` (`.nfblock`) |
| LOD flag set | `LODFlags::GenerateLowPoly` |
| Asset modified since last derivation | `BlockAsset::IsDirty() == true` |

The flag is persisted in the `.nfblock` file header. Artists can toggle it per-asset in the Inspector via the **LOD Settings** section of the Block Asset Editor panel.

---

## 3. LOD Levels

Each `BlockAsset` produces three representations consumed at different distances:

| Level | Name | Format | Usage | Produced by |
|---|---|---|---|---|
| 0 | Full Voxel | `Chunk` voxel grid (live) | In-editor, in-game close range | `VoxelMesher` (real-time) |
| 1 | Low-Poly Shell | `.nflowpoly` indexed triangle mesh | In-game near distance | `LowPolyDerivationPipeline` |
| 2 | Billboard | GPU-generated quad impostor | In-game far distance | Auto-generated from LOD 1 depth pre-pass |

Distance thresholds (in world units, configurable via `Config/novaforge.project.json`):

```
lod_switch_near  = 64   // switch from Level 0 to Level 1 beyond 64 units
lod_switch_far   = 256  // switch from Level 1 to Level 2 beyond 256 units
```

---

## 4. Input Format: BlockAsset VoxelGrid

A `BlockAsset` wraps a 3-D grid of `VoxelId` values stored in a `Chunk` (32 × 32 × 32). Only voxels with `VoxelId != VoxelType::Air` participate in mesh generation.

```
.nfblock binary layout (version 1):
  [4 bytes]  magic: 0x4E464C4F ("NFLO")
  [4 bytes]  version: uint32 = 1
  [1 byte]   lod_flags: uint8 (bit 0 = GenerateLowPoly, bit 1 = GenerateBillboard)
  [4 bytes]  dim_x, dim_y, dim_z: each uint8 (padded to uint32 alignment)
  [N bytes]  voxel_data: flat row-major array of VoxelId (uint8), N = dim_x * dim_y * dim_z
  [M bytes]  material_table: array of (VoxelId → material_slot_index) entries
```

---

## 5. Pipeline Stages

The pipeline executes four sequential stages:

### 5.1 Surface Extraction (Surface Net)

**Algorithm:** Surface Nets (dual contouring variant)  
**Input:** `Chunk` voxel grid  
**Output:** Unindexed triangle soup (position + normal per vertex)

Surface Nets produce smoother quad-based meshes than Marching Cubes and are well-suited for voxel block shapes. Each cell on the boundary between a solid and an air voxel contributes one quad; the quad is split into two triangles.

**Key parameters:**
- `winding_order = CounterClockwise` (matches renderer convention)
- `smooth_normals = false` (hard-edged blocks are the design intent; enable per-asset if desired)

### 5.2 Decimation

**Algorithm:** Quadric Error Metrics (QEM) edge collapse  
**Input:** Triangle soup from Stage 1  
**Output:** Decimated indexed mesh

Target polygon budget is configurable per-asset:

| Asset size | Default target polygon count |
|---|---|
| Small (≤ 4³ voxels filled) | 64 triangles |
| Medium (≤ 8³) | 256 triangles |
| Large (≤ 32³) | 1024 triangles |

The decimation preserves hard edges (dihedral angle > 45°) and boundary edges to avoid silhouette breakup.

### 5.3 UV Unwrap

**Algorithm:** Angle-Based Flattening (ABF) → box projection fallback  
**Input:** Decimated indexed mesh  
**Output:** Mesh with UV coordinates in [0, 1] per-chart

UV charts are packed into a [0, 1] × [0, 1] atlas using a shelf-packing algorithm. Chart seams are placed along hard edges identified in Stage 5.2.

### 5.4 Material Table Mapping

Each triangle in the output mesh inherits the `VoxelId` of the voxel face it was derived from. The pipeline maps `VoxelId → material_slot_index` using the material table embedded in the `.nfblock` file, allowing the renderer to apply per-material draw calls without inspecting individual voxel IDs at runtime.

---

## 6. Output Format: `.nflowpoly`

```
.nflowpoly binary layout (version 1):
  [4 bytes]  magic: 0x4E464C50 ("NFLP")
  [4 bytes]  version: uint32 = 1
  [4 bytes]  vertex_count: uint32
  [4 bytes]  index_count: uint32
  [4 bytes]  submesh_count: uint32
  [vertex_count × 32 bytes]  vertex buffer: (pos.xyz: float3, normal.xyz: float3, uv.xy: float2)
  [index_count × 4 bytes]    index buffer: uint32
  [submesh_count × 12 bytes] submesh table: (start_index: uint32, index_count: uint32, material_slot: uint32)
```

The output file is written to the same directory as the source `.nfblock` with the same base name:
```
Content/Blocks/MyBlock.nfblock  →  Content/Blocks/MyBlock.nflowpoly
```

---

## 7. C++ Pipeline Class

```cpp
// Source/Game/Voxel/LowPolyDerivationPipeline.h

namespace NF::Game {

struct LowPolyMesh {
    struct Vertex   { float px, py, pz, nx, ny, nz, u, v; };
    struct SubMesh  { uint32_t startIndex, indexCount, materialSlot; };

    std::vector<Vertex>   vertices;
    std::vector<uint32_t> indices;
    std::vector<SubMesh>  submeshes;
};

class LowPolyDerivationPipeline {
public:
    /// @brief Run all four stages and return the output mesh.
    /// @param asset   Source block asset containing the voxel grid.
    /// @return        Derived low-poly mesh, or empty mesh on failure.
    [[nodiscard]] LowPolyMesh Derive(const BlockAsset& asset);

    /// @brief Override the triangle budget for the next Derive() call.
    void SetTargetTriangleCount(uint32_t count) { m_TargetTriCount = count; }

private:
    uint32_t m_TargetTriCount{256};

    // Internal stage helpers
    std::vector<LowPolyMesh::Vertex> ExtractSurface(const Chunk& chunk);
    std::vector<LowPolyMesh::Vertex> Decimate(
        const std::vector<LowPolyMesh::Vertex>& soup, uint32_t targetTris);
    void UnwrapUVs(std::vector<LowPolyMesh::Vertex>& mesh);
    std::vector<LowPolyMesh::SubMesh> BuildSubmeshTable(
        const std::vector<LowPolyMesh::Vertex>& mesh,
        const BlockAsset& asset);
};

} // namespace NF::Game
```

---

## 8. Integration Points

| System | Role | Contract |
|---|---|---|
| `AssetDatabase` | Registers `.nflowpoly` as a derived asset. Stores source hash to detect staleness. | `AssetDatabase::RegisterDerivedAsset(sourcePath, derivedPath, sourceHash)` |
| `VoxelMesher` | LOD 0 renderer; not used by this pipeline but shares the `Chunk` input type. | Read-only access to `Chunk::GetRawData()` |
| `BlockAssetEditor` | UI trigger: calls `LowPolyDerivationPipeline::Derive()` on Save when flag is set. | `pipeline.Derive(asset)` → `WriteNflowpoly(outputPath, mesh)` |
| `Renderer` | Consumes `.nflowpoly` at runtime for LOD 1 draw calls. | `RenderDevice::SubmitSubMesh(submesh, material)` |

---

## 9. Error Handling

| Condition | Action |
|---|---|
| Source mesh has 0 surface triangles (all-air or all-solid chunk) | Log warning and write empty `.nflowpoly`; do not treat as error |
| QEM decimation produces degenerate geometry | Abort stage 2 and use undecimated mesh; log warning |
| UV packing fails (extremely high triangle count) | Fall back to box-project UVs; log warning |
| File write fails | Log error and leave existing `.nflowpoly` unchanged |

---

## 10. Future Work (Out of Scope for Phase 2)

- GPU-accelerated Surface Nets (compute shader)
- Per-biome material override tables
- Animated low-poly meshes (blend shapes from two block states)
- Billboard generation automation (Phase 3)

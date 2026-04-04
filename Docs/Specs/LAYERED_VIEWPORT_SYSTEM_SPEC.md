# LAYERED_VIEWPORT_SYSTEM_SPEC.md

**Status:** Spec — pre-implementation  
**Scope:** Refactor `NF::Editor::EditorViewport` into a composable layered viewport reusable
across all standalone editors (BlockAssetEditor, PlanetEditor, ShipEditor, StationEditor,
GalaxyEditor, AnimationEditor, etc.)  
**Depends on:** `NF::Editor::EditorViewport`, `NF::UIRenderer`, `NF::RenderDevice`

---

## 1. Problem Statement

The current `EditorViewport` is a monolithic class that:
- Owns camera state
- Draws the scene
- Draws 2D overlays (selection labels, grid)
- Handles mouse/keyboard input for orbit

Different editors need the same orbit camera and mouse-picking primitives but radically
different scene content, overlay layers, and input mode stacks.  Duplicating `EditorViewport`
for every editor is not maintainable.

**Goal:** Split `EditorViewport` into a layered system where a host configures exactly the
layers it needs.

---

## 2. Architecture

### 2.1 Layer Concept

A **ViewportLayer** is a single-responsibility draw/update unit.  Layers are stacked in a
priority list; lower index = drawn first (back-to-front).

```cpp
// Source/Editor/Viewport/ViewportLayer.h
namespace NF::Editor {

class ViewportLayer {
public:
    virtual ~ViewportLayer() = default;

    /// @brief Called once per frame before Draw() to advance logic state.
    virtual void Update(float dt) {}

    /// @brief Issue draw calls for this layer.
    /// @param ctx  Draw context: references to UIRenderer, camera matrices, viewport bounds.
    virtual void Draw(const ViewportDrawContext& ctx) = 0;

    /// @brief Handle raw input. Return true to consume the event (stops further propagation).
    virtual bool HandleInput(const ViewportInputEvent& evt) { return false; }

    /// @brief Priority within the layer stack (lower = drawn first).
    [[nodiscard]] virtual int Priority() const noexcept { return 0; }

    /// @brief Whether this layer is currently visible.
    [[nodiscard]] virtual bool IsVisible() const noexcept { return true; }
};

} // namespace NF::Editor
```

### 2.2 `ViewportDrawContext`

```cpp
// Source/Editor/Viewport/ViewportDrawContext.h
struct ViewportDrawContext {
    UIRenderer*   ui;            ///< For 2D overlay drawing
    RenderDevice* device;        ///< For 3D draw calls
    Matrix4x4     viewMatrix;
    Matrix4x4     projMatrix;
    float         boundsX, boundsY, boundsW, boundsH;  ///< Viewport pixel bounds
    float         dt;
};
```

### 2.3 `ViewportInputEvent`

```cpp
// Source/Editor/Viewport/ViewportInputEvent.h
enum class ViewportInputEventType {
    MouseMove, MouseButtonDown, MouseButtonUp, MouseWheel,
    KeyDown, KeyUp
};

struct ViewportInputEvent {
    ViewportInputEventType type;
    float mouseX, mouseY;   ///< Viewport-local coordinates
    int   button;           ///< 0=left 1=right 2=middle
    float wheelDelta;
    int   keyCode;
    bool  ctrl, shift, alt;
};
```

### 2.4 `LayeredViewport` (replaces `EditorViewport`)

```cpp
// Source/Editor/Viewport/LayeredViewport.h
class LayeredViewport {
public:
    void Init(RenderDevice* device, UIRenderer* ui);
    void Resize(int width, int height);
    void Update(float dt);
    void Draw(float x, float y, float w, float h);

    /// @brief Add a layer. Layers are sorted by Priority() automatically.
    void AddLayer(std::shared_ptr<ViewportLayer> layer);

    /// @brief Remove a layer by pointer identity.
    void RemoveLayer(ViewportLayer* layer);

    // Camera passthrough (delegates to internal CameraOrbitLayer)
    void SetCameraTarget(const Vector3& t) noexcept;
    void SetCameraZoom(float z) noexcept;
    [[nodiscard]] Matrix4x4 GetViewMatrix() const noexcept;
    [[nodiscard]] Matrix4x4 GetProjectionMatrix() const noexcept;
    [[nodiscard]] bool PickRay(float mx, float my,
                               Vector3& outOrigin, Vector3& outDir) const noexcept;
    [[nodiscard]] bool IsMouseInside() const noexcept;

private:
    std::vector<std::shared_ptr<ViewportLayer>> m_Layers;
    // Internal camera state (moved here from EditorViewport)
    float m_Pitch{0.6f}, m_Yaw{0.f}, m_Zoom{100.f};
    Vector3 m_Target{};
    int m_Width{1280}, m_Height{720};
    float m_BoundsX{}, m_BoundsY{}, m_BoundsW{}, m_BoundsH{};
    RenderDevice* m_Device{nullptr};
    UIRenderer*   m_UI{nullptr};
};
```

### 2.5 Migration Path for `EditorViewport`

`EditorViewport` is **not deleted**; it becomes a thin wrapper around `LayeredViewport` that
pre-constructs the standard layer stack used by the main editor.  All existing call-sites in
`EditorApp` continue to compile unchanged.

```cpp
class EditorViewport {
public:
    void Init(RenderDevice* device);
    // ... existing public API unchanged ...
private:
    LayeredViewport m_Viewport;
    // Standard layers constructed in Init():
    //   WorldVoxelLayer       (priority  0)
    //   GridOverlayLayer      (priority 10)
    //   SelectionHighlightLayer (priority 20)
    //   GizmoLayer            (priority 30)
    //   DebugOverlayLayer     (priority 40)
};
```

---

## 3. Standard Layer Catalogue

These layers ship with `NF::Editor` and are reused across all editors.

| Layer Class | Priority | Description |
|---|---|---|
| `CameraOrbitLayer` | — | Handles mouse orbit/pan/zoom input; not a draw layer, input-only |
| `WorldVoxelLayer` | 0 | Submits `ChunkMap` draw calls to `RenderDevice` |
| `BlockAssetPreviewLayer` | 0 | Submits a single `BlockAsset` mesh; used by `BlockAssetEditorPanel` |
| `PlanetSurfaceLayer` | 0 | Renders a sphere mesh with procedural surface; used by `PlanetEditor` |
| `GalaxyStarFieldLayer` | 0 | Top-down 2D point-cloud of star positions; used by `GalaxyEditor` |
| `ShipVoxelLayer` | 0 | Renders a `ShipDefinition` voxel grid; used by `ShipEditor` |
| `InteriorVoxelLayer` | 0 | Renders a room-scale interior voxel grid |
| `GridOverlayLayer` | 10 | Fallback grid lines drawn with `UIRenderer`; on/off toggle |
| `AxisGizmoLayer` | 15 | World-space XYZ axis indicator in corner |
| `SelectionHighlightLayer` | 20 | Highlights selected chunk/entity/voxel; driven by `SelectionService` |
| `VoxelSliceLayer` | 25 | Draws 2D cross-section slice used in `BlockAssetEditorPanel` layer view |
| `HardpointMarkerLayer` | 25 | Shows hardpoint placement handles; used by `ShipEditor` |
| `OrbitMarkerLayer` | 25 | Shows orbit rings for solar system bodies; used by `SolarSystemEditor` |
| `GizmoLayer` | 30 | `TransformGizmo` (translate/rotate/scale handles) |
| `DebugOverlayLayer` | 40 | Frame stats, picking debug info |
| `BlueprintPreviewLayer` | 45 | Shows node graph evaluation overlays |

---

## 4. Per-Editor Layer Stacks

Each standalone editor constructs its `LayeredViewport` with only the layers it needs.

### 4.1 Main Editor Viewport (current `EditorViewport`)

```
WorldVoxelLayer (0)
GridOverlayLayer (10)
AxisGizmoLayer (15)
SelectionHighlightLayer (20)
GizmoLayer (30)
DebugOverlayLayer (40)
```

### 4.2 Block Asset Editor (3D Preview pane)

```
BlockAssetPreviewLayer (0)
GridOverlayLayer (10)
AxisGizmoLayer (15)
VoxelSliceLayer (25)   ← slice plane indicator
DebugOverlayLayer (40)
```

### 4.3 Galaxy Editor

```
GalaxyStarFieldLayer (0)
OrbitMarkerLayer (25)    ← jump lane indicators
SelectionHighlightLayer (20)
DebugOverlayLayer (40)
```

### 4.4 Solar System Editor

```
SkyboxLayer (0)          ← dark space background
OrbitMarkerLayer (25)    ← orbit ellipses for each body
SelectionHighlightLayer (20)
AxisGizmoLayer (15)
GizmoLayer (30)          ← for dragging orbit radius
DebugOverlayLayer (40)
```

### 4.5 Planet Editor

```
PlanetSurfaceLayer (0)
AtmosphereLayer (5)      ← alpha blended atmosphere sphere
GridOverlayLayer (10)    ← lat/lon grid
BiomePaintOverlayLayer (25) ← shows painted biome regions
DebugOverlayLayer (40)
```

### 4.6 Ship Editor

```
ShipVoxelLayer (0)
GridOverlayLayer (10)
AxisGizmoLayer (15)
HardpointMarkerLayer (25)
GizmoLayer (30)
DebugOverlayLayer (40)
```

### 4.7 Animation Editor

```
CharacterMeshLayer (0)
SkeletonLayer (10)       ← bone hierarchy overlay
GridOverlayLayer (15)
BoneGizmoLayer (30)      ← bone rotation handles
DebugOverlayLayer (40)
```

---

## 5. Input Routing

Input events flow top-to-bottom through the layer stack (highest priority first).
A layer returns `true` from `HandleInput()` to consume the event.

Standard routing order for all editors:
1. `GizmoLayer` / `BoneGizmoLayer` — tool handles take priority
2. Tool-specific layer (e.g. `VoxelSliceLayer` for slice drag)
3. `CameraOrbitLayer` — orbit/pan/zoom if no tool consumed the event
4. All other layers — observation only, must return `false`

---

## 6. File / Module Layout

```
Source/Editor/Viewport/
  ViewportLayer.h              (new)
  ViewportDrawContext.h        (new)
  ViewportInputEvent.h         (new)
  LayeredViewport.h/cpp        (new)
  EditorViewport.h/cpp         (modified — wraps LayeredViewport)
  TransformGizmo.h/cpp         (unchanged)
  Layers/
    WorldVoxelLayer.h/cpp      (extracted from EditorViewport::Draw)
    GridOverlayLayer.h/cpp     (extracted from EditorViewport::Draw)
    SelectionHighlightLayer.h/cpp
    GizmoLayer.h/cpp           (wraps TransformGizmo)
    DebugOverlayLayer.h/cpp
    BlockAssetPreviewLayer.h/cpp  (new — for BlockAssetEditor)
    OrbitMarkerLayer.h/cpp        (new — for SolarSystem/GalaxyEditor)
    PlanetSurfaceLayer.h/cpp      (new — for PlanetEditor)
    ShipVoxelLayer.h/cpp          (new — for ShipEditor)
    HardpointMarkerLayer.h/cpp    (new — for ShipEditor)
    SkeletonLayer.h/cpp           (new — for AnimationEditor)
    BiomePaintOverlayLayer.h/cpp  (new — for PlanetEditor)
```

All new files are added to `Source/Editor/CMakeLists.txt`.

---

## 7. Compatibility Guarantee

- `EditorApp` call-sites: **zero changes required**.  `EditorViewport` public API is preserved.
- `TransformGizmo` wiring: `GizmoLayer` calls `TransformGizmo::SetCameraMatrices()` and
  `SetViewportBounds()` internally; `EditorApp` continues calling `m_Gizmo.SetCameraMatrices()`
  via `EditorViewport::GetViewMatrix()` / `GetProjectionMatrix()` which delegate to `LayeredViewport`.
- `EditorApp::TickFrame()` references to `m_Viewport.SetHighlightState()`,
  `m_Viewport.SetShowGrid()`, `m_Viewport.Draw()` all compile unchanged.

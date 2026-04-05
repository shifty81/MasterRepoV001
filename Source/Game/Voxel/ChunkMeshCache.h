#pragma once
#include "Renderer/Mesh/MeshRenderer.h"
#include "Renderer/RHI/Shader.h"
#include "Renderer/Materials/Material.h"
#include "Renderer/Pipeline/ForwardRenderer.h"
#include "Game/Voxel/ChunkMap.h"
#include "Game/Voxel/VoxelMesher.h"
#include "Core/Math/Matrix.h"
#include "Core/Math/Vector.h"
#include <array>
#include <unordered_map>
#include <memory>

namespace NF::Game {

/// @brief Maintains a GPU @c Mesh cache for every loaded chunk.
///
/// On each frame call @c RebuildDirty() to re-mesh any chunks whose
/// voxel data has changed.  Then call @c Render() to submit all
/// cached meshes to the @c ForwardRenderer.
///
/// The cache is keyed by @c ChunkCoord and keeps meshes in sync with
/// the @c ChunkMap that owns the voxel data.
class ChunkMeshCache {
public:
    ChunkMeshCache() = default;

    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------

    /// @brief Initialise the voxel shader and material.
    ///
    /// Must be called after an OpenGL context is current.
    /// @param forwardRenderer  Pointer to the forward renderer used
    ///                         for submitting draw calls.
    void Init(ForwardRenderer* forwardRenderer);

    /// @brief Release all GPU resources.
    void Shutdown();

    // -------------------------------------------------------------------------
    // Per-frame operations
    // -------------------------------------------------------------------------

    /// @brief Re-mesh any dirty chunks and upload to the GPU.
    ///
    /// Iterates the chunk map, finds dirty chunks, generates new mesh
    /// data via @c VoxelMesher, and uploads to the GPU.
    void RebuildDirty(ChunkMap& map);

    /// @brief Submit all cached chunk meshes to the renderer.
    ///
    /// Call between @c ForwardRenderer::BeginScene() and @c EndScene().
    void Render();

    /// @brief Update the camera position used for specular lighting.
    ///
    /// Call once per frame before @c Render() with the current camera eye.
    void SetCameraPosition(const Vector3& pos) noexcept;

    /// @brief Remove the cached mesh for a chunk that has been unloaded.
    void Evict(const ChunkCoord& coord);

    /// @brief Remove all cached meshes.
    void Clear();

    // -------------------------------------------------------------------------
    // Queries
    // -------------------------------------------------------------------------

    /// @brief Number of chunk meshes currently cached on the GPU.
    [[nodiscard]] int CachedCount() const noexcept {
        return static_cast<int>(m_Meshes.size());
    }

    /// @brief Total triangle count across all cached meshes.
    [[nodiscard]] int TotalTriangles() const noexcept;

    // -------------------------------------------------------------------------
    // Frustum culling
    // -------------------------------------------------------------------------

    /// @brief Supply the combined view-projection matrix used for frustum culling.
    ///
    /// Must be called once per frame (before @c Render()) whenever the camera
    /// moves.  Extracts the 6 world-space frustum planes via the Gribb-Hartmann
    /// method so that @c Render() can skip chunks outside the camera frustum.
    ///
    /// @param viewProj  The combined View × Projection matrix
    ///                  (typically @c proj * view in column-major convention).
    void SetViewProjection(const Matrix4x4& viewProj) noexcept;

private:
    // -------------------------------------------------------------------------
    // Frustum plane storage
    // -------------------------------------------------------------------------

    /// @brief One clip-space half-plane: (nx, ny, nz) = normal, W = distance.
    ///
    /// A world-space point P is on the positive (inside) side when
    /// nx*P.x + ny*P.y + nz*P.z + W ≥ 0.
    using FrustumPlane = Vector4;

    /// @brief 6 frustum planes extracted from the view-projection matrix.
    ///        Initialised to an all-pass state (every chunk visible) until
    ///        @c SetViewProjection() is called.
    std::array<FrustumPlane, 6> m_FrustumPlanes{
        FrustumPlane{ 0,0,1, 1e6f},  // left   — pass-all sentinel
        FrustumPlane{ 0,0,1, 1e6f},  // right
        FrustumPlane{ 0,0,1, 1e6f},  // bottom
        FrustumPlane{ 0,0,1, 1e6f},  // top
        FrustumPlane{ 0,0,1, 1e6f},  // near
        FrustumPlane{ 0,0,1, 1e6f},  // far
    };

    /// @brief Test whether the axis-aligned box for @p coord intersects the frustum.
    ///
    /// Uses the positive-vertex test: for each plane, compute the corner of the
    /// AABB that is furthest in the plane-normal direction.  If that corner is
    /// on the negative side (outside), the whole box is outside and the chunk
    /// can be skipped.
    ///
    /// @return true if the chunk may be visible (should be rendered).
    [[nodiscard]] bool IsChunkVisible(const ChunkCoord& coord) const noexcept;

    struct CachedMesh {
        std::unique_ptr<Mesh> GpuMesh;
        int TriCount{0};
    };

    std::unordered_map<ChunkCoord, CachedMesh> m_Meshes;

    VoxelMesher        m_Mesher;
    ForwardRenderer*   m_Renderer{nullptr};
    Shader             m_Shader;
    Material           m_Material;
};

} // namespace NF::Game

#include "Game/Voxel/ChunkMeshCache.h"
#include "Core/Logging/Log.h"
#include <string>

namespace NF::Game {

// ---------------------------------------------------------------------------
// Voxel GLSL shaders — directional lighting with per-vertex colour
// ---------------------------------------------------------------------------

static const char* kVoxelVertSrc = R"(
#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

uniform mat4 uView;
uniform mat4 uProjection;
uniform mat4 uModel;

out vec3 vNormal;
out float vTypeId;

void main() {
    gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);
    vNormal     = aNormal;
    vTypeId     = aTexCoord.x;
}
)";

static const char* kVoxelFragSrc = R"(
#version 330 core
in vec3 vNormal;
in float vTypeId;

out vec4 FragColor;

uniform vec3 uLightDir;       // normalised world-space direction toward light
uniform vec3 uLightColor;     // directional light colour
uniform vec3 uAmbientColor;   // ambient fill colour

// Simple Phong-ish lighting palette for voxel types 0..7.
vec3 getPalette(int id) {
    // Air=0, Stone=1, Ore=2, Dirt=3, Rock=4, Metal=5, Ice=6, Organic=7
    if (id == 1) return vec3(0.55, 0.55, 0.55);  // Stone   — grey
    if (id == 2) return vec3(0.85, 0.65, 0.25);  // Ore     — gold
    if (id == 3) return vec3(0.45, 0.30, 0.15);  // Dirt    — brown
    if (id == 4) return vec3(0.40, 0.40, 0.42);  // Rock    — dark grey
    if (id == 5) return vec3(0.70, 0.72, 0.75);  // Metal   — silver
    if (id == 6) return vec3(0.70, 0.85, 0.95);  // Ice     — light blue
    if (id == 7) return vec3(0.30, 0.55, 0.20);  // Organic — green
    return vec3(1.0, 0.0, 1.0);                   // Unknown — magenta
}

void main() {
    int typeId = int(vTypeId + 0.5);  // round to nearest integer
    vec3 baseColor = getPalette(typeId);

    vec3 normal = normalize(vNormal);

    // Per-face baked brightness — Minecraft-style directional shading so that
    // every face is distinguishable regardless of the dynamic light direction.
    //   Top    (+Y): 1.00  — fully lit
    //   Sides  (±X, ±Z): 0.80  — slightly dimmer
    //   Bottom (-Y): 0.50  — clearly shadowed
    float faceBrightness;
    if (normal.y > 0.5)       faceBrightness = 1.00;
    else if (normal.y < -0.5) faceBrightness = 0.50;
    else                       faceBrightness = 0.80;

    // Diffuse (Lambert) — view-independent, depends only on face normal and
    // fixed light direction so appearance stays constant as the camera orbits.
    float diff = max(dot(normal, uLightDir), 0.0);
    vec3 diffuse = diff * uLightColor;

    vec3 color = faceBrightness * (uAmbientColor + diffuse) * baseColor;

    FragColor = vec4(color, 1.0);
}
)";

// ---------------------------------------------------------------------------
// Init / Shutdown
// ---------------------------------------------------------------------------

void ChunkMeshCache::Init(ForwardRenderer* forwardRenderer)
{
    m_Renderer = forwardRenderer;

    // Compile the voxel shader.
    if (m_Shader.LoadFromSource(kVoxelVertSrc, kVoxelFragSrc))
        NF::Logger::Log(NF::LogLevel::Info, "ChunkMeshCache", "Voxel shader compiled");
    else
        NF::Logger::Log(NF::LogLevel::Warning, "ChunkMeshCache", "Voxel shader compile failed (may be stub renderer)");

    // Set up the material with default lighting values.
    m_Material.SetShader(&m_Shader);
    m_Material.SetVec3("uLightDir",     NF::Vector3{0.4f, 0.8f, 0.3f}.Normalized());  // sun from upper-right
    m_Material.SetVec3("uLightColor",   {1.0f, 0.95f, 0.85f});
    m_Material.SetVec3("uAmbientColor", {0.30f, 0.32f, 0.35f});

    NF::Logger::Log(NF::LogLevel::Info, "ChunkMeshCache", "Initialised");
}

void ChunkMeshCache::Shutdown()
{
    m_Meshes.clear();
    NF::Logger::Log(NF::LogLevel::Info, "ChunkMeshCache", "Shut down");
}

// ---------------------------------------------------------------------------
// Per-frame
// ---------------------------------------------------------------------------

void ChunkMeshCache::RebuildDirty(ChunkMap& map)
{
    auto dirtyChunks = map.GetMeshDirtyChunks();
    if (dirtyChunks.empty()) return;

    for (Chunk* chunk : dirtyChunks) {
        if (!chunk) continue;

        const ChunkCoord coord = chunk->GetCoord();

        // Fetch the 6 face-adjacent neighbour chunks so the mesher can cull
        // cross-chunk boundary faces correctly.  Null means open air on that
        // side, which is correct for unloaded / non-existent neighbours.
        const Chunk* neighbours[6] = {
            map.GetChunk({coord.X - 1, coord.Y,     coord.Z    }),  // -X
            map.GetChunk({coord.X + 1, coord.Y,     coord.Z    }),  // +X
            map.GetChunk({coord.X,     coord.Y - 1, coord.Z    }),  // -Y
            map.GetChunk({coord.X,     coord.Y + 1, coord.Z    }),  // +Y
            map.GetChunk({coord.X,     coord.Y,     coord.Z - 1}),  // -Z
            map.GetChunk({coord.X,     coord.Y,     coord.Z + 1}),  // +Z
        };

        // Generate mesh data on the CPU.
        MeshData meshData = m_Mesher.Generate(*chunk, neighbours);
        chunk->ClearMeshDirty();

        if (meshData.Indices.empty()) {
            // No visible geometry — evict from cache.
            m_Meshes.erase(coord);
            continue;
        }

        // Upload to GPU.
        auto& cached = m_Meshes[coord];
        if (!cached.GpuMesh)
            cached.GpuMesh = std::make_unique<Mesh>();
        cached.GpuMesh->Upload(meshData);
        cached.TriCount = static_cast<int>(meshData.Indices.size()) / 3;
    }

    NF::Logger::Log(NF::LogLevel::Debug, "ChunkMeshCache",
        "Rebuilt " + std::to_string(dirtyChunks.size()) + " chunk mesh(es)");
}

void ChunkMeshCache::Render()
{
    if (!m_Renderer) return;

    for (auto& [coord, cached] : m_Meshes) {
        if (!cached.GpuMesh) continue;

        // Skip chunks that are entirely outside the camera frustum.
        if (!IsChunkVisible(coord)) continue;

        // Chunk meshes are already in world space (origin baked into vertices).
        m_Renderer->Submit(*cached.GpuMesh, m_Material, Matrix4x4::Identity());
    }
}

void ChunkMeshCache::SetCameraPosition(const Vector3& pos) noexcept
{
    m_Material.SetVec3("uViewPos", pos);
}

void ChunkMeshCache::Evict(const ChunkCoord& coord)
{
    m_Meshes.erase(coord);
}

void ChunkMeshCache::Clear()
{
    m_Meshes.clear();
}

int ChunkMeshCache::TotalTriangles() const noexcept
{
    int total = 0;
    for (const auto& [_, cached] : m_Meshes)
        total += cached.TriCount;
    return total;
}

// ---------------------------------------------------------------------------
// Frustum culling
// ---------------------------------------------------------------------------

// Extracts the 6 world-space frustum planes from the combined view-projection
// matrix using the Gribb-Hartmann method.
//
// Our Matrix4x4 is column-major: M.M[col][row].
// The i-th row of the matrix is therefore: {M[0][i], M[1][i], M[2][i], M[3][i]}.
//
// A clip-space point P_clip = VP * P_world lies inside the frustum iff:
//   -w <= x <= w,  -w <= y <= w,  -w <= z <= w  (OpenGL NDC convention)
//
// Each inequality can be expressed as a linear constraint on P_world:
//   Left:    row3 + row0 (dot with P_world, in homogeneous form) >= 0
//   Right:   row3 - row0 >= 0
//   Bottom:  row3 + row1 >= 0
//   Top:     row3 - row1 >= 0
//   Near:    row3 + row2 >= 0
//   Far:     row3 - row2 >= 0
//
// The resulting plane for each side is FrustumPlane{nx, ny, nz, d} such that
// a point P is inside if  nx*P.x + ny*P.y + nz*P.z + d >= 0.
void ChunkMeshCache::SetViewProjection(const Matrix4x4& vp) noexcept
{
    // Helper: extract a plane from two rows (op = +1 or -1).
    auto extractPlane = [&](int row, float op) -> FrustumPlane {
        // row3 +/- row_i, in column-major storage: M.M[col][row_index]
        return FrustumPlane{
            vp.M[0][3] + op * vp.M[0][row],  // nx
            vp.M[1][3] + op * vp.M[1][row],  // ny
            vp.M[2][3] + op * vp.M[2][row],  // nz
            vp.M[3][3] + op * vp.M[3][row]   // d
        };
    };

    m_FrustumPlanes[0] = extractPlane(0, +1.f);  // Left
    m_FrustumPlanes[1] = extractPlane(0, -1.f);  // Right
    m_FrustumPlanes[2] = extractPlane(1, +1.f);  // Bottom
    m_FrustumPlanes[3] = extractPlane(1, -1.f);  // Top
    m_FrustumPlanes[4] = extractPlane(2, +1.f);  // Near
    m_FrustumPlanes[5] = extractPlane(2, -1.f);  // Far
}

// Tests the chunk AABB against the 6 frustum planes using the positive-vertex
// (p-vertex) method.  For each plane, the p-vertex is the AABB corner that is
// furthest in the direction of the plane normal.  If the p-vertex is on the
// outside of any plane, the entire box is outside and the chunk is culled.
bool ChunkMeshCache::IsChunkVisible(const ChunkCoord& coord) const noexcept
{
    // Compute the chunk AABB in world space.
    int32_t ox, oy, oz;
    ChunkOrigin(coord, ox, oy, oz);

    const float half    = static_cast<float>(kChunkSize) * 0.5f; // e.g. 16.0 when kChunkSize=32
    const float centerX = static_cast<float>(ox) + half;
    const float centerY = static_cast<float>(oy) + half;
    const float centerZ = static_cast<float>(oz) + half;

    for (const auto& plane : m_FrustumPlanes) {
        // P-vertex: the AABB corner furthest along the plane normal.
        const float pvx = centerX + (plane.X >= 0.f ? half : -half);
        const float pvy = centerY + (plane.Y >= 0.f ? half : -half);
        const float pvz = centerZ + (plane.Z >= 0.f ? half : -half);

        // If the p-vertex is outside this plane, the entire chunk is invisible.
        if (plane.X * pvx + plane.Y * pvy + plane.Z * pvz + plane.W < 0.f)
            return false;
    }
    return true;
}

} // namespace NF::Game

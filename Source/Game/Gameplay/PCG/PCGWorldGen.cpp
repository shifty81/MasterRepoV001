// PCGWorldGen.cpp — Seed-based procedural terrain generator.
//
// Port of the WorldGenerator sine/cosine noise logic from tempnovaforge,
// adapted to MasterRepo's Chunk / VoxelType / kChunkSize API.
#include "Game/Gameplay/PCG/PCGWorldGen.h"
#include <cmath>

namespace NF::Game::Gameplay {

// ---------------------------------------------------------------------------
// SurfaceHeight — noise function
// ---------------------------------------------------------------------------
//
// Layered sine/cosine noise that produces natural terrain variation.
// All inputs are world-space voxel coordinates.  The seed is mixed in
// via prime-multiplied offsets so different seeds produce different terrain.
//
// Returns the world-space Y voxel index of the top solid voxel at (wx, wz).

int32_t PCGWorldGen::SurfaceHeight(int32_t wx, int32_t wz) const noexcept
{
    // Scale world coords to a small frequency for low-frequency terrain waves.
    constexpr float kBaseFreq   = 0.03f;
    constexpr float kDetailFreq = 0.11f;
    constexpr float kMicroFreq  = 0.27f;

    // Seed offset to differentiate worlds.
    const float seedX = static_cast<float>(m_Seed * 2654435761u % 1000000u) * 0.001f;
    const float seedZ = static_cast<float>(m_Seed * 2246822519u % 1000000u) * 0.001f;

    const float fx = static_cast<float>(wx) + seedX;
    const float fz = static_cast<float>(wz) + seedZ;

    // Three octaves of sine/cosine noise.
    float h = 0.f;
    h += std::sin(fx * kBaseFreq)   * std::cos(fz * kBaseFreq)   * 12.f;
    h += std::sin(fx * kDetailFreq) * std::cos(fz * kDetailFreq) *  5.f;
    h += std::cos(fx * kMicroFreq)  * std::sin(fz * kMicroFreq)  *  2.f;

    // Base terrain sits at Y = 16 (mid-chunk).
    return static_cast<int32_t>(h + 16.f);
}

// ---------------------------------------------------------------------------
// GenerateChunk
// ---------------------------------------------------------------------------

void PCGWorldGen::GenerateChunk(Chunk& chunk) const
{
    const ChunkCoord& coord = chunk.GetCoord();

    // World-space origin of this chunk.
    int32_t originX, originY, originZ;
    ChunkOrigin(coord, originX, originY, originZ);

    for (uint8_t lx = 0; lx < static_cast<uint8_t>(kChunkSize); ++lx) {
        for (uint8_t lz = 0; lz < static_cast<uint8_t>(kChunkSize); ++lz) {
            const int32_t wx = originX + static_cast<int32_t>(lx);
            const int32_t wz = originZ + static_cast<int32_t>(lz);

            const int32_t surfaceY = SurfaceHeight(wx, wz);

            for (uint8_t ly = 0; ly < static_cast<uint8_t>(kChunkSize); ++ly) {
                const int32_t wy = originY + static_cast<int32_t>(ly);

                VoxelId voxel = static_cast<VoxelId>(VoxelType::Air);

                if (wy < surfaceY - (m_DirtThickness + m_StoneDepth)) {
                    // Deep underground — Stone.
                    voxel = static_cast<VoxelId>(VoxelType::Stone);
                } else if (wy < surfaceY - m_DirtThickness) {
                    // Shallow underground — Dirt.
                    voxel = static_cast<VoxelId>(VoxelType::Dirt);
                } else if (wy < surfaceY) {
                    // Surface layer — Organic (replaces Grass, which does not
                    // exist in the MasterRepo VoxelType enum).
                    voxel = static_cast<VoxelId>(VoxelType::Organic);
                }
                // wy >= surfaceY — Air (already set above)

                chunk.SetVoxel(lx, ly, lz, voxel);
            }
        }
    }

    // Clear dirty flags — generation is not an "edit", so no mesh rebuild
    // should be triggered by the act of generating the chunk itself.
    chunk.ClearDirty();
}

} // namespace NF::Game::Gameplay


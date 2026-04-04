#pragma once
// PCGWorldGen.h — Seed-based procedural terrain generator.
//
// Generates NF::Game::Chunk data from a world seed using layered
// sine/cosine noise.  Assigns Stone, Dirt, and Organic voxel types
// by world-space Y height.
//
// Phase 5 implementation: ports the WorldGenerator logic from
// tempnovaforge while adapting to MasterRepo's Chunk / VoxelType API.

#include "Game/Voxel/Chunk.h"
#include "Game/Voxel/ChunkCoord.h"
#include <memory>

namespace NF::Game::Gameplay {

/// @brief Procedural chunk terrain generator.
///
/// ## Usage
/// ```cpp
/// PCGWorldGen gen;
/// gen.SetSeed(42);
/// Chunk chunk({0, 0, 0});
/// gen.GenerateChunk(chunk);
/// ```
class PCGWorldGen {
public:
    PCGWorldGen() = default;

    // ---- Configuration -------------------------------------------------------

    /// @brief Set the world generation seed.  Must be called before GenerateChunk.
    void SetSeed(uint32_t seed) noexcept { m_Seed = seed; }

    /// @brief Return the current seed.
    [[nodiscard]] uint32_t GetSeed() const noexcept { return m_Seed; }

    /// @brief Surface height at Y=0 is treated as world sea level.
    ///        Voxels below this depth become Stone.
    void SetStoneDepth(int32_t depth) noexcept { m_StoneDepth = depth; }

    /// @brief Thickness of the Dirt layer above the Stone layer (in voxels).
    void SetDirtThickness(int32_t thickness) noexcept { m_DirtThickness = thickness; }

    // ---- Generation ----------------------------------------------------------

    /// @brief Fill @p chunk with procedurally generated voxels.
    ///
    /// Uses the chunk's coordinate to derive world-space positions so that
    /// adjacent chunks tile seamlessly.
    void GenerateChunk(Chunk& chunk) const;

private:
    uint32_t m_Seed{12345};
    int32_t  m_StoneDepth{4};       ///< Voxels below surface-2 become Stone
    int32_t  m_DirtThickness{2};    ///< Voxels at surface-1 and surface become Dirt

    /// @brief Compute the surface height (Y) at world-space (wx, wz).
    ///        Returns a value in voxel space.
    [[nodiscard]] int32_t SurfaceHeight(int32_t wx, int32_t wz) const noexcept;
};

} // namespace NF::Game::Gameplay

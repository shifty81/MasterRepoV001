/// @file PCGWorldGenTests.cpp — Unit tests for PCGWorldGen terrain generator.
#include <catch2/catch_test_macros.hpp>
#include "Game/Gameplay/PCG/PCGWorldGen.h"
#include "Game/Voxel/Chunk.h"
#include "Game/Voxel/ChunkCoord.h"
#include "Game/Voxel/VoxelType.h"

using namespace NF::Game;
using namespace NF::Game::Gameplay;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Fill a chunk at the given coord using a seeded generator.
static Chunk GenerateAt(uint32_t seed, ChunkCoord coord,
                        int32_t stoneDepth = 4, int32_t dirtThickness = 2)
{
    PCGWorldGen gen;
    gen.SetSeed(seed);
    gen.SetStoneDepth(stoneDepth);
    gen.SetDirtThickness(dirtThickness);
    Chunk chunk{coord};
    gen.GenerateChunk(chunk);
    return chunk;
}

// ---------------------------------------------------------------------------
// Basic API
// ---------------------------------------------------------------------------

TEST_CASE("PCGWorldGen: default seed is non-zero", "[pcg]")
{
    PCGWorldGen gen;
    REQUIRE(gen.GetSeed() != 0);
}

TEST_CASE("PCGWorldGen: SetSeed round-trips", "[pcg]")
{
    PCGWorldGen gen;
    gen.SetSeed(99999);
    REQUIRE(gen.GetSeed() == 99999);
}

// ---------------------------------------------------------------------------
// Determinism
// ---------------------------------------------------------------------------

TEST_CASE("PCGWorldGen: same seed produces identical chunks", "[pcg]")
{
    ChunkCoord coord{0, 0, 0};
    Chunk a = GenerateAt(42, coord);
    Chunk b = GenerateAt(42, coord);

    REQUIRE(a.GetRawData() == b.GetRawData());
}

TEST_CASE("PCGWorldGen: different seeds produce different chunks", "[pcg]")
{
    ChunkCoord coord{0, 0, 0};
    Chunk a = GenerateAt(1,  coord);
    Chunk b = GenerateAt(99, coord);

    // With overwhelming probability the terrain heights differ.
    REQUIRE(a.GetRawData() != b.GetRawData());
}

// ---------------------------------------------------------------------------
// Dirty-flag behaviour
// ---------------------------------------------------------------------------

TEST_CASE("PCGWorldGen: chunk is clean after generation", "[pcg]")
{
    Chunk chunk{{0, 0, 0}};
    PCGWorldGen gen;
    gen.SetSeed(1);
    gen.GenerateChunk(chunk);

    REQUIRE(!chunk.IsMeshDirty());
    REQUIRE(!chunk.IsCollisionDirty());
}

// ---------------------------------------------------------------------------
// Voxel layer correctness
// ---------------------------------------------------------------------------

TEST_CASE("PCGWorldGen: top Y layer is not all Stone", "[pcg]")
{
    // At chunk Y=0 the surface sits around Y=16, so the top row (Y=31)
    // should be Air for at least some column.
    Chunk chunk{{0, 0, 0}};
    PCGWorldGen gen;
    gen.SetSeed(1);
    gen.GenerateChunk(chunk);

    bool anyAir = false;
    for (uint8_t x = 0; x < kChunkSize; ++x)
        for (uint8_t z = 0; z < kChunkSize; ++z)
            if (chunk.GetVoxel(x, static_cast<uint8_t>(kChunkSize - 1), z)
                    == static_cast<VoxelId>(VoxelType::Air))
                anyAir = true;

    REQUIRE(anyAir);
}

TEST_CASE("PCGWorldGen: bottom Y layer is not all Air", "[pcg]")
{
    // The very bottom of chunk Y=0 (wy=0) should be below the surface,
    // so at least some voxels there should be Stone or Dirt.
    Chunk chunk{{0, 0, 0}};
    PCGWorldGen gen;
    gen.SetSeed(1);
    gen.GenerateChunk(chunk);

    bool anySolid = false;
    for (uint8_t x = 0; x < kChunkSize; ++x)
        for (uint8_t z = 0; z < kChunkSize; ++z)
            if (chunk.GetVoxel(x, 0, z) != static_cast<VoxelId>(VoxelType::Air))
                anySolid = true;

    REQUIRE(anySolid);
}

TEST_CASE("PCGWorldGen: only recognised VoxelType values are produced", "[pcg]")
{
    Chunk chunk{{0, 0, 0}};
    PCGWorldGen gen;
    gen.SetSeed(1234);
    gen.GenerateChunk(chunk);

    const VoxelId maxKnown = static_cast<VoxelId>(VoxelType::Organic);
    for (const VoxelId id : chunk.GetRawData()) {
        // Generated voxels must be Air, Stone, Dirt, or Organic.
        bool valid = (id == static_cast<VoxelId>(VoxelType::Air)    ||
                      id == static_cast<VoxelId>(VoxelType::Stone)  ||
                      id == static_cast<VoxelId>(VoxelType::Dirt)   ||
                      id == static_cast<VoxelId>(VoxelType::Organic));
        REQUIRE(valid);
    }
    (void)maxKnown;
}

// ---------------------------------------------------------------------------
// Layer parameters
// ---------------------------------------------------------------------------

TEST_CASE("PCGWorldGen: increasing StoneDepth produces more Stone", "[pcg]")
{
    ChunkCoord coord{0, 0, 0};
    Chunk shallowStone = GenerateAt(7, coord, 1, 2);
    Chunk deepStone    = GenerateAt(7, coord, 10, 2);

    int countShallow = 0, countDeep = 0;
    for (VoxelId id : shallowStone.GetRawData())
        if (id == static_cast<VoxelId>(VoxelType::Stone)) ++countShallow;
    for (VoxelId id : deepStone.GetRawData())
        if (id == static_cast<VoxelId>(VoxelType::Stone)) ++countDeep;

    REQUIRE(countDeep >= countShallow);
}

TEST_CASE("PCGWorldGen: increasing DirtThickness produces more Dirt", "[pcg]")
{
    ChunkCoord coord{0, 0, 0};
    Chunk thinDirt  = GenerateAt(7, coord, 4, 1);
    Chunk thickDirt = GenerateAt(7, coord, 4, 8);

    int countThin = 0, countThick = 0;
    for (VoxelId id : thinDirt.GetRawData())
        if (id == static_cast<VoxelId>(VoxelType::Dirt)) ++countThin;
    for (VoxelId id : thickDirt.GetRawData())
        if (id == static_cast<VoxelId>(VoxelType::Dirt)) ++countThick;

    REQUIRE(countThick >= countThin);
}

// ---------------------------------------------------------------------------
// Multi-chunk consistency
// ---------------------------------------------------------------------------

TEST_CASE("PCGWorldGen: adjacent chunks produced with same seed are deterministic", "[pcg]")
{
    PCGWorldGen gen;
    gen.SetSeed(555);

    Chunk a{{1, 0, 0}};
    Chunk b{{2, 0, 0}};
    gen.GenerateChunk(a);
    gen.GenerateChunk(b);

    // Re-generate and confirm identical output.
    Chunk a2{{1, 0, 0}};
    Chunk b2{{2, 0, 0}};
    gen.GenerateChunk(a2);
    gen.GenerateChunk(b2);

    REQUIRE(a.GetRawData() == a2.GetRawData());
    REQUIRE(b.GetRawData() == b2.GetRawData());
}

TEST_CASE("PCGWorldGen: chunk at non-zero coord differs from origin chunk", "[pcg]")
{
    ChunkCoord origin{0, 0, 0};
    ChunkCoord far   {5, 0, 5};

    Chunk a = GenerateAt(1, origin);
    Chunk b = GenerateAt(1, far);

    // Far-offset terrain should differ (extremely high probability).
    REQUIRE(a.GetRawData() != b.GetRawData());
}

TEST_CASE("PCGWorldGen: chunk coord is unchanged after generation", "[pcg]")
{
    ChunkCoord coord{3, 0, -2};
    Chunk chunk{coord};
    PCGWorldGen gen;
    gen.SetSeed(1);
    gen.GenerateChunk(chunk);

    REQUIRE(chunk.GetCoord().X == 3);
    REQUIRE(chunk.GetCoord().Y == 0);
    REQUIRE(chunk.GetCoord().Z == -2);
}

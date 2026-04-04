#include "Game/Voxel/VoxelEditApi.h"

namespace NF::Game {

// ---------------------------------------------------------------------------
// Read
// ---------------------------------------------------------------------------

VoxelId VoxelEditApi::GetVoxel(int32_t wx, int32_t wy, int32_t wz) const noexcept
{
    ChunkCoord cc = WorldToChunk(wx, wy, wz);
    const Chunk* chunk = m_Map.GetChunk(cc);
    if (!chunk) return static_cast<VoxelId>(VoxelType::Air);

    LocalVoxelCoord lc = WorldToLocal(wx, wy, wz);
    return chunk->GetVoxel(lc.X, lc.Y, lc.Z);
}

// ---------------------------------------------------------------------------
// Write
// ---------------------------------------------------------------------------

SetResult VoxelEditApi::SetVoxel(int32_t wx, int32_t wy, int32_t wz, VoxelId id)
{
    ChunkCoord cc = WorldToChunk(wx, wy, wz);
    Chunk* chunk = m_Map.GetOrCreateChunk(cc);

    LocalVoxelCoord lc = WorldToLocal(wx, wy, wz);
    chunk->SetVoxel(lc.X, lc.Y, lc.Z, id);

    // If the edited voxel sits on a chunk boundary, the face-adjacent
    // neighbour chunk may need to show or hide its boundary face.  Mark
    // it dirty so it re-meshes with the updated neighbour data next frame.
    auto markNeighbourDirty = [&](ChunkCoord nc) {
        Chunk* neighbour = m_Map.GetChunk(nc);
        if (neighbour) neighbour->MarkDirty();
    };

    const uint8_t kMaxLocalCoord = static_cast<uint8_t>(kChunkSize - 1);

    if (lc.X == 0)            markNeighbourDirty({cc.X - 1, cc.Y, cc.Z});
    if (lc.X == kMaxLocalCoord) markNeighbourDirty({cc.X + 1, cc.Y, cc.Z});
    if (lc.Y == 0)            markNeighbourDirty({cc.X, cc.Y - 1, cc.Z});
    if (lc.Y == kMaxLocalCoord) markNeighbourDirty({cc.X, cc.Y + 1, cc.Z});
    if (lc.Z == 0)            markNeighbourDirty({cc.X, cc.Y, cc.Z - 1});
    if (lc.Z == kMaxLocalCoord) markNeighbourDirty({cc.X, cc.Y, cc.Z + 1});

    return SetResult::Success;
}

// ---------------------------------------------------------------------------
// Damage / Mining
// ---------------------------------------------------------------------------

MineReport VoxelEditApi::Mine(int32_t wx, int32_t wy, int32_t wz,
                               uint8_t durability)
{
    const VoxelId id = GetVoxel(wx, wy, wz);
    const VoxelTypeInfo& info = GetVoxelTypeInfo(id);

    if (id == static_cast<VoxelId>(VoxelType::Air) || !info.isMineable) {
        return { MineResult::NotMineable, id, 0 };
    }

    if (durability == 0) {
        // Instant mine.
        SetVoxel(wx, wy, wz, VoxelType::Air);
        return { MineResult::Success, id, 0 };
    }

    // Apply hardness as damage.
    if (durability <= info.hardness) {
        // Voxel is destroyed.
        SetVoxel(wx, wy, wz, VoxelType::Air);
        return { MineResult::Success, id, 0 };
    }

    uint8_t remaining = static_cast<uint8_t>(durability - info.hardness);
    return { MineResult::DurabilityLeft, id, remaining };
}

uint8_t VoxelEditApi::Damage(int32_t wx, int32_t wy, int32_t wz,
                              uint8_t durability)
{
    const VoxelId id = GetVoxel(wx, wy, wz);
    const VoxelTypeInfo& info = GetVoxelTypeInfo(id);

    if (id == static_cast<VoxelId>(VoxelType::Air) || !info.isMineable)
        return durability;

    if (durability <= info.hardness) {
        SetVoxel(wx, wy, wz, VoxelType::Air);
        return 0;
    }
    return static_cast<uint8_t>(durability - info.hardness);
}

SetResult VoxelEditApi::Repair(int32_t wx, int32_t wy, int32_t wz, VoxelId id)
{
    return SetVoxel(wx, wy, wz, id);
}

} // namespace NF::Game

#include "Editor/Application/EditorWorldSession.h"
#include "Core/Logging/Log.h"
#include "Game/Voxel/ChunkCoord.h"
#include "Game/Voxel/VoxelType.h"
#include "Game/Gameplay/PCG/PCGWorldGen.h"
#include <cmath>

namespace NF::Editor {

void EditorWorldSession::Init(NF::Game::GameWorld& world, Level& level,
                               const std::string& contentRoot,
                               const std::string& worldName)
{
    m_World           = &world;
    m_Level           = &level;
    m_ContentRoot     = contentRoot;
    m_WorldName       = worldName;
    m_EntityPath      = contentRoot + "/Worlds/" + worldName + ".nfsv";
    m_ChunkPath       = contentRoot + "/Worlds/" + worldName + ".nfck";
    m_SolarSystemPath = contentRoot + "/Worlds/" + worldName + ".nfss";
    m_ItemsPath       = contentRoot + "/Worlds/" + worldName + ".nfpi";
    m_ConfigPath      = contentRoot + "/Definitions/" + worldName + ".json";
    m_Dirty           = false;

    NF::Logger::Log(NF::LogLevel::Info, "EditorWorldSession",
                    "Initialised — world: " + worldName
                    + ", content: " + contentRoot);
}

void EditorWorldSession::NewWorld()
{
    if (!m_World || !m_Level) return;

    NF::Logger::Log(NF::LogLevel::Info, "EditorWorldSession",
                    "Creating new world");
    m_World->Shutdown();
    m_World->Initialize(m_ContentRoot, m_WorldName);
    m_Level->Unload();
    m_Level->Load(m_WorldName);
    m_Dirty = false;
    NF::Logger::Log(NF::LogLevel::Info, "EditorWorldSession",
                    "New world ready");

    if (m_OnWorldChanged)
        m_OnWorldChanged();
}

void EditorWorldSession::LoadWorld(const std::string& worldName)
{
    if (!m_World || !m_Level) return;

    NF::Logger::Log(NF::LogLevel::Info, "EditorWorldSession",
                    "Loading world: " + worldName);

    m_World->Shutdown();
    m_WorldName       = worldName;
    m_EntityPath      = m_ContentRoot + "/Worlds/" + worldName + ".nfsv";
    m_ChunkPath       = m_ContentRoot + "/Worlds/" + worldName + ".nfck";
    m_SolarSystemPath = m_ContentRoot + "/Worlds/" + worldName + ".nfss";
    m_ItemsPath       = m_ContentRoot + "/Worlds/" + worldName + ".nfpi";
    m_ConfigPath      = m_ContentRoot + "/Definitions/" + worldName + ".json";

    m_World->Initialize(m_ContentRoot, worldName);
    LoadSavedChunks();
    m_Level->Unload();
    m_Level->Load(worldName);
    m_Dirty = false;

    NF::Logger::Log(NF::LogLevel::Info, "EditorWorldSession",
                    "World loaded: " + worldName);

    if (m_OnWorldChanged)
        m_OnWorldChanged();
}

bool EditorWorldSession::Save()
{
    if (!m_World) return false;

    NF::Logger::Log(NF::LogLevel::Info, "EditorWorldSession",
                    "Saving world...");
    const bool entityOk = m_World->SaveWorld(m_EntityPath);
    const bool chunkOk  = m_World->SaveChunks(m_ChunkPath);

    // Save world definition (spawn position, gravity, etc.) so changes
    // are picked up by the game on next launch.
    const bool configOk = m_World->GetConfig().SaveToFile(m_ConfigPath);

    // Save solar system data if one is wired up.
    bool solarOk = true;
    if (m_SolarSystem) {
        solarOk = m_SolarSystem->SaveToFile(m_SolarSystemPath);
        if (solarOk)
            NF::Logger::Log(NF::LogLevel::Info, "EditorWorldSession",
                            "Solar system saved");
    }

    // Save PCG item placements if an item generator is wired up.
    bool itemsOk = true;
    if (m_ItemGen) {
        itemsOk = m_ItemGen->SaveToFile(m_ItemsPath);
        if (itemsOk)
            NF::Logger::Log(NF::LogLevel::Info, "EditorWorldSession",
                            "PCG items saved ("
                            + std::to_string(m_ItemGen->ItemCount()) + " items)");
    }

    if (entityOk && chunkOk && configOk) {
        m_Dirty = false;
        NF::Logger::Log(NF::LogLevel::Info, "EditorWorldSession",
                        "World saved (entities + chunks + config"
                        + std::string(m_SolarSystem ? " + solar system" : "")
                        + std::string(m_ItemGen ? " + PCG items" : "")
                        + ")");
    } else {
        NF::Logger::Log(NF::LogLevel::Warning, "EditorWorldSession",
                        "World save incomplete — entities: "
                        + std::string(entityOk ? "ok" : "FAILED")
                        + ", chunks: "
                        + std::string(chunkOk ? "ok" : "FAILED")
                        + ", config: "
                        + std::string(configOk ? "ok" : "FAILED"));
    }

    return entityOk && chunkOk && configOk && solarOk && itemsOk;
}

void EditorWorldSession::Reload()
{
    if (!m_World || !m_Level) return;

    NF::Logger::Log(NF::LogLevel::Info, "EditorWorldSession",
                    "Reloading world...");
    m_World->Shutdown();
    m_World->Initialize(m_ContentRoot, m_WorldName);
    LoadSavedChunks();
    m_Level->Unload();
    m_Level->Load(m_WorldName);
    m_Dirty = false;
    NF::Logger::Log(NF::LogLevel::Info, "EditorWorldSession",
                    "World reloaded");

    if (m_OnWorldChanged)
        m_OnWorldChanged();
}

bool EditorWorldSession::LoadSavedChunks()
{
    if (!m_World) return false;

    bool chunksOk = false;
    if (m_World->LoadChunks(m_ChunkPath)) {
        chunksOk = true;
        NF::Logger::Log(NF::LogLevel::Info, "EditorWorldSession",
                        "Loaded saved chunk data ("
                        + std::to_string(m_World->GetLoadedChunkCount())
                        + " chunks)");
    }

    // Load PCG items if a file exists; it's non-fatal if it doesn't.
    if (m_ItemGen && m_ItemGen->LoadFromFile(m_ItemsPath)) {
        NF::Logger::Log(NF::LogLevel::Info, "EditorWorldSession",
                        "Loaded PCG items ("
                        + std::to_string(m_ItemGen->ItemCount()) + " items)");
    }

    return chunksOk;
}

// ---------------------------------------------------------------------------
// TravelToBody — per-body PCG terrain generation (Phase 5)
// ---------------------------------------------------------------------------

void EditorWorldSession::TravelToBody(const NF::Game::Gameplay::CelestialBody& body)
{
    if (!m_World) return;

    const uint32_t bodySeed = body.id * 2654435761u ^ 0xDEADBEEFu;
    // bodySeed: Knuth multiplicative hash (2654435761) mixed with body.id ensures
    // each body maps to a distinct well-spread seed; XOR with a fixed constant
    // ensures body 0 (star) does not collapse to seed 0.

    NF::Logger::Log(NF::LogLevel::Info, "EditorWorldSession",
                    "Travelling to " + body.name
                    + " (id=" + std::to_string(body.id)
                    + ", seed=" + std::to_string(bodySeed) + ")");

    // Clear existing terrain and regenerate using PCGWorldGen seeded from body.
    m_World->GetChunkMap().Clear();

    NF::Game::Gameplay::PCGWorldGen gen;
    gen.SetSeed(bodySeed);

    // Generate 3×3 chunks around world origin — same footprint as the default world.
    constexpr int kRadius = 1;
    for (int cx = -kRadius; cx <= kRadius; ++cx) {
        for (int cz = -kRadius; cz <= kRadius; ++cz) {
            NF::Game::ChunkCoord coord{cx, 0, cz};
            NF::Game::Chunk* chunk = m_World->GetChunkMap().GetOrCreateChunk(coord);
            if (chunk)
                gen.GenerateChunk(*chunk);
        }
    }

    // Overlay ore veins near deposit positions to bridge deposits → terrain.
    for (const auto& deposit : body.deposits) {
        // Convert flat-map deposit position to voxel chunk.
        const int32_t depCX = static_cast<int32_t>(std::floor(deposit.worldX / NF::Game::kChunkSize));
        const int32_t depCZ = static_cast<int32_t>(std::floor(deposit.worldZ / NF::Game::kChunkSize));
        NF::Game::ChunkCoord dc{depCX, 0, depCZ};
        NF::Game::Chunk* chunk = m_World->GetChunkMap().GetChunk(dc);
        if (!chunk) {
            chunk = m_World->GetChunkMap().GetOrCreateChunk(dc);
            if (chunk) gen.GenerateChunk(*chunk);
        }
        if (!chunk) continue;

        // Place ore voxels in a small vein around the deposit's local position.
        // Use safe modulo so negative worldX/worldZ values wrap into [0, kChunkSize).
        const auto safeMod = [](int32_t v, int32_t m) -> int32_t {
            return ((v % m) + m) % m;
        };
        const int32_t lx = safeMod(static_cast<int32_t>(deposit.worldX), NF::Game::kChunkSize);
        const int32_t lz = safeMod(static_cast<int32_t>(deposit.worldZ), NF::Game::kChunkSize);
        // Place at Y=4..7 (underground) to be mine-able.
        for (int32_t vy = 4; vy <= 7; ++vy) {
            for (int32_t vx = lx - 1; vx <= lx + 1; ++vx) {
                for (int32_t vz = lz - 1; vz <= lz + 1; ++vz) {
                    if (vx < 0 || vx >= NF::Game::kChunkSize) continue;
                    if (vz < 0 || vz >= NF::Game::kChunkSize) continue;
                    chunk->SetVoxel(
                        static_cast<uint8_t>(vx),
                        static_cast<uint8_t>(vy),
                        static_cast<uint8_t>(vz),
                        static_cast<NF::Game::VoxelId>(NF::Game::VoxelType::Ore));
                }
            }
        }
    }

    // Update exploration state.
    if (m_ExplorationSystem)
        m_ExplorationSystem->TravelToBody(body.id);

    m_Dirty = true;

    NF::Logger::Log(NF::LogLevel::Info, "EditorWorldSession",
                    "Arrived at " + body.name + " — terrain regenerated ("
                    + std::to_string(m_World->GetLoadedChunkCount()) + " chunks)");

    if (m_OnWorldChanged)
        m_OnWorldChanged();
}

} // namespace NF::Editor

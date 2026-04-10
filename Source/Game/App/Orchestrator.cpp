#include "Game/App/Orchestrator.h"
#include "Game/World/DevWorldConfig.h"
#include "Game/Voxel/ChunkCoord.h"
#include "Core/Config/ProjectManifest.h"
#include "Core/Logging/Log.h"

namespace NF::Game {

// ---------------------------------------------------------------------------
// Backward-compatible Init (Solo mode)
// ---------------------------------------------------------------------------

bool Orchestrator::Init(RenderDevice* renderDevice)
{
    NetParams solo;
    solo.Mode = NetMode::Solo;
    solo.PlayerName = "Player";
    return Init(renderDevice, solo);
}

// ---------------------------------------------------------------------------
// Init with explicit net mode
// ---------------------------------------------------------------------------

bool Orchestrator::Init(RenderDevice* renderDevice, const NetParams& params)
{
    m_RenderDevice = renderDevice;
    m_NetMode      = params.Mode;
    m_NetParams    = params;

    // ---- World + Level (always needed except pure Client) ----

    if (m_NetMode != NetMode::Client)
    {
        // Read world name and content root from the project manifest so
        // the game automatically picks up whatever the editor last saved.
        NF::ProjectManifest manifest;
        manifest.LoadFromFile("Config/novaforge.project.json");

        const std::string contentRoot =
            manifest.IsValid() ? manifest.ContentRoot : "Content";
        const std::string worldName =
            manifest.IsValid() ? manifest.DefaultWorld : "DevWorld";

        m_GameWorld.Initialize(contentRoot, worldName);

        // Load any previously saved chunk data so the game reflects the
        // same world state that was last saved in the editor.
        m_GameWorld.LoadChunks(contentRoot + "/Worlds/" + worldName + ".nfck");

        // Load solar system data if the editor saved one.
        const std::string solarPath = contentRoot + "/Worlds/" + worldName + ".nfss";
        if (m_SolarSystem.LoadFromFile(solarPath)) {
            NF::Logger::Log(NF::LogLevel::Info, "Game",
                            "Loaded solar system from " + solarPath
                            + " (" + std::to_string(m_SolarSystem.BodyCount()) + " bodies)");
        } else {
            // Fall back to procedural generation from the world seed.
            m_SolarSystem.SetSeed(m_GameWorld.GetConfig().Seed());
            m_SolarSystem.Generate();
        }

        m_Level.Load(worldName);
        m_InteractionLoop.Init(&m_GameWorld.GetVoxelEditApi());

        // ---- Core gameplay systems ----

        // Missions: auto-accept 3 starter missions.
        m_Missions.Init();

        // Wire mine callback → ProgressionSystem XP + MissionRegistry progress.
        m_InteractionLoop.SetOnMineSuccess(
            [this](NF::Game::ResourceType type, uint32_t count) {
                // XP per voxel (flat 10 XP matching MiningSystem::kXpPerMine).
                m_Progression.AddXP(10u * count);

                // Notify mission registry about mining and inventory change.
                m_Missions.NotifyMined(count);
                const uint32_t held = m_InteractionLoop.GetInventory().GetCount(type);
                m_Missions.NotifyInventoryChanged(type, held);

                NF::Logger::Log(NF::LogLevel::Debug, "Game",
                    "Mine: +" + std::to_string(10u * count) + " XP, level "
                    + std::to_string(m_Progression.GetLevel()));
            });

        // Wire ProgressionSystem level-up → MissionRegistry.
        m_Progression.SetLevelUpCallback([this](int level) {
            NF::Logger::Log(NF::LogLevel::Info, "Game",
                            "Level up! Now level " + std::to_string(level));
            m_Missions.NotifyLevelReached(level);
        });

        // Wire CombatSystem kills → XP + MissionRegistry.
        m_Combat.SetDeathCallback([this](uint32_t /*entityId*/) {
            constexpr uint32_t kKillXP = 25u;
            m_Progression.AddXP(kKillXP);
            m_Missions.NotifyKill();
        });

        // Wire MissionRegistry completion callback for logging.
        m_Missions.SetOnMissionComplete([this](uint32_t missionId) {
            const auto& allMissions = m_Missions.GetMissions();
            for (const auto& ms : allMissions) {
                if (ms.def && ms.def->id == missionId) {
                    NF::Logger::Log(NF::LogLevel::Info, "Game",
                        "Mission complete: " + ms.def->name);
                    break;
                }
            }
        });

        // Phase 8: initialise chunk streamer.
        m_Streamer = std::make_unique<ChunkStreamer>();
        ChunkStreamConfig streamCfg;
        streamCfg.LoadRadius   = 4;
        streamCfg.UnloadRadius = 6;
        streamCfg.MaxLoadedChunks = 512;
        streamCfg.MaxLoadsPerTick = 4;
        streamCfg.MaxUnloadsPerTick = 4;
        streamCfg.SaveOnUnload = true;
        m_Streamer->Init(&m_GameWorld.GetChunkMap(), nullptr,
                         streamCfg, m_GameWorld.GetConfig().Seed());
    }

    // ---- Networking ----

    switch (m_NetMode)
    {
    case NetMode::Solo:
    {
        m_Server = std::make_unique<GameServer>();
        if (!m_Server->Init(&m_GameWorld, 0)) return false;
        m_LocalClientId = m_Server->AddLocalClient(params.PlayerName);

        // Spawn player at world spawn point.
        const auto& sp = m_GameWorld.GetSpawnPoint();
        m_PlayerMovement.SetPosition({sp.Position.X,
                                       sp.Position.Y + 2.f,
                                       sp.Position.Z});
        break;
    }
    case NetMode::ListenServer:
    {
        m_Server = std::make_unique<GameServer>();
        if (!m_Server->Init(&m_GameWorld, params.Port)) return false;
        m_LocalClientId = m_Server->AddLocalClient(params.PlayerName);

        // Spawn player at world spawn point.
        const auto& sp = m_GameWorld.GetSpawnPoint();
        m_PlayerMovement.SetPosition({sp.Position.X,
                                       sp.Position.Y + 2.f,
                                       sp.Position.Z});
        break;
    }
    case NetMode::Dedicated:
    {
        m_Server = std::make_unique<GameServer>();
        if (!m_Server->Init(&m_GameWorld, params.Port)) return false;
        break;
    }
    case NetMode::Client:
    {
        m_Client = std::make_unique<GameClient>();
        if (!m_Client->Connect(params.Host, params.Port, params.PlayerName))
            return false;
        break;
    }
    }

    // ---- Audio (Phase 9) ----

    m_AudioDevice.Init(NF::AudioBackend::Null);
    m_AudioMixer.SetSoundBank(&m_SoundBank);
    m_SpatialAudio.SetMixer(&m_AudioMixer);

    // Generate placeholder sounds.
    m_SoundBank.GenerateTone("sfx_mine",    800.f, 0.15f, 0.4f);
    m_SoundBank.GenerateTone("sfx_place",   400.f, 0.2f,  0.3f);
    m_SoundBank.GenerateNoise("sfx_ambient", 2.0f, 0.05f);

    m_Initialized = true;
    NF::Logger::Log(NF::LogLevel::Info, "Game",
                    "Orchestrator::Init complete (NetMode=" +
                    std::to_string(static_cast<int>(m_NetMode)) + ")");
    return true;
}

// ---------------------------------------------------------------------------
// Tick
// ---------------------------------------------------------------------------

void Orchestrator::Tick(float dt)
{
    if (!m_Initialized) return;

    switch (m_NetMode)
    {
    case NetMode::Solo:
    case NetMode::ListenServer:
    case NetMode::Dedicated:
    {
        // Server tick (accepts connections, processes input, builds snapshot).
        if (m_Server) m_Server->Tick(dt);

        // World + level tick (authoritative on server side).
        m_Level.Update(dt);
        m_GameWorld.Tick(dt);
        m_InteractionLoop.Tick(dt);

        // Solo / ListenServer: local player movement.
        if (m_NetMode != NetMode::Dedicated)
        {
            m_PlayerMovement.Update(dt, m_GameWorld.GetChunkMap());

            // Clear collision-dirty flags after movement resolution.
            // The voxel collision system reads solidity directly from
            // voxel data, so no separate collision structure rebuild is
            // needed — just clear the flags so they don't accumulate.
            m_GameWorld.GetChunkMap().ClearAllCollisionDirty();

            // Phase 8: tick chunk streamer with player position.
            if (m_Streamer)
            {
                const auto& pos = m_PlayerMovement.GetPosition();
                const ChunkCoord viewerChunk = WorldToChunk(
                    static_cast<int32_t>(pos.X),
                    static_cast<int32_t>(pos.Y),
                    static_cast<int32_t>(pos.Z));
                m_Streamer->Tick(viewerChunk);
            }

            // Phase 9: update spatial audio listener from player position.
            m_SpatialAudio.SetListenerPosition(m_PlayerMovement.GetPosition());
        }
        else
        {
            // Dedicated server: stream around origin.
            if (m_Streamer)
                m_Streamer->Tick({0, 0, 0});
        }
        break;
    }
    case NetMode::Client:
    {
        if (m_Client) m_Client->Update(dt);
        break;
    }
    }

    // Phase 9: advance audio mixer.
    m_AudioMixer.Update(dt);
}

// ---------------------------------------------------------------------------
// Shutdown
// ---------------------------------------------------------------------------

void Orchestrator::Shutdown()
{
    if (!m_Initialized) return;

    if (m_Client) { m_Client->Disconnect(); m_Client.reset(); }
    if (m_Server) { m_Server->Shutdown();   m_Server.reset(); }
    if (m_Streamer) { m_Streamer->Shutdown(); m_Streamer.reset(); }

    // Phase 9: shutdown audio.
    m_AudioMixer.StopAll();
    m_SoundBank.Clear();
    m_AudioDevice.Shutdown();

    m_GameWorld.Shutdown();
    m_Level.Unload();
    m_RenderDevice   = nullptr;
    m_Initialized    = false;
    m_LocalClientId  = 0;
    NF::Logger::Log(NF::LogLevel::Info, "Game", "Orchestrator::Shutdown");
}

} // namespace NF::Game

#pragma once
#include "Engine/World/Level.h"
#include "Renderer/RHI/RenderDevice.h"
#include "Game/World/GameWorld.h"
#include "Game/Interaction/InteractionLoop.h"
#include "Game/Movement/PlayerMovement.h"
#include "Game/Net/GameServer.h"
#include "Game/Net/GameClient.h"
#include "Game/Voxel/ChunkStreamer.h"
#include "Game/Gameplay/SolarSystem/DevSolarSystem.h"
#include "Game/Gameplay/Progression/ProgressionSystem.h"
#include "Game/Gameplay/Combat/CombatSystem.h"
#include "Game/Gameplay/Missions/MissionRegistry.h"
#include "Game/Gameplay/Economy/ResourceRegistry.h"
#include "Game/Gameplay/Station/StationServices.h"
#include "Game/Gameplay/Salvage/SalvageSystem.h"
#include "Game/Gameplay/Storage/StorageSystem.h"
#include "Game/Gameplay/Inventory/InventorySystem.h"
#include "Audio/Core/AudioDevice.h"
#include "Audio/Core/SoundBank.h"
#include "Audio/Mixer/AudioMixer.h"
#include "Audio/Spatial/SpatialAudio.h"
#include "Game/Gameplay/Factions/FactionRegistry.h"
#include "Game/Gameplay/Anomaly/AnomalySystem.h"
#include <memory>
#include <string>

namespace NF::Game {

/// @brief Network mode for the Orchestrator session.
enum class NetMode : uint8_t {
    Solo          = 0,  ///< Single-player, no networking.
    ListenServer  = 1,  ///< Host plays while serving remote clients.
    Dedicated     = 2,  ///< Headless server only — no local player.
    Client        = 3,  ///< Remote client connecting to a server.
};

/// @brief Connection parameters for networked modes.
struct NetParams {
    NetMode     Mode{NetMode::Solo};
    std::string Host{"127.0.0.1"};   ///< Server address (Client mode).
    uint16_t    Port{7777};           ///< Listen port (server) or connect port (client).
    std::string PlayerName{"Player"};
};

/// @brief Top-level game runtime orchestrator.
///
/// Owns the active @c Level, the game world (voxel + ECS), the Phase 3
/// interaction loop, and the Phase 5 player movement controller.
/// In Phase 7 also owns GameServer / GameClient depending on the
/// active @c NetMode.
///
/// Holds a non-owning pointer to the @c RenderDevice provided at Init()
/// time.  Call Init(), then loop Tick(), then Shutdown().
class Orchestrator {
public:
    Orchestrator() = default;
    ~Orchestrator() = default;

    Orchestrator(const Orchestrator&)            = delete;
    Orchestrator& operator=(const Orchestrator&) = delete;

    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------

    /// @brief Initialise in Solo mode (backward compatible).
    bool Init(RenderDevice* renderDevice);

    /// @brief Initialise with explicit net mode and connection parameters.
    bool Init(RenderDevice* renderDevice, const NetParams& params);

    /// @brief Advance the game simulation by one variable-rate tick.
    void Tick(float dt);

    /// @brief Tear down all runtime subsystems.
    void Shutdown();

    // -------------------------------------------------------------------------
    // Accessors
    // -------------------------------------------------------------------------

    [[nodiscard]] Level*       GetLevel()       noexcept { return &m_Level; }
    [[nodiscard]] const Level* GetLevel() const noexcept { return &m_Level; }

    [[nodiscard]] GameWorld&       GetGameWorld()       noexcept { return m_GameWorld; }
    [[nodiscard]] const GameWorld& GetGameWorld() const noexcept { return m_GameWorld; }

    [[nodiscard]] InteractionLoop&       GetInteractionLoop()       noexcept { return m_InteractionLoop; }
    [[nodiscard]] const InteractionLoop& GetInteractionLoop() const noexcept { return m_InteractionLoop; }

    [[nodiscard]] PlayerMovement&       GetPlayerMovement()       noexcept { return m_PlayerMovement; }
    [[nodiscard]] const PlayerMovement& GetPlayerMovement() const noexcept { return m_PlayerMovement; }

    [[nodiscard]] bool IsInitialized() const noexcept { return m_Initialized; }
    [[nodiscard]] NetMode GetNetMode() const noexcept { return m_NetMode; }

    /// @brief Returns the chunk streamer (non-null after Init for server modes).
    [[nodiscard]] ChunkStreamer* GetChunkStreamer() noexcept { return m_Streamer.get(); }

    /// @brief Returns the audio device (always available after Init).
    [[nodiscard]] AudioDevice&       GetAudioDevice()       noexcept { return m_AudioDevice; }
    [[nodiscard]] const AudioDevice& GetAudioDevice() const noexcept { return m_AudioDevice; }

    /// @brief Returns the sound bank.
    [[nodiscard]] SoundBank&       GetSoundBank()       noexcept { return m_SoundBank; }
    [[nodiscard]] const SoundBank& GetSoundBank() const noexcept { return m_SoundBank; }

    /// @brief Returns the audio mixer.
    [[nodiscard]] AudioMixer&       GetAudioMixer()       noexcept { return m_AudioMixer; }
    [[nodiscard]] const AudioMixer& GetAudioMixer() const noexcept { return m_AudioMixer; }

    /// @brief Returns the spatial audio manager.
    [[nodiscard]] SpatialAudio&       GetSpatialAudio()       noexcept { return m_SpatialAudio; }
    [[nodiscard]] const SpatialAudio& GetSpatialAudio() const noexcept { return m_SpatialAudio; }

    /// @brief Returns the solar system (loaded from editor data or generated).
    [[nodiscard]] NF::Game::Gameplay::DevSolarSystem&       GetSolarSystem()       noexcept { return m_SolarSystem; }
    [[nodiscard]] const NF::Game::Gameplay::DevSolarSystem& GetSolarSystem() const noexcept { return m_SolarSystem; }

    /// @brief Returns the ProgressionSystem (level + XP + skill unlocks).
    [[nodiscard]] NF::Game::Gameplay::ProgressionSystem&       GetProgression()       noexcept { return m_Progression; }
    [[nodiscard]] const NF::Game::Gameplay::ProgressionSystem& GetProgression() const noexcept { return m_Progression; }

    /// @brief Returns the CombatSystem (damage model, kills, respawn).
    [[nodiscard]] NF::Game::Gameplay::CombatSystem&       GetCombat()       noexcept { return m_Combat; }
    [[nodiscard]] const NF::Game::Gameplay::CombatSystem& GetCombat() const noexcept { return m_Combat; }

    /// @brief Returns the MissionRegistry (starter missions).
    [[nodiscard]] NF::Game::Gameplay::MissionRegistry&       GetMissions()       noexcept { return m_Missions; }
    [[nodiscard]] const NF::Game::Gameplay::MissionRegistry& GetMissions() const noexcept { return m_Missions; }

    /// @brief Returns the ResourceRegistry (static resource catalog).
    [[nodiscard]] NF::Game::Gameplay::ResourceRegistry&       GetResources()       noexcept { return m_Resources; }
    [[nodiscard]] const NF::Game::Gameplay::ResourceRegistry& GetResources() const noexcept { return m_Resources; }

    /// @brief Returns the starter station (Homebase).
    [[nodiscard]] NF::Game::Gameplay::StationServices&       GetStation()       noexcept { return m_Station; }
    [[nodiscard]] const NF::Game::Gameplay::StationServices& GetStation() const noexcept { return m_Station; }

    /// @brief Returns the salvage system (wreck sites).
    [[nodiscard]] NF::Game::Gameplay::SalvageSystem&       GetSalvage()       noexcept { return m_Salvage; }
    [[nodiscard]] const NF::Game::Gameplay::SalvageSystem& GetSalvage() const noexcept { return m_Salvage; }

    /// @brief Returns the world storage system (placed boxes).
    [[nodiscard]] NF::Game::Gameplay::StorageSystem&       GetStorage()       noexcept { return m_Storage; }
    [[nodiscard]] const NF::Game::Gameplay::StorageSystem& GetStorage() const noexcept { return m_Storage; }

    /// @brief Returns the inventory system (named containers).
    [[nodiscard]] NF::Game::Gameplay::InventorySystem&       GetInventorySys()       noexcept { return m_InventorySys; }
    [[nodiscard]] const NF::Game::Gameplay::InventorySystem& GetInventorySys() const noexcept { return m_InventorySys; }

    /// @brief Returns the faction registry (player reputation with factions).
    [[nodiscard]] NF::Game::Gameplay::FactionRegistry&       GetFactions()       noexcept { return m_Factions; }
    [[nodiscard]] const NF::Game::Gameplay::FactionRegistry& GetFactions() const noexcept { return m_Factions; }

    /// @brief Returns the anomaly system (encounter sites).
    [[nodiscard]] NF::Game::Gameplay::AnomalySystem&       GetAnomalies()       noexcept { return m_Anomalies; }
    [[nodiscard]] const NF::Game::Gameplay::AnomalySystem& GetAnomalies() const noexcept { return m_Anomalies; }

    /// @brief Returns the GameServer (non-null when Solo/ListenServer/Dedicated).
    [[nodiscard]] GameServer* GetServer() noexcept { return m_Server.get(); }
    /// @brief Returns the GameClient (non-null when ListenServer/Client).
    [[nodiscard]] GameClient* GetClient() noexcept { return m_Client.get(); }

    /// @brief Local client id on the server (Solo / ListenServer only).
    [[nodiscard]] uint32_t GetLocalClientId() const noexcept { return m_LocalClientId; }

private:
    Level            m_Level;
    GameWorld        m_GameWorld;
    InteractionLoop  m_InteractionLoop;
    PlayerMovement   m_PlayerMovement;
    RenderDevice*    m_RenderDevice{nullptr};
    bool             m_Initialized{false};

    // Phase 7 networking
    NetMode                      m_NetMode{NetMode::Solo};
    std::unique_ptr<GameServer>  m_Server;
    std::unique_ptr<GameClient>  m_Client;
    uint32_t                     m_LocalClientId{0};
    NetParams                    m_NetParams;

    // Phase 8 chunk streaming
    std::unique_ptr<ChunkStreamer> m_Streamer;

    // Solar system — loaded from editor data or procedurally generated.
    NF::Game::Gameplay::DevSolarSystem m_SolarSystem;

    // Core gameplay systems — wired in Init().
    NF::Game::Gameplay::ProgressionSystem m_Progression;
    NF::Game::Gameplay::CombatSystem      m_Combat;
    NF::Game::Gameplay::MissionRegistry   m_Missions;

    // Phase 6 economy systems.
    NF::Game::Gameplay::ResourceRegistry  m_Resources;
    NF::Game::Gameplay::StationServices   m_Station{"Homebase"};

    // Phase 7 salvage / storage / inventory systems.
    NF::Game::Gameplay::SalvageSystem     m_Salvage;
    NF::Game::Gameplay::StorageSystem     m_Storage;
    NF::Game::Gameplay::InventorySystem   m_InventorySys;

    // Phase 10 factions and anomalies.
    NF::Game::Gameplay::FactionRegistry   m_Factions;
    NF::Game::Gameplay::AnomalySystem     m_Anomalies;

    // Phase 9 audio
    AudioDevice  m_AudioDevice;
    SoundBank    m_SoundBank;
    AudioMixer   m_AudioMixer;
    SpatialAudio m_SpatialAudio;
};

} // namespace NF::Game

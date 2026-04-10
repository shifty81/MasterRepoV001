#pragma once
// MissionRegistry.h — 2–3 starter missions spawnable in DevWorld.
//
// Missions are data-driven; each mission has an objective, optional reward,
// and a status.  The MissionRegistry owns all active missions for the
// current session and evaluates completion conditions in Tick().
#include "Game/Interaction/ResourceItem.h"
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace NF::Game::Gameplay {

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------

/// @brief High-level mission completion condition.
enum class MissionObjectiveType : uint8_t {
    MineVoxels,       ///< Mine N voxels of any type.
    CollectResource,  ///< Hold >= N units of a specific resource in inventory.
    ReachLevel,       ///< Reach progression level >= N.
    KillEnemies,      ///< Kill N entities via CombatSystem.
};

/// @brief Lifecycle state of a mission.
enum class MissionStatus : uint8_t {
    Inactive,    ///< Not yet accepted.
    Active,      ///< Accepted, objective not yet met.
    Complete,    ///< Objective met; reward not yet collected.
    Collected,   ///< Reward collected; mission is done.
    Failed,      ///< Explicitly failed (reserved for future use).
};

/// @brief Reward granted on mission completion.
struct MissionReward {
    uint32_t              xp{0};                            ///< XP awarded.
    NF::Game::ResourceType resourceType{NF::Game::ResourceType::None};
    uint32_t              resourceAmount{0};                ///< Units of resource awarded.
    float                 credits{0.f};                     ///< Credits awarded.
};

/// @brief Definition of a single mission.
struct MissionDef {
    uint32_t              id{0};
    std::string           name;
    std::string           description;
    MissionObjectiveType  objectiveType{MissionObjectiveType::MineVoxels};
    uint32_t              objectiveTarget{0}; ///< Quantity needed.
    MissionReward         reward;
};

/// @brief Runtime state of an active mission.
struct MissionState {
    const MissionDef* def{nullptr};
    MissionStatus     status{MissionStatus::Inactive};
    uint32_t          progress{0}; ///< Current objective count.
};

// ---------------------------------------------------------------------------
// MissionRegistry
// ---------------------------------------------------------------------------

/// @brief Central registry for session missions.
///
/// At Init() three starter missions are registered.  Callers report
/// gameplay events (mine, kill, level-up) via the Notify*() family and
/// the registry advances progress and status accordingly.
///
/// Rewards are applied via callbacks so the registry does not own game
/// state directly.
class MissionRegistry {
public:
    MissionRegistry() = default;

    // ---- Lifecycle ----------------------------------------------------------

    /// @brief Register the 3 starter missions and reset all state.
    void Init();

    // ---- Callbacks ----------------------------------------------------------

    /// @brief Called when a mission transitions to Complete or Collected.
    ///        Receives the mission id of the completed mission.
    void SetOnMissionComplete(std::function<void(uint32_t)> cb) {
        m_OnComplete = std::move(cb);
    }

    // ---- Mission management -------------------------------------------------

    /// @brief Accept a mission (transition Inactive → Active).
    /// @return False if already active or not found.
    bool Accept(uint32_t missionId);

    /// @brief Collect the reward for a completed mission (Complete → Collected).
    /// @return The reward to apply, or a zero reward if not ready.
    MissionReward CollectReward(uint32_t missionId);

    // ---- Event notifications ------------------------------------------------

    /// @brief Report that the player mined N voxels this frame.
    void NotifyMined(uint32_t count = 1);

    /// @brief Report that the player now holds at least @p count of @p type.
    ///        Called after an inventory change.
    void NotifyInventoryChanged(NF::Game::ResourceType type, uint32_t count);

    /// @brief Report that the player reached a new level.
    void NotifyLevelReached(int level);

    /// @brief Report that the player killed an enemy.
    void NotifyKill();

    // ---- Queries ------------------------------------------------------------

    [[nodiscard]] const std::vector<MissionState>& GetMissions() const noexcept {
        return m_Missions;
    }

    [[nodiscard]] int ActiveCount()    const noexcept;
    [[nodiscard]] int CompleteCount()  const noexcept;
    [[nodiscard]] int CollectedCount() const noexcept;

private:
    std::vector<MissionDef>   m_Defs;
    std::vector<MissionState> m_Missions;

    std::function<void(uint32_t)> m_OnComplete;

    /// @brief Return a mutable pointer to the mission state by id.
    MissionState* FindState(uint32_t id) noexcept;

    /// @brief Advance progress and potentially complete a mission.
    void Advance(MissionState& ms, uint32_t progress);
};

} // namespace NF::Game::Gameplay

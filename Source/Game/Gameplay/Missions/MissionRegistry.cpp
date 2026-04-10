// MissionRegistry.cpp — 3 starter missions spawnable in DevWorld.
#include "Game/Gameplay/Missions/MissionRegistry.h"

namespace NF::Game::Gameplay {

// ---------------------------------------------------------------------------
// Starter mission definitions
// ---------------------------------------------------------------------------

static const MissionDef kStarterMissions[] = {
    // Mission 1: First Contact — mine 10 voxels of any kind.
    {
        1u,
        "First Contact",
        "The RIG's mining array is cold. Break ground: mine 10 voxels to warm it up.",
        MissionObjectiveType::MineVoxels,
        10u,
        { 50u,  NF::Game::ResourceType::Stone, 5u,  0.f  }
    },
    // Mission 2: Ore Run — collect 5 units of Ore in inventory.
    {
        2u,
        "Ore Run",
        "The station processor is hungry. Collect 5 units of raw Ore.",
        MissionObjectiveType::CollectResource,
        5u,
        { 150u, NF::Game::ResourceType::Metal, 2u,  25.f }
    },
    // Mission 3: Journeyman — reach progression level 3.
    {
        3u,
        "Journeyman",
        "Prove your worth. Reach Pilot level 3 through mining and construction.",
        MissionObjectiveType::ReachLevel,
        3u,
        { 300u, NF::Game::ResourceType::None,  0u, 100.f }
    },
};

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------

void MissionRegistry::Init()
{
    m_Defs.clear();
    m_Missions.clear();

    for (const auto& def : kStarterMissions)
    {
        m_Defs.push_back(def);
        MissionState ms;
        ms.def      = &m_Defs.back();
        ms.status   = MissionStatus::Active; // auto-accept all starter missions
        ms.progress = 0;
        m_Missions.push_back(ms);
    }
}

// ---------------------------------------------------------------------------
// FindState
// ---------------------------------------------------------------------------

MissionState* MissionRegistry::FindState(uint32_t id) noexcept
{
    for (auto& ms : m_Missions)
        if (ms.def && ms.def->id == id)
            return &ms;
    return nullptr;
}

// ---------------------------------------------------------------------------
// Accept
// ---------------------------------------------------------------------------

bool MissionRegistry::Accept(uint32_t missionId)
{
    MissionState* ms = FindState(missionId);
    if (!ms || ms->status != MissionStatus::Inactive)
        return false;
    ms->status = MissionStatus::Active;
    return true;
}

// ---------------------------------------------------------------------------
// CollectReward
// ---------------------------------------------------------------------------

MissionReward MissionRegistry::CollectReward(uint32_t missionId)
{
    MissionState* ms = FindState(missionId);
    if (!ms || ms->status != MissionStatus::Complete)
        return MissionReward{};

    ms->status = MissionStatus::Collected;
    return ms->def->reward;
}

// ---------------------------------------------------------------------------
// Advance — shared completion logic
// ---------------------------------------------------------------------------

void MissionRegistry::Advance(MissionState& ms, uint32_t newProgress)
{
    if (ms.status != MissionStatus::Active) return;

    ms.progress = newProgress;
    if (ms.progress >= ms.def->objectiveTarget) {
        ms.status   = MissionStatus::Complete;
        ms.progress = ms.def->objectiveTarget; // clamp
        if (m_OnComplete)
            m_OnComplete(ms.def->id);
    }
}

// ---------------------------------------------------------------------------
// Notifications
// ---------------------------------------------------------------------------

void MissionRegistry::NotifyMined(uint32_t count)
{
    for (auto& ms : m_Missions) {
        if (ms.status != MissionStatus::Active) continue;
        if (ms.def->objectiveType == MissionObjectiveType::MineVoxels)
            Advance(ms, ms.progress + count);
    }
}

void MissionRegistry::NotifyInventoryChanged(NF::Game::ResourceType type, uint32_t count)
{
    for (auto& ms : m_Missions) {
        if (ms.status != MissionStatus::Active) continue;
        if (ms.def->objectiveType == MissionObjectiveType::CollectResource) {
            // Check if the resource type matches — None means any.
            // For "Ore Run", collect Ore.  The objectiveTarget stores units needed.
            // We treat CollectResource as: reach at least objectiveTarget of the type
            // whose ResourceType is stored in the reward (as a proxy for objective type).
            // For simplicity, the first CollectResource mission targets Ore.
            const bool matches =
                (ms.def->reward.resourceType == type)
                || (ms.def->reward.resourceType == NF::Game::ResourceType::None);
            if (matches)
                Advance(ms, count); // absolute count, not incremental
        }
    }
}

void MissionRegistry::NotifyLevelReached(int level)
{
    for (auto& ms : m_Missions) {
        if (ms.status != MissionStatus::Active) continue;
        if (ms.def->objectiveType == MissionObjectiveType::ReachLevel)
            Advance(ms, static_cast<uint32_t>(level));
    }
}

void MissionRegistry::NotifyKill()
{
    for (auto& ms : m_Missions) {
        if (ms.status != MissionStatus::Active) continue;
        if (ms.def->objectiveType == MissionObjectiveType::KillEnemies)
            Advance(ms, ms.progress + 1u);
    }
}

// ---------------------------------------------------------------------------
// Count queries
// ---------------------------------------------------------------------------

int MissionRegistry::ActiveCount() const noexcept
{
    int n = 0;
    for (const auto& ms : m_Missions)
        if (ms.status == MissionStatus::Active) ++n;
    return n;
}

int MissionRegistry::CompleteCount() const noexcept
{
    int n = 0;
    for (const auto& ms : m_Missions)
        if (ms.status == MissionStatus::Complete) ++n;
    return n;
}

int MissionRegistry::CollectedCount() const noexcept
{
    int n = 0;
    for (const auto& ms : m_Missions)
        if (ms.status == MissionStatus::Collected) ++n;
    return n;
}

} // namespace NF::Game::Gameplay


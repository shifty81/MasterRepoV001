#include "Game/Gameplay/Anomaly/AnomalySystem.h"
#include "Game/Interaction/Inventory.h"
#include <cmath>
#include <algorithm>

namespace NF::Game::Gameplay {

// ---------------------------------------------------------------------------
// Placement
// ---------------------------------------------------------------------------

AnomalyId AnomalySystem::PlaceAnomaly(const std::string& name,
                                       const NF::Vector3& position,
                                       AnomalyType type)
{
    Anomaly a;
    a.id       = m_NextId++;
    a.name     = name;
    a.position = position;
    a.type     = type;
    m_Anomalies.push_back(std::move(a));
    return m_Anomalies.back().id;
}

void AnomalySystem::AddLoot(AnomalyId id, NF::Game::ResourceType type, uint32_t count)
{
    Anomaly* a = FindMutable(id);
    if (!a || count == 0) return;
    // Merge with an existing entry of the same type if present.
    for (auto& entry : a->loot)
    {
        if (entry.type == type) { entry.count += count; return; }
    }
    a->loot.push_back({type, count});
}

// ---------------------------------------------------------------------------
// Interaction
// ---------------------------------------------------------------------------

AnomalyId AnomalySystem::FindNearest(const NF::Vector3& pos, float radius) const noexcept
{
    AnomalyId bestId   = kInvalidAnomalyId;
    float     bestDistSq = radius * radius;

    for (const auto& a : m_Anomalies)
    {
        if (a.investigated) continue;

        const float dx = a.position.X - pos.X;
        const float dy = a.position.Y - pos.Y;
        const float dz = a.position.Z - pos.Z;
        const float distSq = dx * dx + dy * dy + dz * dz;

        if (distSq < bestDistSq)
        {
            bestDistSq = distSq;
            bestId     = a.id;
        }
    }
    return bestId;
}

uint32_t AnomalySystem::Investigate(AnomalyId id, NF::Game::Inventory& inventory)
{
    Anomaly* a = FindMutable(id);
    if (!a || a->investigated) return 0;

    uint32_t total = 0;
    for (const auto& entry : a->loot)
    {
        if (entry.count == 0) continue;
    inventory.AddItem(entry.type, entry.count);
        total += entry.count;
    }
    a->loot.clear();
    a->investigated = true;

    if (m_OnInvestigated)
        m_OnInvestigated(id, total);

    return total;
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

const Anomaly* AnomalySystem::Get(AnomalyId id) const noexcept
{
    for (const auto& a : m_Anomalies)
        if (a.id == id) return &a;
    return nullptr;
}

const char* AnomalySystem::TypeName(AnomalyType t) noexcept
{
    switch (t)
    {
    case AnomalyType::Derelict:  return "Derelict";
    case AnomalyType::Mineral:   return "Mineral";
    case AnomalyType::Signal:    return "Signal";
    case AnomalyType::Radiation: return "Radiation";
    }
    return "Unknown";
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

Anomaly* AnomalySystem::FindMutable(AnomalyId id) noexcept
{
    for (auto& a : m_Anomalies)
        if (a.id == id) return &a;
    return nullptr;
}

} // namespace NF::Game::Gameplay

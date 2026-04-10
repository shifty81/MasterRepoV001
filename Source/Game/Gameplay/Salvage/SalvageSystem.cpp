// SalvageSystem.cpp — wreck-site salvage implementation.
#include "Game/Gameplay/Salvage/SalvageSystem.h"
#include <algorithm>
#include <cmath>

namespace NF::Game::Gameplay {

// ---------------------------------------------------------------------------
// WreckSite helpers
// ---------------------------------------------------------------------------

bool WreckSite::IsEmpty() const noexcept
{
    for (int i = 0; i < lootCount; ++i)
        if (loot[i].count > 0) return false;
    return true;
}

uint32_t WreckSite::TotalLoot() const noexcept
{
    uint32_t total = 0;
    for (int i = 0; i < lootCount; ++i)
        total += loot[i].count;
    return total;
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

WreckSite* SalvageSystem::GetWreck(WreckId id) noexcept
{
    for (auto& w : m_Wrecks)
        if (w.id == id) return &w;
    return nullptr;
}

const WreckSite* SalvageSystem::GetWreck(WreckId id) const noexcept
{
    for (const auto& w : m_Wrecks)
        if (w.id == id) return &w;
    return nullptr;
}

// ---------------------------------------------------------------------------
// Wreck management
// ---------------------------------------------------------------------------

WreckId SalvageSystem::PlaceWreck(const std::string& name, const NF::Vector3& position)
{
    WreckSite site;
    site.id       = m_NextId++;
    site.name     = name;
    site.position = position;
    m_Wrecks.push_back(std::move(site));
    return m_Wrecks.back().id;
}

bool SalvageSystem::AddLoot(WreckId id, NF::Game::ResourceType type, uint32_t count)
{
    auto* w = GetWreck(id);
    if (!w) return false;

    // Check if the type already has an entry.
    for (int i = 0; i < w->lootCount; ++i) {
        if (w->loot[i].type == type) {
            w->loot[i].count += count;
            return true;
        }
    }

    if (w->lootCount >= kMaxWreckLoot) return false;
    w->loot[w->lootCount].type  = type;
    w->loot[w->lootCount].count = count;
    ++w->lootCount;
    return true;
}

bool SalvageSystem::RemoveWreck(WreckId id)
{
    const auto it = std::find_if(m_Wrecks.begin(), m_Wrecks.end(),
        [id](const WreckSite& w) { return w.id == id; });
    if (it == m_Wrecks.end()) return false;
    m_Wrecks.erase(it);
    return true;
}

// ---------------------------------------------------------------------------
// FindNearest
// ---------------------------------------------------------------------------

WreckId SalvageSystem::FindNearest(const NF::Vector3& position, float radius) const noexcept
{
    WreckId bestId   = kInvalidWreckId;
    float   bestDistSq = radius * radius;

    for (const auto& w : m_Wrecks) {
        if (w.IsEmpty()) continue;
        const NF::Vector3 diff = w.position - position;
        const float distSq = diff.LengthSq();
        if (distSq < bestDistSq) {
            bestDistSq = distSq;
            bestId     = w.id;
        }
    }
    return bestId;
}

// ---------------------------------------------------------------------------
// Salvage
// ---------------------------------------------------------------------------

uint32_t SalvageSystem::Salvage(WreckId id, NF::Game::Inventory& inv, uint32_t maxPerBatch)
{
    auto* w = GetWreck(id);
    if (!w || w->IsEmpty()) return 0;

    uint32_t totalExtracted = 0;
    uint32_t remaining      = maxPerBatch;

    for (int i = 0; i < w->lootCount && remaining > 0; ++i) {
        auto& slot = w->loot[i];
        if (slot.count == 0) continue;

        const uint32_t toExtract = std::min(slot.count, remaining);
        if (!inv.AddItem(slot.type, toExtract)) continue; // inventory full

        slot.count     -= toExtract;
        totalExtracted += toExtract;
        remaining      -= toExtract;
    }

    return totalExtracted;
}

} // namespace NF::Game::Gameplay

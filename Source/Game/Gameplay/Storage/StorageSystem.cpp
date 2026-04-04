#include "Game/Gameplay/Storage/StorageSystem.h"
#include <algorithm>
#include <cmath>

namespace NF::Game::Gameplay {

StorageSystem::BoxEntry* StorageSystem::FindEntry(StorageBoxId id) noexcept
{
    for (auto& b : m_Boxes)
        if (b.id == id) return &b;
    return nullptr;
}

const StorageSystem::BoxEntry* StorageSystem::FindEntry(StorageBoxId id) const noexcept
{
    for (auto& b : m_Boxes)
        if (b.id == id) return &b;
    return nullptr;
}

StorageBoxId StorageSystem::AddBox(const std::string& name, const NF::Vector3& position)
{
    BoxEntry entry;
    entry.id       = m_NextId++;
    entry.name     = name;
    entry.position = position;
    m_Boxes.push_back(std::move(entry));
    return m_Boxes.back().id;
}

NF::Game::Inventory* StorageSystem::GetBox(StorageBoxId id) noexcept
{
    auto* e = FindEntry(id);
    return e ? &e->inventory : nullptr;
}

StorageBoxId StorageSystem::FindNearest(const NF::Vector3& position,
                                         float radius) const noexcept
{
    StorageBoxId best   = kInvalidBoxId;
    float        bestDist = radius * radius;

    for (const auto& b : m_Boxes) {
        const NF::Vector3 diff = b.position - position;
        const float distSq = diff.LengthSq();
        if (distSq < bestDist) {
            bestDist = distSq;
            best     = b.id;
        }
    }
    return best;
}

bool StorageSystem::Deposit(StorageBoxId boxId, NF::Game::Inventory& playerInv,
                            NF::Game::ResourceType type, uint32_t count)
{
    auto* box = FindEntry(boxId);
    if (!box) return false;
    if (!playerInv.HasItem(type, count)) return false;
    if (!box->inventory.AddItem(type, count)) return false;
    playerInv.RemoveItem(type, count);
    return true;
}

bool StorageSystem::Withdraw(StorageBoxId boxId, NF::Game::Inventory& playerInv,
                             NF::Game::ResourceType type, uint32_t count)
{
    auto* box = FindEntry(boxId);
    if (!box) return false;
    if (!box->inventory.HasItem(type, count)) return false;
    if (!playerInv.AddItem(type, count)) return false;
    box->inventory.RemoveItem(type, count);
    return true;
}

} // namespace NF::Game::Gameplay

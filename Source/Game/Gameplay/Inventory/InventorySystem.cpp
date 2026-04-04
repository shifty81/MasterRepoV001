#include "Game/Gameplay/Inventory/InventorySystem.h"
#include <algorithm>

namespace NF::Game::Gameplay {

InventorySystem::ContainerEntry* InventorySystem::FindEntry(ContainerId id) noexcept
{
    for (auto& c : m_Containers)
        if (c.id == id) return &c;
    return nullptr;
}

const InventorySystem::ContainerEntry* InventorySystem::FindEntry(ContainerId id) const noexcept
{
    for (auto& c : m_Containers)
        if (c.id == id) return &c;
    return nullptr;
}

ContainerId InventorySystem::AddContainer(const std::string& name)
{
    ContainerEntry entry;
    entry.id   = m_NextId++;
    entry.name = name;
    m_Containers.push_back(std::move(entry));
    return m_Containers.back().id;
}

NF::Game::Inventory* InventorySystem::GetContainer(ContainerId id) noexcept
{
    auto* e = FindEntry(id);
    return e ? &e->inventory : nullptr;
}

ContainerId InventorySystem::FindContainer(const std::string& name) const noexcept
{
    for (auto& c : m_Containers)
        if (c.name == name) return c.id;
    return kInvalidContainer;
}

bool InventorySystem::Transfer(ContainerId src, ContainerId dst,
                               NF::Game::ResourceType type, uint32_t count)
{
    auto* srcE = FindEntry(src);
    auto* dstE = FindEntry(dst);
    if (!srcE || !dstE) return false;

    if (!srcE->inventory.HasItem(type, count)) return false;
    if (!dstE->inventory.AddItem(type, count)) return false;
    srcE->inventory.RemoveItem(type, count);
    return true;
}

bool InventorySystem::RemoveContainer(ContainerId id)
{
    auto it = std::find_if(m_Containers.begin(), m_Containers.end(),
        [id](const ContainerEntry& e){ return e.id == id; });
    if (it == m_Containers.end()) return false;
    m_Containers.erase(it);
    return true;
}

} // namespace NF::Game::Gameplay

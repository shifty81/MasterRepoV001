#include "Game/Gameplay/Combat/CombatSystem.h"
#include <algorithm>

namespace NF::Game::Gameplay {

CombatSystem::Entry* CombatSystem::FindEntry(uint32_t id) noexcept
{
    for (auto& e : m_Entities)
        if (e.id == id) return &e;
    return nullptr;
}

const CombatSystem::Entry* CombatSystem::FindEntry(uint32_t id) const noexcept
{
    for (auto& e : m_Entities)
        if (e.id == id) return &e;
    return nullptr;
}

void CombatSystem::RegisterEntity(uint32_t entityId, float maxHealth, float armor)
{
    if (FindEntry(entityId)) return; // Already registered.

    Entry entry;
    entry.id                = entityId;
    entry.state.health      = maxHealth;
    entry.state.maxHealth   = maxHealth;
    entry.state.armor       = armor;
    entry.state.alive       = true;
    m_Entities.push_back(entry);
}

bool CombatSystem::UnregisterEntity(uint32_t entityId)
{
    auto it = std::find_if(m_Entities.begin(), m_Entities.end(),
        [entityId](const Entry& e){ return e.id == entityId; });
    if (it == m_Entities.end()) return false;
    m_Entities.erase(it);
    return true;
}

DamageResult CombatSystem::DealDamage(uint32_t entityId, float amount, DamageType type)
{
    Entry* e = FindEntry(entityId);
    if (!e) return DamageResult::NotFound;
    if (!e->state.alive) return DamageResult::AlreadyDead;

    // Only physical damage is reduced by armor.
    float effective = amount;
    if (type == DamageType::Physical)
        effective = std::max(0.f, amount - e->state.armor);

    e->state.health = std::max(0.f, e->state.health - effective);

    if (e->state.health <= 0.f) {
        e->state.alive = false;
        if (m_DeathCb)
            m_DeathCb(entityId);
        return DamageResult::Killed;
    }
    return DamageResult::Hit;
}

bool CombatSystem::Heal(uint32_t entityId, float amount)
{
    Entry* e = FindEntry(entityId);
    if (!e || !e->state.alive) return false;

    e->state.health = std::min(e->state.maxHealth, e->state.health + amount);
    return true;
}

bool CombatSystem::IsAlive(uint32_t entityId) const noexcept
{
    const Entry* e = FindEntry(entityId);
    return e && e->state.alive;
}

CombatState CombatSystem::GetState(uint32_t entityId) const noexcept
{
    const Entry* e = FindEntry(entityId);
    if (e) return e->state;
    return CombatState{0.f, 0.f, 0.f, false};
}

bool CombatSystem::Respawn(uint32_t entityId)
{
    Entry* e = FindEntry(entityId);
    if (!e) return false;
    if (e->state.alive) return false;

    e->state.health = e->state.maxHealth;
    e->state.alive  = true;
    return true;
}

void CombatSystem::Clear() noexcept
{
    m_Entities.clear();
}

} // namespace NF::Game::Gameplay

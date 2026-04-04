#include "Game/Gameplay/Progression/ProgressionSystem.h"

namespace NF::Game::Gameplay {

uint32_t ProgressionSystem::XPThreshold(int level) noexcept
{
    if (level <= 1) return 0;
    // Cumulative XP for level N: kXpPerLevel * (N-1) * N / 2
    // Use uint64_t to avoid overflow for large levels.
    const auto n = static_cast<uint64_t>(level);
    return static_cast<uint32_t>(kXpPerLevel * (n - 1) * n / 2);
}

int ProgressionSystem::UnlockLevel(SkillUnlock skill) noexcept
{
    switch (skill) {
        case SkillUnlock::Tier2Mining:   return 3;
        case SkillUnlock::AdvancedBuild: return 5;
        case SkillUnlock::Tier3Mining:   return 7;
        case SkillUnlock::CombatMaster:  return 10;
        default: return 999;
    }
}

void ProgressionSystem::AddXP(uint32_t amount)
{
    m_TotalXP += amount;
    RecalculateLevel();
}

void ProgressionSystem::RecalculateLevel()
{
    while (XPThreshold(m_Level + 1) <= m_TotalXP) {
        ++m_Level;
        if (m_LevelUpCb)
            m_LevelUpCb(m_Level);
    }
}

uint32_t ProgressionSystem::GetXPForNextLevel() const noexcept
{
    const uint32_t next = XPThreshold(m_Level + 1);
    return (next > m_TotalXP) ? (next - m_TotalXP) : 0;
}

bool ProgressionSystem::IsUnlocked(SkillUnlock skill) const noexcept
{
    return m_Level >= UnlockLevel(skill);
}

void ProgressionSystem::Reset() noexcept
{
    m_TotalXP = 0;
    m_Level   = 1;
}

} // namespace NF::Game::Gameplay

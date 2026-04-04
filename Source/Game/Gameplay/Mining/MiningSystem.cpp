#include "Game/Gameplay/Mining/MiningSystem.h"
#include "Game/Gameplay/Progression/ProgressionSystem.h"
#include "Game/Interaction/ResourceItem.h"

namespace NF::Game::Gameplay {

void MiningSystem::SyncTier(const ProgressionSystem& prog) noexcept
{
    if (prog.IsUnlocked(SkillUnlock::Tier3Mining))
        m_Tier = MiningTier::Tier3;
    else if (prog.IsUnlocked(SkillUnlock::Tier2Mining))
        m_Tier = MiningTier::Tier2;
    else
        m_Tier = MiningTier::Tier1;
}

float MiningSystem::GetEnergyCost() const noexcept
{
    switch (m_Tier) {
        case MiningTier::Tier1: return NF::Game::MiningTool::kEnergyCostPerSwing;
        case MiningTier::Tier2: return NF::Game::MiningTool::kEnergyCostPerSwing * 1.5f;
        case MiningTier::Tier3: return NF::Game::MiningTool::kEnergyCostPerSwing * 2.0f;
        default: return NF::Game::MiningTool::kEnergyCostPerSwing;
    }
}

uint32_t MiningSystem::GetYieldMultiplier() const noexcept
{
    switch (m_Tier) {
        case MiningTier::Tier1: return 1;
        case MiningTier::Tier2: return 2;
        case MiningTier::Tier3: return 3;
        default: return 1;
    }
}

NF::Game::MiningResult MiningSystem::Mine(NF::Game::VoxelEditApi& api,
                                           NF::Game::RigState& rig,
                                           int32_t wx, int32_t wy, int32_t wz,
                                           ProgressionSystem* prog)
{
    NF::Game::MiningResult result = m_Tool.Use(api, rig, wx, wy, wz);

    if (result.voxelReport.result == NF::Game::MineResult::Success) {
        // Scale yield by tier multiplier.
        result.gathered.count *= GetYieldMultiplier();
        ++m_TotalMined;

        // Award XP.
        if (prog)
            prog->AddXP(kXpPerMine);
    }

    return result;
}

} // namespace NF::Game::Gameplay

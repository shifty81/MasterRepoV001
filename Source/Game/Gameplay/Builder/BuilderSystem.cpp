#include "Game/Gameplay/Builder/BuilderSystem.h"
#include "Game/Gameplay/Progression/ProgressionSystem.h"

namespace NF::Game::Gameplay {

BuilderSystem::BuilderSystem()
{
    using RT = NF::Game::ResourceType;

    // 7 recipes matching each non-None resource type.
    m_Recipes[0] = {RT::Stone,   1, 0, false};
    m_Recipes[1] = {RT::Ore,     1, 0, false};
    m_Recipes[2] = {RT::Dirt,    1, 0, false};
    m_Recipes[3] = {RT::Rock,    1, 0, false};
    m_Recipes[4] = {RT::Metal,   2, 1, true };  // Advanced tier, partial refund
    m_Recipes[5] = {RT::Ice,     1, 0, false};
    m_Recipes[6] = {RT::Organic, 1, 0, false};
}

const BuildRecipe* BuilderSystem::FindRecipe(NF::Game::ResourceType type) const noexcept
{
    for (int i = 0; i < kRecipeCount; ++i)
        if (m_Recipes[i].resource == type)
            return &m_Recipes[i];
    return nullptr;
}

BuildResult BuilderSystem::Build(NF::Game::VoxelEditApi& api,
                                 NF::Game::Inventory& inv,
                                 int32_t wx, int32_t wy, int32_t wz,
                                 NF::Game::ResourceType type,
                                 ProgressionSystem* prog)
{
    const BuildRecipe* recipe = FindRecipe(type);
    if (!recipe) return BuildResult::InvalidRecipe;

    // Tier-gate check: Metal requires AdvancedBuild unlock.
    if (recipe->requiresAdvancedBuild) {
        if (!prog || !prog->IsUnlocked(SkillUnlock::AdvancedBuild))
            return BuildResult::LockedTier;
    }

    // Resource check.
    if (!inv.HasItem(type, recipe->cost))
        return BuildResult::InsufficientResources;

    // Place voxel.
    NF::Game::VoxelId voxelId = NF::Game::ResourceToVoxelId(type);
    NF::Game::SetResult setR = api.SetVoxel(wx, wy, wz, voxelId);
    if (setR == NF::Game::SetResult::OutOfBounds)
        return BuildResult::OutOfBounds;

    // Deduct resources.
    inv.RemoveItem(type, recipe->cost);
    ++m_TotalBuilt;

    // Award XP.
    if (prog)
        prog->AddXP(kXpPerBuild);

    return BuildResult::Success;
}

TeardownResult BuilderSystem::Teardown(NF::Game::VoxelEditApi& api,
                                       NF::Game::Inventory& inv,
                                       int32_t wx, int32_t wy, int32_t wz)
{
    // Read what's there first.
    NF::Game::VoxelId existing = api.GetVoxel(wx, wy, wz);
    if (existing == static_cast<NF::Game::VoxelId>(NF::Game::VoxelType::Air))
        return TeardownResult::EmptyCell;

    // Mine/remove the voxel.
    NF::Game::MineReport report = api.Mine(wx, wy, wz, 0);
    if (report.result == NF::Game::MineResult::OutOfBounds)
        return TeardownResult::OutOfBounds;
    if (report.result != NF::Game::MineResult::Success)
        return TeardownResult::EmptyCell;

    // Refund resources based on recipe (if one exists).
    NF::Game::ResourceType resType = NF::Game::VoxelTypeToResource(report.extractedType);
    const BuildRecipe* recipe = FindRecipe(resType);
    uint32_t refund = recipe ? recipe->refund : 0;
    // If recipe has no explicit refund, give back 1 unit of the resource.
    if (refund == 0 && resType != NF::Game::ResourceType::None)
        refund = 1;

    if (refund > 0 && resType != NF::Game::ResourceType::None)
        inv.AddItem(resType, refund);

    ++m_TotalTornDown;
    return TeardownResult::Success;
}

} // namespace NF::Game::Gameplay

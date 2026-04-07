#include "Game/Gameplay/Manufacturing/ManufacturingQueue.h"
#include <algorithm>

namespace NF::Game::Gameplay {

// ---------------------------------------------------------------------------
// Construction — register built-in recipes
// ---------------------------------------------------------------------------

ManufacturingQueue::ManufacturingQueue()
{
    using RT = NF::Game::ResourceType;

    // Recipe: Metal (5 Stone + 3 Ore, 8 s)
    {
        CraftRecipe& r = m_Recipes[m_RecipeCount++];
        r.output           = RT::Metal;
        r.outputCount      = 1;
        r.duration         = 8.f;
        r.ingredientCount  = 2;
        r.ingredients[0]   = {RT::Stone, 5};
        r.ingredients[1]   = {RT::Ore,   3};
    }

    // Recipe: Rock (2 Stone, 3 s)
    {
        CraftRecipe& r = m_Recipes[m_RecipeCount++];
        r.output           = RT::Rock;
        r.outputCount      = 2;
        r.duration         = 3.f;
        r.ingredientCount  = 1;
        r.ingredients[0]   = {RT::Stone, 2};
    }

    // Recipe: Organic (2 Ice + 1 Dirt, 5 s)
    {
        CraftRecipe& r = m_Recipes[m_RecipeCount++];
        r.output           = RT::Organic;
        r.outputCount      = 1;
        r.duration         = 5.f;
        r.ingredientCount  = 2;
        r.ingredients[0]   = {RT::Ice,  2};
        r.ingredients[1]   = {RT::Dirt, 1};
    }
}

// ---------------------------------------------------------------------------
// Recipe queries
// ---------------------------------------------------------------------------

int ManufacturingQueue::RecipeCount() const noexcept
{
    return m_RecipeCount;
}

const CraftRecipe* ManufacturingQueue::FindRecipe(NF::Game::ResourceType output) const noexcept
{
    for (int i = 0; i < m_RecipeCount; ++i)
        if (m_Recipes[i].output == output)
            return &m_Recipes[i];
    return nullptr;
}

// ---------------------------------------------------------------------------
// Queue management
// ---------------------------------------------------------------------------

JobId ManufacturingQueue::Enqueue(NF::Game::ResourceType output, NF::Game::Inventory& inv)
{
    const CraftRecipe* recipe = FindRecipe(output);
    if (!recipe) return kInvalidJobId;

    // Verify we have all ingredients before touching the inventory.
    for (int i = 0; i < recipe->ingredientCount; ++i)
    {
        const auto& ing = recipe->ingredients[i];
        if (!inv.HasItem(ing.type, ing.count))
            return kInvalidJobId;
    }

    // Deduct ingredients.
    for (int i = 0; i < recipe->ingredientCount; ++i)
    {
        const auto& ing = recipe->ingredients[i];
        inv.RemoveItem(ing.type, ing.count);
    }

    Job job;
    job.id      = m_NextId++;
    job.recipe  = *recipe;
    job.elapsed = 0.f;

    if (!m_HasActive)
    {
        m_Active    = job;
        m_HasActive = true;
    }
    else
    {
        m_Pending.push_back(job);
    }

    return job.id;
}

void ManufacturingQueue::Tick(float dt) noexcept
{
    if (m_HasActive && m_Active.elapsed < m_Active.recipe.duration)
        m_Active.elapsed += dt;
}

// ---------------------------------------------------------------------------
// Output collection
// ---------------------------------------------------------------------------

bool ManufacturingQueue::HasCompletedJob() const noexcept
{
    return m_HasActive && (m_Active.elapsed >= m_Active.recipe.duration);
}

bool ManufacturingQueue::CollectOutput(NF::Game::Inventory& inv)
{
    if (!HasCompletedJob()) return false;

    const auto& r = m_Active.recipe;
    inv.AddItem(r.output, r.outputCount);

    m_HasActive = false;
    StartNext();
    return true;
}

// ---------------------------------------------------------------------------
// State queries
// ---------------------------------------------------------------------------

bool ManufacturingQueue::IsIdle() const noexcept
{
    return !m_HasActive && m_Pending.empty();
}

int ManufacturingQueue::PendingCount() const noexcept
{
    return (m_HasActive ? 1 : 0) + static_cast<int>(m_Pending.size());
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void ManufacturingQueue::StartNext() noexcept
{
    if (!m_Pending.empty())
    {
        m_Active    = m_Pending.front();
        m_Pending.pop_front();
        m_HasActive = true;
    }
}

} // namespace NF::Game::Gameplay

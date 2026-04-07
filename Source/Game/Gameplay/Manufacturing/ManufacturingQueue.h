#pragma once
#include "Game/Interaction/ResourceItem.h"
#include "Game/Interaction/Inventory.h"
#include <cstdint>
#include <deque>

namespace NF::Game::Gameplay {

using JobId = uint32_t;
static constexpr JobId kInvalidJobId = 0u;

static constexpr int kMaxIngredients = 4; ///< Max distinct ingredient types per recipe.

/// @brief A single ingredient requirement in a crafting recipe.
struct CraftIngredient {
    NF::Game::ResourceType type{NF::Game::ResourceType::None};
    uint32_t               count{0};
};

/// @brief A crafting recipe: required ingredients + output + time cost.
struct CraftRecipe {
    NF::Game::ResourceType output{NF::Game::ResourceType::None};
    uint32_t               outputCount{1};
    float                  duration{5.f};   ///< Seconds to complete one job.
    int                    ingredientCount{0};
    CraftIngredient        ingredients[kMaxIngredients]{};
};

/// @brief Queued crafting / manufacturing pipeline.
///
/// Recipes are built-in at construction time.  At most one job is active at a
/// time; additional enqueued jobs start automatically when the active job is
/// collected.
class ManufacturingQueue {
public:
    ManufacturingQueue();
    ~ManufacturingQueue() = default;

    // -------------------------------------------------------------------------
    // Recipe queries
    // -------------------------------------------------------------------------

    /// @brief Number of built-in recipes.
    [[nodiscard]] int RecipeCount() const noexcept;

    /// @brief Find the recipe that produces @p output, or nullptr if none.
    [[nodiscard]] const CraftRecipe* FindRecipe(NF::Game::ResourceType output) const noexcept;

    // -------------------------------------------------------------------------
    // Queue management
    // -------------------------------------------------------------------------

    /// @brief Enqueue a new job for @p output.
    ///        Deducts required ingredients from @p inv immediately.
    /// @return Job ID on success, kInvalidJobId if recipe not found or
    ///         insufficient ingredients.
    JobId Enqueue(NF::Game::ResourceType output, NF::Game::Inventory& inv);

    /// @brief Advance the active job by @p dt seconds.
    void Tick(float dt) noexcept;

    // -------------------------------------------------------------------------
    // Output collection
    // -------------------------------------------------------------------------

    /// @brief Returns true if the active job has finished processing.
    [[nodiscard]] bool HasCompletedJob() const noexcept;

    /// @brief Deposit output of the completed job into @p inv and start the
    ///        next pending job (if any).
    /// @return True if a completed job was collected.
    bool CollectOutput(NF::Game::Inventory& inv);

    // -------------------------------------------------------------------------
    // State queries
    // -------------------------------------------------------------------------

    /// @brief True when there are no active or pending jobs.
    [[nodiscard]] bool IsIdle() const noexcept;

    /// @brief Number of jobs currently in the queue (including active job).
    [[nodiscard]] int PendingCount() const noexcept;

private:
    struct Job {
        JobId       id{kInvalidJobId};
        CraftRecipe recipe{};
        float       elapsed{0.f};
    };

    static constexpr int kMaxRecipes = 8;
    CraftRecipe  m_Recipes[kMaxRecipes]{};
    int          m_RecipeCount{0};

    Job          m_Active{};
    bool         m_HasActive{false};
    std::deque<Job> m_Pending{};
    JobId        m_NextId{1};

    void StartNext() noexcept;
};

} // namespace NF::Game::Gameplay

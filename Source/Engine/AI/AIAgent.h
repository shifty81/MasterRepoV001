#pragma once
// AIAgent.h — Runtime AI agent state.
//
// An AIAgent represents a single AI-controlled entity in the game world.
// It holds the agent's perception state, goal queue, and reference to
// its active BehaviorTree or GraphVM-driven behavior graph.

#include "Engine/ECS/World.h"
#include "Engine/NodeGraph/GraphVM.h"
#include <memory>
#include <string>
#include <vector>

namespace NF {

// Forward declarations
class BehaviorTree;

/// @brief Priority levels for AI goal ordering.
enum class GoalPriority : uint8_t {
    Low    = 0,
    Normal = 1,
    High   = 2,
    Urgent = 3,
};

/// @brief A single goal in the agent's goal queue.
struct AIGoal {
    std::string   id;                   ///< Unique goal identifier (e.g., "seek_cover")
    GoalPriority  priority{GoalPriority::Normal};
    float         desirability{0.5f};   ///< Utility score [0, 1]
    bool          active{false};
};

/// @brief Runtime AI agent.
///
/// Owned by the AISystem; referenced by entity via ECS component.
class AIAgent {
public:
    explicit AIAgent(EntityId entityId) noexcept : m_EntityId(entityId) {}

    [[nodiscard]] EntityId GetEntityId() const noexcept { return m_EntityId; }

    // ---- Goals ---------------------------------------------------------------

    void AddGoal(AIGoal goal);
    void RemoveGoal(const std::string& goalId);
    [[nodiscard]] const AIGoal* GetActiveGoal() const noexcept;

    // ---- Behavior graph ------------------------------------------------------

    void SetBehaviorGraph(std::shared_ptr<CompiledGraph> graph);
    [[nodiscard]] ExecutionContext*       GetGraphContext()       noexcept { return m_GraphCtx.get(); }
    [[nodiscard]] const ExecutionContext* GetGraphContext() const noexcept { return m_GraphCtx.get(); }

    // ---- Perception ----------------------------------------------------------

    /// @brief List of entity IDs currently perceived by this agent.
    std::vector<EntityId> visibleEntities;
    std::vector<EntityId> audibleEntities;

    // ---- Blackboard shorthand ------------------------------------------------

    void SetVar(const std::string& key, GraphValue value);
    [[nodiscard]] GraphValue GetVar(const std::string& key) const;

private:
    EntityId                            m_EntityId;
    std::vector<AIGoal>                 m_Goals;
    std::shared_ptr<CompiledGraph>      m_BehaviorGraph;
    std::unique_ptr<ExecutionContext>   m_GraphCtx;
};

} // namespace NF

#pragma once
// AISystem.h — Top-level AI system tick and agent registry.
//
// AISystem owns all active AIAgents, runs the GraphVM tick on behavior
// graphs each frame, and exposes the FactionManager.

#include "Engine/AI/AIAgent.h"
#include "Engine/AI/Blackboard.h"
#include "Engine/AI/FactionManager.h"
#include "Engine/NodeGraph/GraphVM.h"
#include <memory>
#include <unordered_map>

namespace NF {

/// @brief Central AI subsystem.
///
/// Lifecycle:
///   1. AISystem::Init()  — register tools with GraphVM
///   2. Each frame: AISystem::Tick(dt)  — advance all agents
///   3. AISystem::Shutdown()
class AISystem {
public:
    AISystem();
    ~AISystem() = default;

    // ---- Lifecycle -----------------------------------------------------------

    void Init();
    void Tick(float dt);
    void Shutdown();

    // ---- Agent management ----------------------------------------------------

    /// @brief Register an agent for this entity.  Replaces any existing agent.
    AIAgent& RegisterAgent(EntityId entityId);

    /// @brief Unregister and destroy the agent for this entity.
    void UnregisterAgent(EntityId entityId);

    /// @brief Return the agent for this entity, or nullptr if not registered.
    [[nodiscard]] AIAgent*       GetAgent(EntityId entityId)       noexcept;
    [[nodiscard]] const AIAgent* GetAgent(EntityId entityId) const noexcept;

    // ---- Shared services -----------------------------------------------------

    [[nodiscard]] FactionManager&       GetFactionManager()       noexcept { return m_FactionManager; }
    [[nodiscard]] const FactionManager& GetFactionManager() const noexcept { return m_FactionManager; }

    [[nodiscard]] GraphVM&       GetGraphVM()       noexcept { return m_GraphVM; }
    [[nodiscard]] const GraphVM& GetGraphVM() const noexcept { return m_GraphVM; }

    /// @brief Shared global blackboard (faction-wide data, world events, etc.).
    [[nodiscard]] Blackboard&       GetGlobalBlackboard()       noexcept { return m_GlobalBlackboard; }
    [[nodiscard]] const Blackboard& GetGlobalBlackboard() const noexcept { return m_GlobalBlackboard; }

private:
    std::unordered_map<EntityId, std::unique_ptr<AIAgent>> m_Agents;
    FactionManager  m_FactionManager;
    GraphVM         m_GraphVM;
    Blackboard      m_GlobalBlackboard;

    void RegisterBuiltinTools();
    void TickAgent(AIAgent& agent, float dt);
};

} // namespace NF

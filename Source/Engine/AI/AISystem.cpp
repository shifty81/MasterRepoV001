// AISystem.cpp — AI subsystem implementation.
#include "Engine/AI/AISystem.h"
#include "Engine/AI/UtilitySelector.h"
#include "Core/Logging/Log.h"
#include <algorithm>

namespace NF {

// ---------------------------------------------------------------------------
// AIAgent implementation
// ---------------------------------------------------------------------------

void AIAgent::AddGoal(AIGoal goal)
{
    // Remove any existing goal with the same id
    RemoveGoal(goal.id);
    m_Goals.push_back(std::move(goal));
    // Keep sorted by priority descending
    std::sort(m_Goals.begin(), m_Goals.end(),
        [](const AIGoal& a, const AIGoal& b) {
            return static_cast<uint8_t>(a.priority) > static_cast<uint8_t>(b.priority);
        });
}

void AIAgent::RemoveGoal(const std::string& goalId)
{
    m_Goals.erase(
        std::remove_if(m_Goals.begin(), m_Goals.end(),
            [&](const AIGoal& g) { return g.id == goalId; }),
        m_Goals.end());
}

const AIGoal* AIAgent::GetActiveGoal() const noexcept
{
    for (const auto& g : m_Goals)
        if (g.active) return &g;
    return m_Goals.empty() ? nullptr : &m_Goals.front();
}

void AIAgent::SetBehaviorGraph(std::shared_ptr<CompiledGraph> graph)
{
    m_BehaviorGraph = std::move(graph);
    if (m_BehaviorGraph) {
        m_GraphCtx = std::make_unique<ExecutionContext>();
        m_GraphCtx->graph    = m_BehaviorGraph.get();
        m_GraphCtx->entityId = static_cast<uint64_t>(m_EntityId);
    } else {
        m_GraphCtx.reset();
    }
}

void AIAgent::SetVar(const std::string& key, GraphValue value)
{
    if (m_GraphCtx)
        m_GraphCtx->blackboard[key] = std::move(value);
}

GraphValue AIAgent::GetVar(const std::string& key) const
{
    if (m_GraphCtx) {
        auto it = m_GraphCtx->blackboard.find(key);
        if (it != m_GraphCtx->blackboard.end()) return it->second;
    }
    return {};
}

// ---------------------------------------------------------------------------
// FactionManager implementation
// ---------------------------------------------------------------------------

std::string FactionManager::MakeKey(const std::string& a, const std::string& b)
{
    return (a < b) ? (a + "|" + b) : (b + "|" + a);
}

int32_t FactionManager::GetReputation(const std::string& a, const std::string& b) const noexcept
{
    auto it = m_Reputation.find(MakeKey(a, b));
    return it != m_Reputation.end() ? it->second : kNeutral;
}

bool FactionManager::IsHostile(const std::string& a, const std::string& b) const noexcept
{
    return GetReputation(a, b) <= kHostileThresh;
}

bool FactionManager::IsFriendly(const std::string& a, const std::string& b) const noexcept
{
    return GetReputation(a, b) >= kFriendlyThresh;
}

void FactionManager::ApplyReputation(const std::string& a, const std::string& b,
                                     int32_t delta, bool clamp)
{
    auto& val = m_Reputation[MakeKey(a, b)];
    val += delta;
    if (clamp) {
        if (val < kMinReputation) val = kMinReputation;
        if (val > kMaxReputation) val = kMaxReputation;
    }
}

void FactionManager::SetReputation(const std::string& a, const std::string& b, int32_t value)
{
    m_Reputation[MakeKey(a, b)] = value;
}

void FactionManager::SetAlliance(const std::string& a, const std::string& b)
{
    SetReputation(a, b, kFriendlyThresh + 100);
}

void FactionManager::SetWar(const std::string& a, const std::string& b)
{
    SetReputation(a, b, kHostileThresh - 100);
}

void FactionManager::Clear()
{
    m_Reputation.clear();
}

// ---------------------------------------------------------------------------
// UtilitySelector implementation
// ---------------------------------------------------------------------------

void UtilitySelector::AddAction(UtilityAction action)
{
    RemoveAction(action.id);
    m_Actions.push_back(std::move(action));
}

void UtilitySelector::RemoveAction(const std::string& id)
{
    m_Actions.erase(
        std::remove_if(m_Actions.begin(), m_Actions.end(),
            [&](const UtilityAction& a) { return a.id == id; }),
        m_Actions.end());
}

void UtilitySelector::ClearActions()
{
    m_Actions.clear();
}

std::string UtilitySelector::SelectBest(const AIAgent& agent) const
{
    float bestScore = -1.f;
    std::string bestId;
    for (const auto& action : m_Actions) {
        if (!action.scoreFunc) continue;
        float score = action.scoreFunc(agent);
        if (score > bestScore) {
            bestScore = score;
            bestId    = action.id;
        }
    }
    return bestId;
}

std::string UtilitySelector::SelectAndExecute(AIAgent& agent)
{
    float bestScore = -1.f;
    UtilityAction* best = nullptr;
    for (auto& action : m_Actions) {
        if (!action.scoreFunc) continue;
        float score = action.scoreFunc(agent);
        if (score > bestScore) {
            bestScore = score;
            best      = &action;
        }
    }
    if (best && best->executeFunc) {
        best->executeFunc(agent);
        return best->id;
    }
    return {};
}

// ---------------------------------------------------------------------------
// AISystem implementation
// ---------------------------------------------------------------------------

AISystem::AISystem() = default;

void AISystem::Init()
{
    RegisterBuiltinTools();
    NF::Logger::Log(NF::LogLevel::Info, "AI", "AISystem initialised");
}

void AISystem::Shutdown()
{
    m_Agents.clear();
    NF::Logger::Log(NF::LogLevel::Info, "AI", "AISystem shut down");
}

AIAgent& AISystem::RegisterAgent(EntityId entityId)
{
    auto& ptr = m_Agents[entityId];
    ptr = std::make_unique<AIAgent>(entityId);
    return *ptr;
}

void AISystem::UnregisterAgent(EntityId entityId)
{
    m_Agents.erase(entityId);
}

AIAgent* AISystem::GetAgent(EntityId entityId) noexcept
{
    auto it = m_Agents.find(entityId);
    return it != m_Agents.end() ? it->second.get() : nullptr;
}

const AIAgent* AISystem::GetAgent(EntityId entityId) const noexcept
{
    auto it = m_Agents.find(entityId);
    return it != m_Agents.end() ? it->second.get() : nullptr;
}

void AISystem::Tick(float dt)
{
    for (auto& [id, agent] : m_Agents) {
        if (agent) TickAgent(*agent, dt);
    }
    (void)dt;
}

void AISystem::TickAgent(AIAgent& agent, float /*dt*/)
{
    ExecutionContext* ctx = agent.GetGraphContext();
    if (!ctx || ctx->finished)
        return;

    if (ctx->suspended) {
        m_GraphVM.Resume(*ctx, 500);
    } else {
        m_GraphVM.Run(*ctx, 500);
    }
}

void AISystem::RegisterBuiltinTools()
{
    // Log tool — useful for debugging graphs
    m_GraphVM.RegisterTool("Log", [](ExecutionContext& ctx, std::string_view) -> GraphValue {
        if (!ctx.stack.empty()) {
            auto v = ctx.stack.back();
            ctx.stack.pop_back();
            if (auto* s = std::get_if<std::string>(&v))
                NF::Logger::Log(NF::LogLevel::Info, "GraphVM", *s);
        }
        return {};
    });

    // SetBlackboard(key, value) — pops value then key from stack
    m_GraphVM.RegisterTool("SetBlackboard", [](ExecutionContext& ctx, std::string_view) -> GraphValue {
        if (ctx.stack.size() >= 2) {
            GraphValue val = ctx.stack.back(); ctx.stack.pop_back();
            GraphValue key = ctx.stack.back(); ctx.stack.pop_back();
            if (auto* k = std::get_if<std::string>(&key))
                ctx.blackboard[*k] = std::move(val);
        }
        return {};
    });

    // GetBlackboard(key) — pops key, pushes value
    m_GraphVM.RegisterTool("GetBlackboard", [](ExecutionContext& ctx, std::string_view) -> GraphValue {
        if (!ctx.stack.empty()) {
            GraphValue key = ctx.stack.back(); ctx.stack.pop_back();
            if (auto* k = std::get_if<std::string>(&key)) {
                auto it = ctx.blackboard.find(*k);
                return it != ctx.blackboard.end() ? it->second : GraphValue{};
            }
        }
        return {};
    });
}

} // namespace NF

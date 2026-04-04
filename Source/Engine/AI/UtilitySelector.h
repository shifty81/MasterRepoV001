#pragma once
// UtilitySelector.h — Utility-theory action selection for AI agents.
//
// Each candidate action has a score function.  UtilitySelector evaluates
// all registered actions and returns the one with the highest score.
// Used by AIAgent when no behavior graph is assigned.

#include <functional>
#include <string>
#include <vector>

namespace NF {

class AIAgent;  // forward declaration

/// @brief A single candidate action with a scoring function.
struct UtilityAction {
    std::string                          id;
    std::function<float(const AIAgent&)> scoreFunc; ///< Returns [0, 1]
    std::function<void(AIAgent&)>        executeFunc;
};

/// @brief Evaluates a set of utility actions and picks the best one.
class UtilitySelector {
public:
    void AddAction(UtilityAction action);
    void RemoveAction(const std::string& id);
    void ClearActions();

    /// @brief Evaluate all actions against the agent and execute the best one.
    /// @return ID of the selected action, or empty string if none.
    std::string SelectAndExecute(AIAgent& agent);

    /// @brief Evaluate only — returns the best action ID without executing.
    [[nodiscard]] std::string SelectBest(const AIAgent& agent) const;

private:
    std::vector<UtilityAction> m_Actions;
};

} // namespace NF

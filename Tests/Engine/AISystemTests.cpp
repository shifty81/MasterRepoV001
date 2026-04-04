/// @file AISystemTests.cpp — Unit tests for Blackboard, FactionManager,
///       AIAgent, UtilitySelector, and AISystem.
#include <catch2/catch_test_macros.hpp>
#include "Engine/AI/Blackboard.h"
#include "Engine/AI/FactionManager.h"
#include "Engine/AI/AIAgent.h"
#include "Engine/AI/UtilitySelector.h"
#include "Engine/AI/AISystem.h"

using namespace NF;

// ---------------------------------------------------------------------------
// Blackboard
// ---------------------------------------------------------------------------

TEST_CASE("Blackboard::Set and Get round-trip a bool", "[ai][blackboard]")
{
    Blackboard bb;
    bb.Set("alive", GraphValue{true});
    REQUIRE(bb.Has("alive"));
    REQUIRE(bb.GetBool("alive") == true);
}

TEST_CASE("Blackboard::Set and Get round-trip an int", "[ai][blackboard]")
{
    Blackboard bb;
    bb.Set("health", GraphValue{int32_t{100}});
    REQUIRE(bb.GetInt("health") == 100);
}

TEST_CASE("Blackboard::Set and Get round-trip a float", "[ai][blackboard]")
{
    Blackboard bb;
    bb.Set("speed", GraphValue{1.5f});
    REQUIRE(bb.GetFloat("speed") == 1.5f);
}

TEST_CASE("Blackboard::Set and Get round-trip a string", "[ai][blackboard]")
{
    Blackboard bb;
    bb.Set("name", GraphValue{std::string{"TestAgent"}});
    REQUIRE(bb.GetString("name") == "TestAgent");
}

TEST_CASE("Blackboard::Has returns false for missing key", "[ai][blackboard]")
{
    Blackboard bb;
    REQUIRE(!bb.Has("missing"));
}

TEST_CASE("Blackboard::GetBool returns default for missing key", "[ai][blackboard]")
{
    Blackboard bb;
    REQUIRE(bb.GetBool("x", false) == false);
    REQUIRE(bb.GetBool("x", true)  == true);
}

TEST_CASE("Blackboard::GetInt returns default for missing key", "[ai][blackboard]")
{
    Blackboard bb;
    REQUIRE(bb.GetInt("x", 42) == 42);
}

TEST_CASE("Blackboard::GetFloat returns default for missing key", "[ai][blackboard]")
{
    Blackboard bb;
    REQUIRE(bb.GetFloat("x", 3.5f) == 3.5f);
}

TEST_CASE("Blackboard::GetString returns default for missing key", "[ai][blackboard]")
{
    Blackboard bb;
    REQUIRE(bb.GetString("x", "def") == "def");
}

TEST_CASE("Blackboard::Clear removes a key", "[ai][blackboard]")
{
    Blackboard bb;
    bb.Set("k", GraphValue{int32_t{1}});
    bb.Clear("k");
    REQUIRE(!bb.Has("k"));
}

TEST_CASE("Blackboard::ClearAll removes all keys", "[ai][blackboard]")
{
    Blackboard bb;
    bb.Set("a", GraphValue{int32_t{1}});
    bb.Set("b", GraphValue{int32_t{2}});
    bb.ClearAll();
    REQUIRE(!bb.Has("a"));
    REQUIRE(!bb.Has("b"));
}

TEST_CASE("Blackboard::Get returns null GraphValue for missing key", "[ai][blackboard]")
{
    Blackboard bb;
    const GraphValue& v = bb.Get("missing");
    REQUIRE(IsNull(v));
}

// ---------------------------------------------------------------------------
// FactionManager
// ---------------------------------------------------------------------------

TEST_CASE("FactionManager::GetReputation returns neutral by default", "[ai][faction]")
{
    FactionManager fm;
    REQUIRE(fm.GetReputation("A", "B") == FactionManager::kNeutral);
}

TEST_CASE("FactionManager::ApplyReputation adds delta", "[ai][faction]")
{
    FactionManager fm;
    fm.ApplyReputation("A", "B", 200);
    REQUIRE(fm.GetReputation("A", "B") == 200);
}

TEST_CASE("FactionManager::ApplyReputation is symmetric (A|B == B|A)", "[ai][faction]")
{
    FactionManager fm;
    fm.ApplyReputation("Alpha", "Beta", 300);
    REQUIRE(fm.GetReputation("Alpha", "Beta") == fm.GetReputation("Beta", "Alpha"));
}

TEST_CASE("FactionManager::ApplyReputation clamps to max", "[ai][faction]")
{
    FactionManager fm;
    fm.ApplyReputation("A", "B", 2000, true);
    REQUIRE(fm.GetReputation("A", "B") == FactionManager::kMaxReputation);
}

TEST_CASE("FactionManager::ApplyReputation clamps to min", "[ai][faction]")
{
    FactionManager fm;
    fm.ApplyReputation("A", "B", -2000, true);
    REQUIRE(fm.GetReputation("A", "B") == FactionManager::kMinReputation);
}

TEST_CASE("FactionManager::SetReputation sets exact value", "[ai][faction]")
{
    FactionManager fm;
    fm.SetReputation("A", "B", 750);
    REQUIRE(fm.GetReputation("A", "B") == 750);
}

TEST_CASE("FactionManager::IsHostile returns true below threshold", "[ai][faction]")
{
    FactionManager fm;
    fm.SetReputation("A", "B", -600);
    REQUIRE(fm.IsHostile("A", "B"));
}

TEST_CASE("FactionManager::IsHostile returns false above threshold", "[ai][faction]")
{
    FactionManager fm;
    fm.SetReputation("A", "B", 0);
    REQUIRE(!fm.IsHostile("A", "B"));
}

TEST_CASE("FactionManager::IsFriendly returns true above threshold", "[ai][faction]")
{
    FactionManager fm;
    fm.SetReputation("A", "B", 600);
    REQUIRE(fm.IsFriendly("A", "B"));
}

TEST_CASE("FactionManager::IsFriendly returns false below threshold", "[ai][faction]")
{
    FactionManager fm;
    fm.SetReputation("A", "B", 0);
    REQUIRE(!fm.IsFriendly("A", "B"));
}

TEST_CASE("FactionManager::SetAlliance marks factions as friendly", "[ai][faction]")
{
    FactionManager fm;
    fm.SetAlliance("Forge", "Nomads");
    REQUIRE(fm.IsFriendly("Forge", "Nomads"));
}

TEST_CASE("FactionManager::SetWar marks factions as hostile", "[ai][faction]")
{
    FactionManager fm;
    fm.SetWar("Forge", "Raiders");
    REQUIRE(fm.IsHostile("Forge", "Raiders"));
}

TEST_CASE("FactionManager::Clear resets all reputations", "[ai][faction]")
{
    FactionManager fm;
    fm.SetReputation("A", "B", 500);
    fm.Clear();
    REQUIRE(fm.GetReputation("A", "B") == FactionManager::kNeutral);
}

// ---------------------------------------------------------------------------
// AIAgent
// ---------------------------------------------------------------------------

TEST_CASE("AIAgent::GetEntityId returns the constructed id", "[ai][agent]")
{
    AIAgent agent{42};
    REQUIRE(agent.GetEntityId() == 42);
}

TEST_CASE("AIAgent::AddGoal and GetActiveGoal return highest-priority goal", "[ai][agent]")
{
    AIAgent agent{1};
    agent.AddGoal({ "low",    GoalPriority::Low });
    agent.AddGoal({ "urgent", GoalPriority::Urgent });
    agent.AddGoal({ "normal", GoalPriority::Normal });

    const AIGoal* active = agent.GetActiveGoal();
    REQUIRE(active != nullptr);
    REQUIRE(active->id == "urgent");
}

TEST_CASE("AIAgent::RemoveGoal removes the goal by id", "[ai][agent]")
{
    AIAgent agent{1};
    agent.AddGoal({ "patrol", GoalPriority::Normal });
    agent.AddGoal({ "attack", GoalPriority::High });
    agent.RemoveGoal("attack");

    const AIGoal* active = agent.GetActiveGoal();
    REQUIRE(active != nullptr);
    REQUIRE(active->id == "patrol");
}

TEST_CASE("AIAgent::AddGoal deduplicates by id", "[ai][agent]")
{
    AIAgent agent{1};
    agent.AddGoal({ "seek", GoalPriority::Normal });
    agent.AddGoal({ "seek", GoalPriority::High }); // replaces existing

    // Only one goal should exist; its priority is High.
    const AIGoal* active = agent.GetActiveGoal();
    REQUIRE(active != nullptr);
    REQUIRE(active->priority == GoalPriority::High);
}

TEST_CASE("AIAgent::GetActiveGoal returns null when no goals exist", "[ai][agent]")
{
    AIAgent agent{1};
    REQUIRE(agent.GetActiveGoal() == nullptr);
}

TEST_CASE("AIAgent::SetVar and GetVar round-trip via graph context", "[ai][agent]")
{
    AIAgent agent{1};
    // Need a behavior graph to have a context
    auto graph = std::make_shared<CompiledGraph>();
    graph->name = "test";
    agent.SetBehaviorGraph(graph);

    agent.SetVar("hp", GraphValue{int32_t{80}});
    GraphValue v = agent.GetVar("hp");
    REQUIRE(std::get<int32_t>(v) == 80);
}

TEST_CASE("AIAgent::GetVar returns null when no graph context", "[ai][agent]")
{
    AIAgent agent{1};
    REQUIRE(IsNull(agent.GetVar("anything")));
}

TEST_CASE("AIAgent::SetBehaviorGraph sets graph context", "[ai][agent]")
{
    AIAgent agent{1};
    REQUIRE(agent.GetGraphContext() == nullptr);

    auto graph = std::make_shared<CompiledGraph>();
    graph->name = "behavior";
    agent.SetBehaviorGraph(graph);

    REQUIRE(agent.GetGraphContext() != nullptr);
    REQUIRE(agent.GetGraphContext()->graph == graph.get());
}

TEST_CASE("AIAgent::SetBehaviorGraph with nullptr clears context", "[ai][agent]")
{
    AIAgent agent{1};
    auto graph = std::make_shared<CompiledGraph>();
    agent.SetBehaviorGraph(graph);
    agent.SetBehaviorGraph(nullptr);

    REQUIRE(agent.GetGraphContext() == nullptr);
}

// ---------------------------------------------------------------------------
// UtilitySelector
// ---------------------------------------------------------------------------

TEST_CASE("UtilitySelector::SelectBest returns empty string with no actions", "[ai][utility]")
{
    UtilitySelector sel;
    AIAgent agent{1};
    REQUIRE(sel.SelectBest(agent).empty());
}

TEST_CASE("UtilitySelector::SelectBest picks highest-scoring action", "[ai][utility]")
{
    UtilitySelector sel;
    sel.AddAction({ "low",  [](const AIAgent&) { return 0.1f; }, nullptr });
    sel.AddAction({ "high", [](const AIAgent&) { return 0.9f; }, nullptr });
    sel.AddAction({ "mid",  [](const AIAgent&) { return 0.5f; }, nullptr });

    AIAgent agent{1};
    REQUIRE(sel.SelectBest(agent) == "high");
}

TEST_CASE("UtilitySelector::SelectAndExecute executes the best action", "[ai][utility]")
{
    UtilitySelector sel;
    bool executed = false;
    sel.AddAction({
        "best",
        [](const AIAgent&) { return 1.0f; },
        [&executed](AIAgent&) { executed = true; }
    });
    sel.AddAction({
        "other",
        [](const AIAgent&) { return 0.2f; },
        nullptr
    });

    AIAgent agent{1};
    std::string chosen = sel.SelectAndExecute(agent);
    REQUIRE(chosen == "best");
    REQUIRE(executed);
}

TEST_CASE("UtilitySelector::RemoveAction removes by id", "[ai][utility]")
{
    UtilitySelector sel;
    sel.AddAction({ "alpha", [](const AIAgent&) { return 1.0f; }, nullptr });
    sel.AddAction({ "beta",  [](const AIAgent&) { return 0.5f; }, nullptr });
    sel.RemoveAction("alpha");

    AIAgent agent{1};
    REQUIRE(sel.SelectBest(agent) == "beta");
}

TEST_CASE("UtilitySelector::ClearActions leaves no selectable actions", "[ai][utility]")
{
    UtilitySelector sel;
    sel.AddAction({ "x", [](const AIAgent&) { return 1.0f; }, nullptr });
    sel.ClearActions();

    AIAgent agent{1};
    REQUIRE(sel.SelectBest(agent).empty());
}

// ---------------------------------------------------------------------------
// AISystem
// ---------------------------------------------------------------------------

TEST_CASE("AISystem::Init and Shutdown do not throw", "[ai][system]")
{
    AISystem sys;
    REQUIRE_NOTHROW(sys.Init());
    REQUIRE_NOTHROW(sys.Shutdown());
}

TEST_CASE("AISystem::RegisterAgent creates and returns a valid agent", "[ai][system]")
{
    AISystem sys;
    sys.Init();

    AIAgent& agent = sys.RegisterAgent(10);
    REQUIRE(agent.GetEntityId() == 10);
    REQUIRE(sys.GetAgent(10) != nullptr);

    sys.Shutdown();
}

TEST_CASE("AISystem::UnregisterAgent removes the agent", "[ai][system]")
{
    AISystem sys;
    sys.Init();
    sys.RegisterAgent(5);
    sys.UnregisterAgent(5);

    REQUIRE(sys.GetAgent(5) == nullptr);

    sys.Shutdown();
}

TEST_CASE("AISystem::GetAgent returns nullptr for unknown entity", "[ai][system]")
{
    AISystem sys;
    sys.Init();
    REQUIRE(sys.GetAgent(999) == nullptr);
    sys.Shutdown();
}

TEST_CASE("AISystem::RegisterAgent replaces existing agent for same entity", "[ai][system]")
{
    AISystem sys;
    sys.Init();

    AIAgent& first  = sys.RegisterAgent(1);
    AIAgent& second = sys.RegisterAgent(1);

    // After re-registration the pointer returned should be for the fresh agent
    REQUIRE(sys.GetAgent(1) == &second);
    (void)first;

    sys.Shutdown();
}

TEST_CASE("AISystem::Tick does not crash with agents that have no graph", "[ai][system]")
{
    AISystem sys;
    sys.Init();
    sys.RegisterAgent(1);
    sys.RegisterAgent(2);

    REQUIRE_NOTHROW(sys.Tick(0.016f));

    sys.Shutdown();
}

TEST_CASE("AISystem::Tick runs graph to completion for agent with a Yield-free graph", "[ai][system]")
{
    AISystem sys;
    sys.Init();

    // Build a trivial graph: PushInt 42 → Return
    auto graph = std::make_shared<CompiledGraph>();
    graph->name = "trivial";
    graph->instructions.push_back({ Opcode::PushInt, 42 });
    graph->instructions.push_back({ Opcode::Return,  0  });

    AIAgent& agent = sys.RegisterAgent(3);
    agent.SetBehaviorGraph(graph);

    sys.Tick(0.016f);

    REQUIRE(agent.GetGraphContext()->finished);

    sys.Shutdown();
}

TEST_CASE("AISystem::GetFactionManager is accessible and mutable", "[ai][system]")
{
    AISystem sys;
    sys.Init();

    sys.GetFactionManager().SetReputation("A", "B", 300);
    REQUIRE(sys.GetFactionManager().GetReputation("A", "B") == 300);

    sys.Shutdown();
}

TEST_CASE("AISystem::GetGlobalBlackboard stores shared world data", "[ai][system]")
{
    AISystem sys;
    sys.Init();

    sys.GetGlobalBlackboard().Set("worldTime", GraphValue{float{12.5f}});
    REQUIRE(sys.GetGlobalBlackboard().GetFloat("worldTime") == 12.5f);

    sys.Shutdown();
}

/// @file NodeGraphTests.cpp — Unit tests for the GraphVM bytecode VM.
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "Engine/NodeGraph/GraphVM.h"

using namespace NF;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Build a minimal CompiledGraph with the given instructions.
static CompiledGraph MakeGraph(std::vector<Instruction> instrs,
                               std::vector<std::string> strings   = {},
                               std::vector<std::string> variables = {},
                               std::vector<std::string> tools     = {})
{
    CompiledGraph g;
    g.name         = "test";
    g.type         = GraphType::GameFlow;
    g.instructions = std::move(instrs);
    g.stringTable  = std::move(strings);
    g.variableNames = std::move(variables);
    g.toolNames    = std::move(tools);
    return g;
}

static Instruction Op(Opcode op, int32_t operand = 0, float fOp = 0.f, uint16_t strIdx = 0)
{
    Instruction i;
    i.opcode   = op;
    i.operand  = operand;
    i.fOperand = fOp;
    i.stringIdx = strIdx;
    return i;
}

// ---------------------------------------------------------------------------
// CreateContext
// ---------------------------------------------------------------------------

TEST_CASE("GraphVM::CreateContext initialises a fresh context", "[graphvm]")
{
    GraphVM vm;
    auto g = MakeGraph({ Op(Opcode::Return) });
    auto ctx = vm.CreateContext(g);

    REQUIRE(ctx.graph == &g);
    REQUIRE(ctx.pc == 0);
    REQUIRE(ctx.stack.empty());
    REQUIRE(!ctx.finished);
    REQUIRE(!ctx.suspended);
}

// ---------------------------------------------------------------------------
// Nop / Return
// ---------------------------------------------------------------------------

TEST_CASE("GraphVM::Nop does nothing", "[graphvm]")
{
    GraphVM vm;
    auto g = MakeGraph({ Op(Opcode::Nop), Op(Opcode::Return) });
    auto ctx = vm.CreateContext(g);
    vm.Run(ctx);

    REQUIRE(ctx.finished);
    REQUIRE(ctx.stack.empty());
}

TEST_CASE("GraphVM::Return marks context finished", "[graphvm]")
{
    GraphVM vm;
    auto g = MakeGraph({ Op(Opcode::Return) });
    auto ctx = vm.CreateContext(g);
    vm.Step(ctx);

    REQUIRE(ctx.finished);
}

TEST_CASE("GraphVM: empty instruction list finishes immediately", "[graphvm]")
{
    GraphVM vm;
    auto g = MakeGraph({});
    auto ctx = vm.CreateContext(g);
    vm.Step(ctx);

    REQUIRE(ctx.finished);
}

// ---------------------------------------------------------------------------
// Push / Pop / Dup
// ---------------------------------------------------------------------------

TEST_CASE("GraphVM::PushNull pushes a null value", "[graphvm]")
{
    GraphVM vm;
    auto g = MakeGraph({ Op(Opcode::PushNull), Op(Opcode::Return) });
    auto ctx = vm.CreateContext(g);
    vm.Run(ctx);

    REQUIRE(ctx.stack.size() == 1);
    REQUIRE(IsNull(ctx.stack[0]));
}

TEST_CASE("GraphVM::PushBool pushes true when operand != 0", "[graphvm]")
{
    GraphVM vm;
    auto g = MakeGraph({ Op(Opcode::PushBool, 1), Op(Opcode::Return) });
    auto ctx = vm.CreateContext(g);
    vm.Run(ctx);

    REQUIRE(std::get<bool>(ctx.stack[0]) == true);
}

TEST_CASE("GraphVM::PushBool pushes false when operand == 0", "[graphvm]")
{
    GraphVM vm;
    auto g = MakeGraph({ Op(Opcode::PushBool, 0), Op(Opcode::Return) });
    auto ctx = vm.CreateContext(g);
    vm.Run(ctx);

    REQUIRE(std::get<bool>(ctx.stack[0]) == false);
}

TEST_CASE("GraphVM::PushInt pushes integer operand", "[graphvm]")
{
    GraphVM vm;
    auto g = MakeGraph({ Op(Opcode::PushInt, 42), Op(Opcode::Return) });
    auto ctx = vm.CreateContext(g);
    vm.Run(ctx);

    REQUIRE(std::get<int32_t>(ctx.stack[0]) == 42);
}

TEST_CASE("GraphVM::PushFloat pushes float operand", "[graphvm]")
{
    GraphVM vm;
    auto g = MakeGraph({ Op(Opcode::PushFloat, 0, 3.14f), Op(Opcode::Return) });
    auto ctx = vm.CreateContext(g);
    vm.Run(ctx);

    REQUIRE_THAT(std::get<float>(ctx.stack[0]),
                 Catch::Matchers::WithinRel(3.14f, 1e-5f));
}

TEST_CASE("GraphVM::PushString pushes string from table", "[graphvm]")
{
    GraphVM vm;
    // stringIdx = 0 → "hello"
    auto g = MakeGraph({ Op(Opcode::PushString, 0, 0.f, 0), Op(Opcode::Return) },
                       { "hello" });
    auto ctx = vm.CreateContext(g);
    vm.Run(ctx);

    REQUIRE(std::get<std::string>(ctx.stack[0]) == "hello");
}

TEST_CASE("GraphVM::PushString out-of-range idx pushes empty string", "[graphvm]")
{
    GraphVM vm;
    // No string table, stringIdx = 5 → empty
    auto g = MakeGraph({ Op(Opcode::PushString, 0, 0.f, 5), Op(Opcode::Return) });
    auto ctx = vm.CreateContext(g);
    vm.Run(ctx);

    REQUIRE(std::get<std::string>(ctx.stack[0]).empty());
}

TEST_CASE("GraphVM::Pop removes top value", "[graphvm]")
{
    GraphVM vm;
    auto g = MakeGraph({ Op(Opcode::PushInt, 7), Op(Opcode::Pop), Op(Opcode::Return) });
    auto ctx = vm.CreateContext(g);
    vm.Run(ctx);

    REQUIRE(ctx.stack.empty());
}

TEST_CASE("GraphVM::Dup duplicates top value", "[graphvm]")
{
    GraphVM vm;
    auto g = MakeGraph({ Op(Opcode::PushInt, 5), Op(Opcode::Dup), Op(Opcode::Return) });
    auto ctx = vm.CreateContext(g);
    vm.Run(ctx);

    REQUIRE(ctx.stack.size() == 2);
    REQUIRE(std::get<int32_t>(ctx.stack[0]) == 5);
    REQUIRE(std::get<int32_t>(ctx.stack[1]) == 5);
}

// ---------------------------------------------------------------------------
// Integer arithmetic
// ---------------------------------------------------------------------------

TEST_CASE("GraphVM::AddInt adds two integers", "[graphvm]")
{
    GraphVM vm;
    auto g = MakeGraph({
        Op(Opcode::PushInt, 10),
        Op(Opcode::PushInt, 3),
        Op(Opcode::AddInt),
        Op(Opcode::Return)
    });
    auto ctx = vm.CreateContext(g);
    vm.Run(ctx);
    REQUIRE(std::get<int32_t>(ctx.stack[0]) == 13);
}

TEST_CASE("GraphVM::SubInt subtracts two integers", "[graphvm]")
{
    GraphVM vm;
    auto g = MakeGraph({
        Op(Opcode::PushInt, 10),
        Op(Opcode::PushInt, 3),
        Op(Opcode::SubInt),
        Op(Opcode::Return)
    });
    auto ctx = vm.CreateContext(g);
    vm.Run(ctx);
    REQUIRE(std::get<int32_t>(ctx.stack[0]) == 7);
}

TEST_CASE("GraphVM::MulInt multiplies two integers", "[graphvm]")
{
    GraphVM vm;
    auto g = MakeGraph({
        Op(Opcode::PushInt, 6),
        Op(Opcode::PushInt, 7),
        Op(Opcode::MulInt),
        Op(Opcode::Return)
    });
    auto ctx = vm.CreateContext(g);
    vm.Run(ctx);
    REQUIRE(std::get<int32_t>(ctx.stack[0]) == 42);
}

TEST_CASE("GraphVM::DivInt divides two integers", "[graphvm]")
{
    GraphVM vm;
    auto g = MakeGraph({
        Op(Opcode::PushInt, 12),
        Op(Opcode::PushInt, 4),
        Op(Opcode::DivInt),
        Op(Opcode::Return)
    });
    auto ctx = vm.CreateContext(g);
    vm.Run(ctx);
    REQUIRE(std::get<int32_t>(ctx.stack[0]) == 3);
}

TEST_CASE("GraphVM::DivInt returns 0 on divide-by-zero", "[graphvm]")
{
    GraphVM vm;
    auto g = MakeGraph({
        Op(Opcode::PushInt, 10),
        Op(Opcode::PushInt, 0),
        Op(Opcode::DivInt),
        Op(Opcode::Return)
    });
    auto ctx = vm.CreateContext(g);
    vm.Run(ctx);
    REQUIRE(std::get<int32_t>(ctx.stack[0]) == 0);
}

// ---------------------------------------------------------------------------
// Float arithmetic
// ---------------------------------------------------------------------------

TEST_CASE("GraphVM::AddFloat adds two floats", "[graphvm]")
{
    GraphVM vm;
    auto g = MakeGraph({
        Op(Opcode::PushFloat, 0, 1.5f),
        Op(Opcode::PushFloat, 0, 2.5f),
        Op(Opcode::AddFloat),
        Op(Opcode::Return)
    });
    auto ctx = vm.CreateContext(g);
    vm.Run(ctx);
    REQUIRE_THAT(std::get<float>(ctx.stack[0]), Catch::Matchers::WithinRel(4.0f, 1e-5f));
}

TEST_CASE("GraphVM::MulFloat multiplies two floats", "[graphvm]")
{
    GraphVM vm;
    auto g = MakeGraph({
        Op(Opcode::PushFloat, 0, 2.0f),
        Op(Opcode::PushFloat, 0, 3.5f),
        Op(Opcode::MulFloat),
        Op(Opcode::Return)
    });
    auto ctx = vm.CreateContext(g);
    vm.Run(ctx);
    REQUIRE_THAT(std::get<float>(ctx.stack[0]), Catch::Matchers::WithinRel(7.0f, 1e-5f));
}

TEST_CASE("GraphVM::DivFloat returns 0 on divide-by-zero", "[graphvm]")
{
    GraphVM vm;
    auto g = MakeGraph({
        Op(Opcode::PushFloat, 0, 5.0f),
        Op(Opcode::PushFloat, 0, 0.0f),
        Op(Opcode::DivFloat),
        Op(Opcode::Return)
    });
    auto ctx = vm.CreateContext(g);
    vm.Run(ctx);
    REQUIRE_THAT(std::get<float>(ctx.stack[0]), Catch::Matchers::WithinAbs(0.f, 1e-5f));
}

// ---------------------------------------------------------------------------
// Comparison — integer
// ---------------------------------------------------------------------------

TEST_CASE("GraphVM::EqInt returns true when equal", "[graphvm]")
{
    GraphVM vm;
    auto g = MakeGraph({ Op(Opcode::PushInt,3), Op(Opcode::PushInt,3), Op(Opcode::EqInt), Op(Opcode::Return) });
    auto ctx = vm.CreateContext(g);
    vm.Run(ctx);
    REQUIRE(std::get<bool>(ctx.stack[0]) == true);
}

TEST_CASE("GraphVM::NeInt returns true when not equal", "[graphvm]")
{
    GraphVM vm;
    auto g = MakeGraph({ Op(Opcode::PushInt,3), Op(Opcode::PushInt,4), Op(Opcode::NeInt), Op(Opcode::Return) });
    auto ctx = vm.CreateContext(g);
    vm.Run(ctx);
    REQUIRE(std::get<bool>(ctx.stack[0]) == true);
}

TEST_CASE("GraphVM::LtInt returns true when a < b", "[graphvm]")
{
    GraphVM vm;
    auto g = MakeGraph({ Op(Opcode::PushInt,2), Op(Opcode::PushInt,5), Op(Opcode::LtInt), Op(Opcode::Return) });
    auto ctx = vm.CreateContext(g);
    vm.Run(ctx);
    REQUIRE(std::get<bool>(ctx.stack[0]) == true);
}

TEST_CASE("GraphVM::GtInt returns false when a < b", "[graphvm]")
{
    GraphVM vm;
    auto g = MakeGraph({ Op(Opcode::PushInt,2), Op(Opcode::PushInt,5), Op(Opcode::GtInt), Op(Opcode::Return) });
    auto ctx = vm.CreateContext(g);
    vm.Run(ctx);
    REQUIRE(std::get<bool>(ctx.stack[0]) == false);
}

// ---------------------------------------------------------------------------
// Logic
// ---------------------------------------------------------------------------

TEST_CASE("GraphVM::And returns true when both true", "[graphvm]")
{
    GraphVM vm;
    auto g = MakeGraph({ Op(Opcode::PushBool,1), Op(Opcode::PushBool,1), Op(Opcode::And), Op(Opcode::Return) });
    auto ctx = vm.CreateContext(g);
    vm.Run(ctx);
    REQUIRE(std::get<bool>(ctx.stack[0]) == true);
}

TEST_CASE("GraphVM::And returns false when one is false", "[graphvm]")
{
    GraphVM vm;
    auto g = MakeGraph({ Op(Opcode::PushBool,1), Op(Opcode::PushBool,0), Op(Opcode::And), Op(Opcode::Return) });
    auto ctx = vm.CreateContext(g);
    vm.Run(ctx);
    REQUIRE(std::get<bool>(ctx.stack[0]) == false);
}

TEST_CASE("GraphVM::Or returns true when one is true", "[graphvm]")
{
    GraphVM vm;
    auto g = MakeGraph({ Op(Opcode::PushBool,0), Op(Opcode::PushBool,1), Op(Opcode::Or), Op(Opcode::Return) });
    auto ctx = vm.CreateContext(g);
    vm.Run(ctx);
    REQUIRE(std::get<bool>(ctx.stack[0]) == true);
}

TEST_CASE("GraphVM::Not flips bool", "[graphvm]")
{
    GraphVM vm;
    auto g = MakeGraph({ Op(Opcode::PushBool,1), Op(Opcode::Not), Op(Opcode::Return) });
    auto ctx = vm.CreateContext(g);
    vm.Run(ctx);
    REQUIRE(std::get<bool>(ctx.stack[0]) == false);
}

// ---------------------------------------------------------------------------
// Variables (LoadVar / StoreVar)
// ---------------------------------------------------------------------------

TEST_CASE("GraphVM::StoreVar and LoadVar round-trip an integer", "[graphvm]")
{
    GraphVM vm;
    // variableNames[0] = "counter"
    // PushInt 99 → StoreVar 0 → LoadVar 0 → Return
    auto g = MakeGraph({
        Op(Opcode::PushInt, 99),
        Op(Opcode::StoreVar, 0),
        Op(Opcode::LoadVar,  0),
        Op(Opcode::Return)
    }, {}, { "counter" });
    auto ctx = vm.CreateContext(g);
    vm.Run(ctx);

    REQUIRE(ctx.stack.size() == 1);
    REQUIRE(std::get<int32_t>(ctx.stack[0]) == 99);
    REQUIRE(ctx.blackboard.at("counter") == GraphValue{int32_t{99}});
}

TEST_CASE("GraphVM::LoadVar of unset variable pushes null", "[graphvm]")
{
    GraphVM vm;
    auto g = MakeGraph({
        Op(Opcode::LoadVar, 0),
        Op(Opcode::Return)
    }, {}, { "missing" });
    auto ctx = vm.CreateContext(g);
    vm.Run(ctx);
    REQUIRE(IsNull(ctx.stack[0]));
}

// ---------------------------------------------------------------------------
// Control flow — Jump
// ---------------------------------------------------------------------------

TEST_CASE("GraphVM::Jump jumps unconditionally", "[graphvm]")
{
    GraphVM vm;
    // Instruction 0: Jump → 2
    // Instruction 1: PushInt 999  (must be skipped)
    // Instruction 2: PushInt 42
    // Instruction 3: Return
    auto g = MakeGraph({
        Op(Opcode::Jump,    2),
        Op(Opcode::PushInt, 999),
        Op(Opcode::PushInt, 42),
        Op(Opcode::Return)
    });
    auto ctx = vm.CreateContext(g);
    vm.Run(ctx);

    REQUIRE(ctx.stack.size() == 1);
    REQUIRE(std::get<int32_t>(ctx.stack[0]) == 42);
}

TEST_CASE("GraphVM::JumpTrue jumps when top is true", "[graphvm]")
{
    GraphVM vm;
    // 0: PushBool 1
    // 1: JumpTrue → 3
    // 2: PushInt 0  (skipped)
    // 3: PushInt 1
    // 4: Return
    auto g = MakeGraph({
        Op(Opcode::PushBool, 1),
        Op(Opcode::JumpTrue, 3),
        Op(Opcode::PushInt,  0),
        Op(Opcode::PushInt,  1),
        Op(Opcode::Return)
    });
    auto ctx = vm.CreateContext(g);
    vm.Run(ctx);

    REQUIRE(ctx.stack.size() == 1);
    REQUIRE(std::get<int32_t>(ctx.stack[0]) == 1);
}

TEST_CASE("GraphVM::JumpFalse jumps when top is false", "[graphvm]")
{
    GraphVM vm;
    // 0: PushBool 0
    // 1: JumpFalse → 3
    // 2: PushInt 0  (skipped)
    // 3: PushInt 2
    // 4: Return
    auto g = MakeGraph({
        Op(Opcode::PushBool,  0),
        Op(Opcode::JumpFalse, 3),
        Op(Opcode::PushInt,   0),
        Op(Opcode::PushInt,   2),
        Op(Opcode::Return)
    });
    auto ctx = vm.CreateContext(g);
    vm.Run(ctx);

    REQUIRE(ctx.stack.size() == 1);
    REQUIRE(std::get<int32_t>(ctx.stack[0]) == 2);
}

TEST_CASE("GraphVM::JumpTrue does not jump when top is false", "[graphvm]")
{
    GraphVM vm;
    // 0: PushBool 0
    // 1: JumpTrue → 3  (not taken)
    // 2: PushInt 99
    // 3: Return
    auto g = MakeGraph({
        Op(Opcode::PushBool, 0),
        Op(Opcode::JumpTrue, 3),
        Op(Opcode::PushInt,  99),
        Op(Opcode::Return)
    });
    auto ctx = vm.CreateContext(g);
    vm.Run(ctx);

    REQUIRE(ctx.stack.size() == 1);
    REQUIRE(std::get<int32_t>(ctx.stack[0]) == 99);
}

// ---------------------------------------------------------------------------
// Yield / Resume
// ---------------------------------------------------------------------------

TEST_CASE("GraphVM::Yield suspends execution", "[graphvm]")
{
    GraphVM vm;
    // 0: Yield
    // 1: PushInt 10
    // 2: Return
    auto g = MakeGraph({
        Op(Opcode::Yield),
        Op(Opcode::PushInt, 10),
        Op(Opcode::Return)
    });
    auto ctx = vm.CreateContext(g);
    vm.Run(ctx);

    REQUIRE(ctx.suspended);
    REQUIRE(!ctx.finished);
    REQUIRE(ctx.stack.empty());
}

TEST_CASE("GraphVM::Resume continues after Yield", "[graphvm]")
{
    GraphVM vm;
    auto g = MakeGraph({
        Op(Opcode::Yield),
        Op(Opcode::PushInt, 10),
        Op(Opcode::Return)
    });
    auto ctx = vm.CreateContext(g);
    vm.Run(ctx);          // suspends at Yield
    vm.Resume(ctx);       // continues from instruction 1

    REQUIRE(ctx.finished);
    REQUIRE(ctx.stack.size() == 1);
    REQUIRE(std::get<int32_t>(ctx.stack[0]) == 10);
}

TEST_CASE("GraphVM::Step does nothing on finished context", "[graphvm]")
{
    GraphVM vm;
    auto g = MakeGraph({ Op(Opcode::Return) });
    auto ctx = vm.CreateContext(g);
    vm.Run(ctx);
    uint32_t pc = ctx.pc;
    vm.Step(ctx);
    REQUIRE(ctx.pc == pc);
}

TEST_CASE("GraphVM::Step does nothing on suspended context", "[graphvm]")
{
    GraphVM vm;
    auto g = MakeGraph({ Op(Opcode::Yield), Op(Opcode::Return) });
    auto ctx = vm.CreateContext(g);
    vm.Run(ctx);
    uint32_t pc = ctx.pc;
    vm.Step(ctx);
    REQUIRE(ctx.pc == pc);
}

// ---------------------------------------------------------------------------
// CallTool
// ---------------------------------------------------------------------------

TEST_CASE("GraphVM::CallTool invokes registered tool", "[graphvm]")
{
    GraphVM vm;
    bool called = false;
    vm.RegisterTool("MyTool", [&called](ExecutionContext&, std::string_view) -> GraphValue {
        called = true;
        return int32_t{7};
    });

    // toolNames[0] = "MyTool"
    auto g = MakeGraph({
        Op(Opcode::CallTool, 0),
        Op(Opcode::Return)
    }, {}, {}, { "MyTool" });
    auto ctx = vm.CreateContext(g);
    vm.Run(ctx);

    REQUIRE(called);
    REQUIRE(ctx.stack.size() == 1);
    REQUIRE(std::get<int32_t>(ctx.stack[0]) == 7);
}

TEST_CASE("GraphVM::CallTool pushes null for unknown tool", "[graphvm]")
{
    GraphVM vm;
    auto g = MakeGraph({
        Op(Opcode::CallTool, 0),
        Op(Opcode::Return)
    }, {}, {}, { "DoesNotExist" });
    auto ctx = vm.CreateContext(g);
    vm.Run(ctx);

    REQUIRE(ctx.stack.size() == 1);
    REQUIRE(IsNull(ctx.stack[0]));
}

TEST_CASE("GraphVM::UnregisterTool removes a tool", "[graphvm]")
{
    GraphVM vm;
    bool called = false;
    vm.RegisterTool("T", [&called](ExecutionContext&, std::string_view) -> GraphValue {
        called = true;
        return {};
    });
    vm.UnregisterTool("T");

    auto g = MakeGraph({ Op(Opcode::CallTool, 0), Op(Opcode::Return) }, {}, {}, { "T" });
    auto ctx = vm.CreateContext(g);
    vm.Run(ctx);

    REQUIRE(!called);
}

// ---------------------------------------------------------------------------
// maxSteps safety limit
// ---------------------------------------------------------------------------

TEST_CASE("GraphVM::Run respects maxSteps limit", "[graphvm]")
{
    GraphVM vm;
    // Infinite loop: Jump 0 forever
    auto g = MakeGraph({ Op(Opcode::Jump, 0) });
    auto ctx = vm.CreateContext(g);
    int32_t steps = vm.Run(ctx, 10);

    REQUIRE(steps <= 10);
    REQUIRE(!ctx.finished);
}

// ---------------------------------------------------------------------------
// Disassemble
// ---------------------------------------------------------------------------

TEST_CASE("GraphVM::Disassemble returns non-empty string", "[graphvm]")
{
    GraphVM vm;
    auto g = MakeGraph({
        Op(Opcode::PushInt, 5),
        Op(Opcode::Return)
    });
    std::string dis = vm.Disassemble(g);
    REQUIRE(!dis.empty());
    REQUIRE(dis.find("test") != std::string::npos);
}

// ---------------------------------------------------------------------------
// Builtin tools (Log, SetBlackboard, GetBlackboard) via AISystem-style setup
// ---------------------------------------------------------------------------

TEST_CASE("GraphVM Log builtin tool does not crash", "[graphvm]")
{
    GraphVM vm;
    vm.RegisterTool("Log", [](ExecutionContext& ctx, std::string_view) -> GraphValue {
        if (!ctx.stack.empty()) ctx.stack.pop_back();
        return {};
    });

    auto g = MakeGraph({
        Op(Opcode::PushString, 0, 0.f, 0),
        Op(Opcode::CallTool, 0),
        Op(Opcode::Return)
    }, { "hello world" }, {}, { "Log" });
    auto ctx = vm.CreateContext(g);
    REQUIRE_NOTHROW(vm.Run(ctx));
}

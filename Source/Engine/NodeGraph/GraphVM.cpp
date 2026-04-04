// GraphVM.cpp — Deterministic bytecode VM implementation.
#include "Engine/NodeGraph/GraphVM.h"
#include <sstream>
#include <stdexcept>

namespace NF {

// ---------------------------------------------------------------------------
// Tool registry
// ---------------------------------------------------------------------------

void GraphVM::RegisterTool(std::string name, ToolFn fn)
{
    m_Tools[std::move(name)] = std::move(fn);
}

void GraphVM::UnregisterTool(const std::string& name)
{
    m_Tools.erase(name);
}

// ---------------------------------------------------------------------------
// Context creation
// ---------------------------------------------------------------------------

ExecutionContext GraphVM::CreateContext(const CompiledGraph& graph) const
{
    ExecutionContext ctx;
    ctx.graph = &graph;
    ctx.pc    = 0;
    ctx.stack.reserve(64);
    return ctx;
}

// ---------------------------------------------------------------------------
// Step — execute one instruction
// ---------------------------------------------------------------------------

void GraphVM::Step(ExecutionContext& ctx) const
{
    if (!ctx.graph || ctx.finished || ctx.suspended)
        return;

    const auto& instructions = ctx.graph->instructions;
    if (ctx.pc >= static_cast<uint32_t>(instructions.size())) {
        ctx.finished = true;
        return;
    }

    ExecuteInstruction(ctx);
}

// ---------------------------------------------------------------------------
// Run — execute up to maxSteps instructions
// ---------------------------------------------------------------------------

int32_t GraphVM::Run(ExecutionContext& ctx, int32_t maxSteps) const
{
    int32_t steps = 0;
    while (!ctx.finished && !ctx.suspended && steps < maxSteps) {
        Step(ctx);
        ++steps;
    }
    if (steps >= maxSteps && !ctx.finished) {
        NF::Logger::Log(NF::LogLevel::Warning, "GraphVM",
            "Instruction limit reached for graph: " +
            (ctx.graph ? ctx.graph->name : "(null)"));
    }
    return steps;
}

// ---------------------------------------------------------------------------
// Resume
// ---------------------------------------------------------------------------

int32_t GraphVM::Resume(ExecutionContext& ctx, int32_t maxSteps) const
{
    ctx.suspended = false;
    return Run(ctx, maxSteps);
}

// ---------------------------------------------------------------------------
// ExecuteInstruction — inner dispatch loop
// ---------------------------------------------------------------------------

void GraphVM::ExecuteInstruction(ExecutionContext& ctx) const
{
    const auto& instr = ctx.graph->instructions[ctx.pc];
    ++ctx.pc;  // advance before executing so jumps can overwrite

    auto& stack = ctx.stack;

    auto popValue = [&]() -> GraphValue {
        if (stack.empty()) {
            NF::Logger::Log(NF::LogLevel::Error, "GraphVM", "Stack underflow");
            return {};
        }
        GraphValue v = std::move(stack.back());
        stack.pop_back();
        return v;
    };

    auto topBool = [&]() -> bool {
        auto v = popValue();
        if (auto* b = std::get_if<bool>(&v)) return *b;
        if (auto* i = std::get_if<int32_t>(&v)) return *i != 0;
        return false;
    };

    auto topInt = [&]() -> int32_t {
        auto v = popValue();
        if (auto* i = std::get_if<int32_t>(&v)) return *i;
        if (auto* f = std::get_if<float>(&v))   return static_cast<int32_t>(*f);
        return 0;
    };

    auto topFloat = [&]() -> float {
        auto v = popValue();
        if (auto* f = std::get_if<float>(&v))   return *f;
        if (auto* i = std::get_if<int32_t>(&v)) return static_cast<float>(*i);
        return 0.f;
    };

    switch (instr.opcode) {
    // ---- Control flow -------------------------------------------------------
    case Opcode::Nop:
        break;

    case Opcode::Jump:
        ctx.pc = static_cast<uint32_t>(instr.operand);
        break;

    case Opcode::JumpTrue:
        if (topBool())
            ctx.pc = static_cast<uint32_t>(instr.operand);
        break;

    case Opcode::JumpFalse:
        if (!topBool())
            ctx.pc = static_cast<uint32_t>(instr.operand);
        break;

    case Opcode::Return:
        ctx.finished = true;
        break;

    // ---- Stack --------------------------------------------------------------
    case Opcode::PushNull:
        stack.emplace_back(std::monostate{});
        break;

    case Opcode::PushBool:
        stack.emplace_back(instr.operand != 0);
        break;

    case Opcode::PushInt:
        stack.emplace_back(instr.operand);
        break;

    case Opcode::PushFloat:
        stack.emplace_back(instr.fOperand);
        break;

    case Opcode::PushString: {
        const auto& st = ctx.graph->stringTable;
        uint16_t idx = instr.stringIdx;
        if (idx < static_cast<uint16_t>(st.size()))
            stack.emplace_back(st[idx]);
        else
            stack.emplace_back(std::string{});
        break;
    }

    case Opcode::Pop:
        if (!stack.empty()) stack.pop_back();
        break;

    case Opcode::Dup:
        if (!stack.empty()) stack.push_back(stack.back());
        break;

    // ---- Integer arithmetic -------------------------------------------------
    case Opcode::AddInt: { int32_t b = topInt(), a = topInt(); stack.emplace_back(a + b); break; }
    case Opcode::SubInt: { int32_t b = topInt(), a = topInt(); stack.emplace_back(a - b); break; }
    case Opcode::MulInt: { int32_t b = topInt(), a = topInt(); stack.emplace_back(a * b); break; }
    case Opcode::DivInt: {
        int32_t b = topInt(), a = topInt();
        stack.emplace_back(b != 0 ? a / b : 0);
        break;
    }

    // ---- Float arithmetic ---------------------------------------------------
    case Opcode::AddFloat: { float b = topFloat(), a = topFloat(); stack.emplace_back(a + b); break; }
    case Opcode::SubFloat: { float b = topFloat(), a = topFloat(); stack.emplace_back(a - b); break; }
    case Opcode::MulFloat: { float b = topFloat(), a = topFloat(); stack.emplace_back(a * b); break; }
    case Opcode::DivFloat: {
        float b = topFloat(), a = topFloat();
        stack.emplace_back(b != 0.f ? a / b : 0.f);
        break;
    }

    // ---- Integer comparison -------------------------------------------------
    case Opcode::EqInt: { int32_t b = topInt(), a = topInt(); stack.emplace_back(a == b); break; }
    case Opcode::NeInt: { int32_t b = topInt(), a = topInt(); stack.emplace_back(a != b); break; }
    case Opcode::LtInt: { int32_t b = topInt(), a = topInt(); stack.emplace_back(a <  b); break; }
    case Opcode::GtInt: { int32_t b = topInt(), a = topInt(); stack.emplace_back(a >  b); break; }

    // ---- Float comparison ---------------------------------------------------
    case Opcode::EqFloat: { float b = topFloat(), a = topFloat(); stack.emplace_back(a == b); break; }
    case Opcode::NeFloat: { float b = topFloat(), a = topFloat(); stack.emplace_back(a != b); break; }
    case Opcode::LtFloat: { float b = topFloat(), a = topFloat(); stack.emplace_back(a <  b); break; }
    case Opcode::GtFloat: { float b = topFloat(), a = topFloat(); stack.emplace_back(a >  b); break; }

    // ---- Logic --------------------------------------------------------------
    case Opcode::And: { bool b = topBool(), a = topBool(); stack.emplace_back(a && b); break; }
    case Opcode::Or:  { bool b = topBool(), a = topBool(); stack.emplace_back(a || b); break; }
    case Opcode::Not: { stack.emplace_back(!topBool()); break; }

    // ---- Variables ----------------------------------------------------------
    case Opcode::LoadVar: {
        const auto& varNames = ctx.graph->variableNames;
        uint32_t idx = static_cast<uint32_t>(instr.operand);
        if (idx < varNames.size()) {
            auto it = ctx.blackboard.find(varNames[idx]);
            stack.emplace_back(it != ctx.blackboard.end() ? it->second : GraphValue{});
        } else {
            stack.emplace_back(GraphValue{});
        }
        break;
    }

    case Opcode::StoreVar: {
        const auto& varNames = ctx.graph->variableNames;
        uint32_t idx = static_cast<uint32_t>(instr.operand);
        if (!stack.empty() && idx < varNames.size()) {
            ctx.blackboard[varNames[idx]] = popValue();
        }
        break;
    }

    // ---- Tools --------------------------------------------------------------
    case Opcode::CallTool: {
        const auto& toolNames = ctx.graph->toolNames;
        uint32_t idx = static_cast<uint32_t>(instr.operand);
        if (idx < toolNames.size()) {
            const std::string& toolName = toolNames[idx];
            auto it = m_Tools.find(toolName);
            if (it != m_Tools.end()) {
                GraphValue result = it->second(ctx, toolName);
                stack.emplace_back(std::move(result));
            } else {
                NF::Logger::Log(NF::LogLevel::Warning, "GraphVM",
                    "Unknown tool: " + toolName);
                stack.emplace_back(GraphValue{});
            }
        }
        break;
    }

    // ---- Yield --------------------------------------------------------------
    case Opcode::Yield:
        ctx.suspended = true;
        // pc already incremented — will resume at the instruction after Yield
        break;

    default:
        NF::Logger::Log(NF::LogLevel::Error, "GraphVM",
            "Unknown opcode: " + std::to_string(static_cast<uint32_t>(instr.opcode)));
        ctx.finished = true;
        break;
    }
}

// ---------------------------------------------------------------------------
// Disassemble
// ---------------------------------------------------------------------------

std::string GraphVM::Disassemble(const CompiledGraph& graph) const
{
    std::ostringstream oss;
    oss << "=== GraphVM Disassembly: " << graph.name << " ===\n";
    oss << "Type       : " << static_cast<int>(graph.type) << "\n";
    oss << "Instructions: " << graph.instructions.size() << "\n";
    oss << "Strings    : " << graph.stringTable.size() << "\n";
    oss << "Variables  : " << graph.variableNames.size() << "\n";
    oss << "Tools      : " << graph.toolNames.size() << "\n\n";

    for (uint32_t i = 0; i < static_cast<uint32_t>(graph.instructions.size()); ++i) {
        const auto& instr = graph.instructions[i];
        oss << std::to_string(i) << "\t";
        oss << static_cast<uint32_t>(instr.opcode);
        if (instr.operand != 0) oss << "\t" << instr.operand;
        if (instr.fOperand != 0.f) oss << "\tf=" << instr.fOperand;
        if (instr.stringIdx != 0) oss << "\ts=" << instr.stringIdx;
        oss << "\n";
    }
    return oss.str();
}

} // namespace NF

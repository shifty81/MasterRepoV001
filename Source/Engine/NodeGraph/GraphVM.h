#pragma once
// GraphVM.h — Deterministic bytecode virtual machine for NovaForge node graphs.
//
// GraphVM executes compiled .nfgraph bytecode at runtime.  It is used by all
// 14 graph types (World, Behavior, Conversation, Animation, UI, UIScreen,
// GameFlow, Story, etc.).  The VM is fully deterministic: given the same
// initial state and inputs, execution always produces the same result.  This
// is a hard requirement for lockstep-rollback multiplayer.
//
// Porting note (from tempnovaforge):
//   - Removed #include "NF/Core/Core.h" — use "Core/Logging/Log.h" instead.
//   - Vec2 replaced with NF::Vector2 from "Core/Math/Vector.h".
//   - JsonValue replaced with a forward-declared stub; full JSON support is
//     wired in Phase 4 via nlohmann/json.

#include "Core/Logging/Log.h"
#include "Core/Math/Vector.h"
#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace NF {

// ---------------------------------------------------------------------------
// Graph type catalogue
// ---------------------------------------------------------------------------

/// @brief Identifies which high-level authoring domain a graph belongs to.
enum class GraphType : uint8_t {
    World        = 0,
    Behavior     = 1,
    Conversation = 2,
    Animation    = 3,
    UI           = 4,
    UIScreen     = 5,
    GameFlow     = 6,
    Story        = 7,
    Economy      = 8,
    Faction      = 9,
    Combat       = 10,
    Crafting     = 11,
    Weather      = 12,
    Mission      = 13,
};

// ---------------------------------------------------------------------------
// Value — the universal runtime value type
// ---------------------------------------------------------------------------

/// @brief Tagged-union value that can hold any type the VM operates on.
using GraphValue = std::variant<
    std::monostate,   // Null / void
    bool,
    int32_t,
    float,
    Vector2,
    Vector3,
    std::string
>;

/// @brief Convenience helpers for common value checks.
inline bool IsNull(const GraphValue& v) noexcept {
    return std::holds_alternative<std::monostate>(v);
}

// ---------------------------------------------------------------------------
// Opcode
// ---------------------------------------------------------------------------

/// @brief Bytecode instruction set for the GraphVM.
enum class Opcode : uint8_t {
    // Control flow
    Nop         = 0x00,
    Jump        = 0x01,  // unconditional jump to instruction offset
    JumpTrue    = 0x02,  // pop bool; jump if true
    JumpFalse   = 0x03,  // pop bool; jump if false
    Return      = 0x04,  // end execution of current graph
    // Stack
    PushNull    = 0x10,
    PushBool    = 0x11,
    PushInt     = 0x12,
    PushFloat   = 0x13,
    PushString  = 0x14,
    Pop         = 0x15,
    Dup         = 0x16,
    // Arithmetic
    AddInt      = 0x20,
    SubInt      = 0x21,
    MulInt      = 0x22,
    DivInt      = 0x23,
    AddFloat    = 0x24,
    SubFloat    = 0x25,
    MulFloat    = 0x26,
    DivFloat    = 0x27,
    // Comparison
    EqInt       = 0x30,
    NeInt       = 0x31,
    LtInt       = 0x32,
    GtInt       = 0x33,
    EqFloat     = 0x34,
    NeFloat     = 0x35,
    LtFloat     = 0x36,
    GtFloat     = 0x37,
    // Logic
    And         = 0x40,
    Or          = 0x41,
    Not         = 0x42,
    // Variables
    LoadVar     = 0x50,  // push named variable from blackboard
    StoreVar    = 0x51,  // pop and store to named variable
    // Calls
    CallTool    = 0x60,  // invoke a registered tool by name-index
    // Yield
    Yield       = 0x70,  // suspend execution; resume next tick
};

// ---------------------------------------------------------------------------
// Instruction
// ---------------------------------------------------------------------------

/// @brief A single bytecode instruction.
struct Instruction {
    Opcode   opcode{Opcode::Nop};
    int32_t  operand{0};      ///< Integer / offset operand (meaning varies by opcode)
    float    fOperand{0.f};   ///< Float operand for PushFloat / FP arithmetic
    uint16_t stringIdx{0};    ///< Index into the constant string table
};

// ---------------------------------------------------------------------------
// CompiledGraph — bytecode + constant tables
// ---------------------------------------------------------------------------

/// @brief Complete compiled representation of a single graph.
struct CompiledGraph {
    GraphType               type{GraphType::GameFlow};
    std::string             name;                  ///< Debug name (matches file stem)
    std::vector<Instruction> instructions;
    std::vector<std::string> stringTable;          ///< String constants indexed by stringIdx
    std::vector<std::string> variableNames;        ///< Blackboard variable name table
    std::vector<std::string> toolNames;            ///< Tool names indexed by CallTool operand
};

// ---------------------------------------------------------------------------
// ExecutionContext — per-instance runtime state
// ---------------------------------------------------------------------------

/// @brief Runtime state for a single executing graph instance.
///
/// Multiple instances of the same CompiledGraph can execute simultaneously
/// (e.g., the same NPC Behavior graph running on 10 NPCs) — each with its
/// own ExecutionContext.
struct ExecutionContext {
    const CompiledGraph* graph{nullptr}; ///< Bytecode being executed (non-owning)

    uint32_t                pc{0};           ///< Program counter
    std::vector<GraphValue> stack;           ///< Operand stack
    std::unordered_map<std::string, GraphValue> blackboard; ///< Named variables

    bool   suspended{false};   ///< True when Yield was executed
    bool   finished{false};    ///< True when Return was executed

    // Optional entity context — the entity this graph is running on behalf of.
    // Set by the caller; the VM does not interpret it.
    uint64_t entityId{0};
};

// ---------------------------------------------------------------------------
// Tool — external callable registered with the VM
// ---------------------------------------------------------------------------

/// @brief Signature for a tool function called from bytecode via CallTool.
///
/// @param ctx   The current execution context (stack access, blackboard).
/// @param name  The tool's registered name (for diagnostic use).
/// @return      Value to push onto the stack (push Null if void).
using ToolFn = std::function<GraphValue(ExecutionContext&, std::string_view)>;

// ---------------------------------------------------------------------------
// GraphVM
// ---------------------------------------------------------------------------

/// @brief Deterministic bytecode virtual machine for node graphs.
///
/// ## Usage
/// ```cpp
/// GraphVM vm;
/// vm.RegisterTool("SpawnNPC", [](ExecutionContext& ctx, std::string_view) {
///     // ... implementation
///     return GraphValue{};
/// });
///
/// CompiledGraph graph = GraphCompiler::Compile("mission.nfgraph");
/// ExecutionContext ctx = vm.CreateContext(graph);
/// ctx.entityId = missionEntityId;
///
/// while (!ctx.finished) {
///     vm.Step(ctx);      // advance one instruction
///     if (ctx.suspended)
///         break;         // resume next game tick
/// }
/// ```
class GraphVM {
public:
    // ---- Tool registry -------------------------------------------------------

    /// @brief Register a named tool callable from graph bytecode.
    void RegisterTool(std::string name, ToolFn fn);

    /// @brief Unregister a tool by name.
    void UnregisterTool(const std::string& name);

    // ---- Execution -----------------------------------------------------------

    /// @brief Create a fresh execution context for a compiled graph.
    [[nodiscard]] ExecutionContext CreateContext(const CompiledGraph& graph) const;

    /// @brief Advance execution by one instruction.
    ///
    /// Returns immediately if @p ctx is already finished or suspended.
    /// Sets ctx.suspended on Yield, ctx.finished on Return.
    void Step(ExecutionContext& ctx) const;

    /// @brief Run until Return, Yield, or the instruction limit is hit.
    ///
    /// @param ctx          Execution context to advance.
    /// @param maxSteps     Safety limit (prevents infinite loops). Default 10 000.
    /// @return             Number of instructions executed.
    int32_t Run(ExecutionContext& ctx, int32_t maxSteps = 10000) const;

    /// @brief Resume a previously suspended context.
    ///
    /// Clears the suspended flag and calls Run().
    int32_t Resume(ExecutionContext& ctx, int32_t maxSteps = 10000) const;

    // ---- Diagnostics ---------------------------------------------------------

    /// @brief Return a human-readable disassembly of the compiled graph.
    [[nodiscard]] std::string Disassemble(const CompiledGraph& graph) const;

private:
    std::unordered_map<std::string, ToolFn> m_Tools;

    // Internal dispatch — executes one instruction, advances pc.
    void ExecuteInstruction(ExecutionContext& ctx) const;
};

} // namespace NF

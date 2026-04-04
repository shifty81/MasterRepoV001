#pragma once
// Blackboard.h — Shared key-value data store for AI agents.
//
// A Blackboard is a shared data bus that allows multiple AI subsystems
// (perception, navigation, combat, dialogue) to communicate without
// direct coupling.  Each agent has a private Blackboard; factions share
// a single FactionBlackboard.

#include "Engine/NodeGraph/GraphVM.h"
#include <string>
#include <unordered_map>

namespace NF {

/// @brief Generic key-value data store for AI communication.
///
/// Values are stored as @c GraphValue (variant of bool/int/float/string/Vec2/Vec3).
/// Reading a missing key returns a null GraphValue.
class Blackboard {
public:
    // ---- Write ---------------------------------------------------------------

    void Set(const std::string& key, GraphValue value) {
        m_Data[key] = std::move(value);
    }

    void Clear(const std::string& key) {
        m_Data.erase(key);
    }

    void ClearAll() {
        m_Data.clear();
    }

    // ---- Read ----------------------------------------------------------------

    [[nodiscard]] const GraphValue& Get(const std::string& key) const noexcept {
        static const GraphValue kNull{};
        auto it = m_Data.find(key);
        return it != m_Data.end() ? it->second : kNull;
    }

    [[nodiscard]] bool Has(const std::string& key) const noexcept {
        return m_Data.count(key) > 0;
    }

    // ---- Typed helpers -------------------------------------------------------

    [[nodiscard]] bool GetBool(const std::string& key, bool defaultVal = false) const noexcept {
        auto it = m_Data.find(key);
        if (it == m_Data.end()) return defaultVal;
        if (auto* v = std::get_if<bool>(&it->second)) return *v;
        return defaultVal;
    }

    [[nodiscard]] int32_t GetInt(const std::string& key, int32_t defaultVal = 0) const noexcept {
        auto it = m_Data.find(key);
        if (it == m_Data.end()) return defaultVal;
        if (auto* v = std::get_if<int32_t>(&it->second)) return *v;
        return defaultVal;
    }

    [[nodiscard]] float GetFloat(const std::string& key, float defaultVal = 0.f) const noexcept {
        auto it = m_Data.find(key);
        if (it == m_Data.end()) return defaultVal;
        if (auto* v = std::get_if<float>(&it->second)) return *v;
        return defaultVal;
    }

    [[nodiscard]] std::string GetString(const std::string& key, std::string defaultVal = {}) const {
        auto it = m_Data.find(key);
        if (it == m_Data.end()) return defaultVal;
        if (auto* v = std::get_if<std::string>(&it->second)) return *v;
        return defaultVal;
    }

private:
    std::unordered_map<std::string, GraphValue> m_Data;
};

} // namespace NF

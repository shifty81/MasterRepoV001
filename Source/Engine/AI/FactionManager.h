#pragma once
// FactionManager.h — Tracks inter-faction and player-faction reputation.
//
// Reputation values are integers in [-1000, +1000].
//   < -500  = hostile (attack on sight)
//   -500..0 = unfriendly
//   0..+500 = neutral
//   > +500  = friendly (access to faction services)

#include <cstdint>
#include <string>
#include <unordered_map>

namespace NF {

/// @brief Inter-faction and entity-faction reputation store.
///
/// Reputation changes are applied symmetrically by default:
/// if the player helps Faction A, Faction A's rivals gain negative reputation.
class FactionManager {
public:
    static constexpr int32_t kMinReputation  = -1000;
    static constexpr int32_t kMaxReputation  = +1000;
    static constexpr int32_t kNeutral        =  0;
    static constexpr int32_t kHostileThresh  = -500;
    static constexpr int32_t kFriendlyThresh = +500;

    // ---- Query ---------------------------------------------------------------

    /// @brief Get reputation between two factions (or entity ID and faction).
    [[nodiscard]] int32_t GetReputation(const std::string& factionA,
                                        const std::string& factionB) const noexcept;

    /// @brief True if factionA is hostile to factionB.
    [[nodiscard]] bool IsHostile(const std::string& factionA,
                                 const std::string& factionB) const noexcept;

    /// @brief True if factionA is friendly with factionB.
    [[nodiscard]] bool IsFriendly(const std::string& factionA,
                                  const std::string& factionB) const noexcept;

    // ---- Mutation ------------------------------------------------------------

    /// @brief Apply a delta to the reputation between two factions.
    /// @param clamp If true, clamps to [kMinReputation, kMaxReputation].
    void ApplyReputation(const std::string& factionA,
                         const std::string& factionB,
                         int32_t delta,
                         bool clamp = true);

    /// @brief Set an explicit reputation value.
    void SetReputation(const std::string& factionA,
                       const std::string& factionB,
                       int32_t value);

    /// @brief Declare two factions as inherently allied or at war.
    void SetAlliance(const std::string& factionA, const std::string& factionB);
    void SetWar(const std::string& factionA, const std::string& factionB);

    // ---- Serialization -------------------------------------------------------

    void Clear();

private:
    // Key: "factionA|factionB" (alphabetically sorted to avoid duplicates)
    std::unordered_map<std::string, int32_t> m_Reputation;

    static std::string MakeKey(const std::string& a, const std::string& b);
};

} // namespace NF

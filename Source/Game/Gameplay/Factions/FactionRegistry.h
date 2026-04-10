#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace NF::Game::Gameplay {

// ---------------------------------------------------------------------------
// Faction types
// ---------------------------------------------------------------------------

/// @brief Identifier for a built-in faction.
enum class FactionId : uint8_t {
    MinersGuild  = 0,
    TradersUnion = 1,
    Raiders      = 2,
};

static constexpr int kFactionCount = 3;

/// @brief Descriptive string for a player's standing with a faction.
enum class FactionStanding : uint8_t {
    Hostile,   ///< rep < -25
    Neutral,   ///< -25 <= rep < 25
    Friendly,  ///< 25 <= rep < 75
    Allied,    ///< rep >= 75
};

/// @brief Runtime state for one faction.
struct FactionState {
    FactionId   id{FactionId::MinersGuild};
    std::string name;
    int         reputation{0};   ///< Clamped to [-100, 100].
};

// ---------------------------------------------------------------------------
// FactionRegistry
// ---------------------------------------------------------------------------

/// @brief Tracks player reputation with the three built-in factions.
///
/// Reputation changes fire from Notify*() helpers called by the Orchestrator.
/// Each faction starts at 0 (Neutral).  The range is clamped to [-100, 100].
///
/// Standing thresholds:
///   rep < -25  → Hostile
///   -25 ≤ rep < 25 → Neutral
///   25 ≤ rep < 75  → Friendly
///   rep >= 75      → Allied
class FactionRegistry {
public:
    FactionRegistry();

    // ---- Lifecycle ----------------------------------------------------------

    /// @brief Reset all reputations to 0 and register the three starter factions.
    void Init();

    // ---- Event notifications ------------------------------------------------

    /// @brief Called when the player mines voxels.
    ///        Awards +1 Miners Guild reputation per call.
    void NotifyMined();

    /// @brief Called when the player sells resources at a station.
    ///        Awards +1 Traders Union reputation per call.
    void NotifySold();

    /// @brief Called when the player kills an enemy.
    ///        Deducts 5 Raiders reputation (improving standing vs Raiders).
    void NotifyKill();

    /// @brief Called when the player investigates an anomaly.
    ///        Awards +2 Traders Union reputation (anomaly intel has trade value).
    void NotifyInvestigated();

    // ---- Reputation management ----------------------------------------------

    /// @brief Directly adjust reputation for a faction by @p delta.
    ///        Clamped to [-100, 100].
    void AddRep(FactionId faction, int delta);

    // ---- Queries ------------------------------------------------------------

    /// @brief Return the current reputation in [-100, 100] for a faction.
    [[nodiscard]] int GetRep(FactionId faction) const noexcept;

    /// @brief Return the human-readable standing for a faction.
    [[nodiscard]] FactionStanding GetStanding(FactionId faction) const noexcept;

    /// @brief Convert a standing value to a display string.
    [[nodiscard]] static const char* StandingName(FactionStanding s) noexcept;

    /// @brief Read-only access to all faction states (for UI panels).
    [[nodiscard]] const std::vector<FactionState>& GetFactions() const noexcept {
        return m_Factions;
    }

private:
    std::vector<FactionState> m_Factions;

    FactionState* Find(FactionId id) noexcept;
    const FactionState* Find(FactionId id) const noexcept;
};

} // namespace NF::Game::Gameplay

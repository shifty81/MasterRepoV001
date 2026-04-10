#pragma once
#include "Core/Math/Vector.h"
#include "Game/Interaction/ResourceItem.h"
#include "Game/Interaction/Inventory.h"
#include <cstdint>
#include <string>
#include <vector>
#include <functional>

namespace NF::Game::Gameplay {

// ---------------------------------------------------------------------------
// Anomaly types
// ---------------------------------------------------------------------------

/// @brief Classification of a space anomaly.
enum class AnomalyType : uint8_t {
    Derelict,   ///< Abandoned structure — salvage rewards.
    Mineral,    ///< Rich mineral pocket — resource rewards.
    Signal,     ///< Mysterious signal — intelligence / credits.
    Radiation,  ///< Irradiated zone — hazard + rare loot.
};

/// @brief Opaque identifier for a placed anomaly.
using AnomalyId = uint32_t;
static constexpr AnomalyId kInvalidAnomalyId = 0;

/// @brief Loot entry inside an anomaly.
struct AnomalyLootEntry {
    NF::Game::ResourceType type{NF::Game::ResourceType::None};
    uint32_t               count{0};
};

/// @brief Runtime state for a single anomaly site.
struct Anomaly {
    AnomalyId                    id{kInvalidAnomalyId};
    std::string                  name;
    NF::Vector3                  position;
    AnomalyType                  type{AnomalyType::Signal};
    std::vector<AnomalyLootEntry> loot;
    bool                         investigated{false}; ///< True once investigated.
};

// ---------------------------------------------------------------------------
// AnomalySystem
// ---------------------------------------------------------------------------

/// @brief Manages space anomaly encounter sites in the game world.
///
/// Anomalies are placed via PlaceAnomaly() and persist for the session.
/// Investigate() extracts loot from the anomaly and marks it investigated.
///
/// Phase 10 seeds one default anomaly ("Strange Signal") at Init time via
/// the Orchestrator.
class AnomalySystem {
public:
    AnomalySystem()  = default;
    ~AnomalySystem() = default;

    // ---- Placement ----------------------------------------------------------

    /// @brief Register an anomaly site and return its id.
    AnomalyId PlaceAnomaly(const std::string& name,
                            const NF::Vector3& position,
                            AnomalyType type = AnomalyType::Signal);

    /// @brief Add a loot entry to an anomaly.
    void AddLoot(AnomalyId id, NF::Game::ResourceType type, uint32_t count);

    // ---- Interaction --------------------------------------------------------

    /// @brief Find the nearest un-investigated anomaly within @p radius units.
    /// @return kInvalidAnomalyId if none found.
    [[nodiscard]] AnomalyId FindNearest(const NF::Vector3& pos, float radius) const noexcept;

    /// @brief Investigate an anomaly — deposits loot into @p inventory,
    ///        marks the anomaly investigated, and fires the callback.
    /// @return Number of items extracted (0 if already investigated or invalid).
    uint32_t Investigate(AnomalyId id, NF::Game::Inventory& inventory);

    // ---- Callbacks ----------------------------------------------------------

    /// @brief Optional callback fired after a successful investigation.
    ///        Receives the anomaly id and the number of items extracted.
    void SetOnInvestigated(std::function<void(AnomalyId, uint32_t)> cb) {
        m_OnInvestigated = std::move(cb);
    }

    // ---- Queries ------------------------------------------------------------

    /// @brief Read-only access to all anomaly sites.
    [[nodiscard]] const std::vector<Anomaly>& GetAnomalies() const noexcept {
        return m_Anomalies;
    }

    /// @brief Retrieve a specific anomaly by id (nullptr if not found).
    [[nodiscard]] const Anomaly* Get(AnomalyId id) const noexcept;

    /// @brief Human-readable type name for display.
    [[nodiscard]] static const char* TypeName(AnomalyType t) noexcept;

private:
    AnomalyId             m_NextId{1};
    std::vector<Anomaly>  m_Anomalies;

    std::function<void(AnomalyId, uint32_t)> m_OnInvestigated;

    Anomaly* FindMutable(AnomalyId id) noexcept;
};

} // namespace NF::Game::Gameplay


#pragma once
#include "Game/Interaction/ResourceItem.h"
#include "Game/Interaction/Inventory.h"
#include "Core/Math/Vector.h"
#include <cstdint>
#include <string>
#include <vector>

namespace NF::Game::Gameplay {

/// @brief Unique handle for a world-placed wreck site.
using WreckId = uint32_t;

/// @brief Sentinel for an invalid wreck ID.
inline constexpr WreckId kInvalidWreckId = 0u;

/// @brief A single resource stack available in a wreck.
struct WreckLoot {
    NF::Game::ResourceType type{NF::Game::ResourceType::None};
    uint32_t               count{0};
};

static constexpr int kMaxWreckLoot = 4; ///< Max distinct resource types per wreck.

/// @brief A destroyed ship / structure that can be salvaged for resources.
struct WreckSite {
    WreckId     id{kInvalidWreckId};
    std::string name;
    NF::Vector3 position{};

    /// @brief Remaining loot in the wreck.  Counts decrease as the player salvages.
    WreckLoot   loot[kMaxWreckLoot]{};
    int         lootCount{0};      ///< Number of distinct loot entries.

    /// @brief Returns true when all loot has been extracted.
    [[nodiscard]] bool IsEmpty() const noexcept;

    /// @brief Total number of items remaining across all loot stacks.
    [[nodiscard]] uint32_t TotalLoot() const noexcept;
};

/// @brief Manages world-placed wreck sites that can be salvaged for resources.
///
/// A wreck has a position and a pool of resource loot.  Each call to
/// @c Salvage extracts one batch from the wreck into the player's inventory.
/// When a wreck is fully looted it becomes empty (but is not removed — callers
/// may choose to hide or delete it).
///
/// The Orchestrator places a default wreck at session start and exposes
/// @c NotifySalvaged() to MissionRegistry.
class SalvageSystem {
public:
    SalvageSystem()  = default;
    ~SalvageSystem() = default;

    // -------------------------------------------------------------------------
    // Wreck management
    // -------------------------------------------------------------------------

    /// @brief Place a wreck at @p position and return its ID.
    WreckId PlaceWreck(const std::string& name, const NF::Vector3& position);

    /// @brief Add loot to an existing wreck.
    /// @return False if the wreck has no room for another loot type.
    bool AddLoot(WreckId id, NF::Game::ResourceType type, uint32_t count);

    /// @brief Remove a wreck (e.g. after it is fully looted and cleaned up).
    bool RemoveWreck(WreckId id);

    // -------------------------------------------------------------------------
    // Salvage
    // -------------------------------------------------------------------------

    /// @brief Find the nearest wreck to @p position within @p radius.
    /// @return ID of the nearest non-empty wreck, or kInvalidWreckId if none.
    [[nodiscard]] WreckId FindNearest(const NF::Vector3& position,
                                      float radius) const noexcept;

    /// @brief Extract one batch of loot from the wreck into @p inv.
    ///
    /// Extracts all remaining resources from the first non-empty loot stack
    /// (up to @p maxPerBatch units).  Returns the number of items actually
    /// extracted (0 if wreck is empty or invalid).
    uint32_t Salvage(WreckId id, NF::Game::Inventory& inv,
                     uint32_t maxPerBatch = 5);

    // -------------------------------------------------------------------------
    // Queries
    // -------------------------------------------------------------------------

    [[nodiscard]] const WreckSite* GetWreck(WreckId id) const noexcept;
    [[nodiscard]] WreckSite*       GetWreck(WreckId id)       noexcept;

    [[nodiscard]] const std::vector<WreckSite>& GetWrecks() const noexcept {
        return m_Wrecks;
    }

    [[nodiscard]] int WreckCount() const noexcept {
        return static_cast<int>(m_Wrecks.size());
    }

private:
    std::vector<WreckSite> m_Wrecks;
    WreckId                m_NextId{1};
};

} // namespace NF::Game::Gameplay

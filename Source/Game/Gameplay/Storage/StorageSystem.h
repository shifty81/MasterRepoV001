#pragma once
#include "Game/Interaction/Inventory.h"
#include "Game/Interaction/ResourceItem.h"
#include "Core/Math/Vector.h"
#include <cstdint>
#include <string>
#include <vector>

namespace NF::Game::Gameplay {

/// @brief Unique handle for a world-placed storage box.
using StorageBoxId = uint32_t;

/// @brief Sentinel value for an invalid / not-found storage box.
inline constexpr StorageBoxId kInvalidBoxId = 0;

/// @brief Persistent storage containers and logistics.
///
/// Each storage box has a world-space position, a name, and its own
/// Inventory.  The system supports spatial queries (FindNearest) and
/// item transfer helpers (Deposit / Withdraw).
class StorageSystem {
public:
    StorageSystem()  = default;
    ~StorageSystem() = default;

    // -------------------------------------------------------------------------
    // Box management
    // -------------------------------------------------------------------------

    /// @brief Place a new storage box in the world.
    /// @return Unique StorageBoxId.
    StorageBoxId AddBox(const std::string& name, const NF::Vector3& position);

    /// @brief Return a pointer to the box's inventory, or nullptr.
    [[nodiscard]] NF::Game::Inventory* GetBox(StorageBoxId id) noexcept;

    /// @brief Find the nearest storage box to @p position within @p radius.
    /// @return StorageBoxId of the closest box, or kInvalidBoxId if none.
    [[nodiscard]] StorageBoxId FindNearest(const NF::Vector3& position,
                                           float radius) const noexcept;

    // -------------------------------------------------------------------------
    // Transfer helpers
    // -------------------------------------------------------------------------

    /// @brief Move items from the player inventory into a storage box.
    bool Deposit(StorageBoxId boxId, NF::Game::Inventory& playerInv,
                 NF::Game::ResourceType type, uint32_t count);

    /// @brief Move items from a storage box into the player inventory.
    bool Withdraw(StorageBoxId boxId, NF::Game::Inventory& playerInv,
                  NF::Game::ResourceType type, uint32_t count);

private:
    struct BoxEntry {
        StorageBoxId         id{0};
        std::string          name;
        NF::Vector3          position{};
        NF::Game::Inventory  inventory;
    };

    std::vector<BoxEntry> m_Boxes;
    StorageBoxId          m_NextId{1};

    [[nodiscard]] BoxEntry*       FindEntry(StorageBoxId id) noexcept;
    [[nodiscard]] const BoxEntry* FindEntry(StorageBoxId id) const noexcept;
};

} // namespace NF::Game::Gameplay

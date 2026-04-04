#pragma once
#include "Game/Interaction/Inventory.h"
#include "Game/Interaction/ResourceItem.h"
#include <cstdint>
#include <string>
#include <vector>

namespace NF::Game::Gameplay {

/// @brief Unique handle for a managed inventory container.
using ContainerId = uint32_t;

/// @brief Sentinel value for an invalid / not-found container.
inline constexpr ContainerId kInvalidContainer = 0;

/// @brief Full inventory management system (complements Interaction/Inventory).
///
/// Manages named Inventory containers (e.g. "Backpack", "Chest") with
/// unique IDs.  Supports inter-container transfers and lookup by name.
class InventorySystem {
public:
    InventorySystem()  = default;
    ~InventorySystem() = default;

    // -------------------------------------------------------------------------
    // Container management
    // -------------------------------------------------------------------------

    /// @brief Create a new named container.
    /// @return Unique ContainerId (never kInvalidContainer).
    ContainerId AddContainer(const std::string& name);

    /// @brief Return a pointer to the container's inventory, or nullptr.
    [[nodiscard]] NF::Game::Inventory* GetContainer(ContainerId id) noexcept;

    /// @brief Find a container by name (case-sensitive).
    /// @return The container's ID, or kInvalidContainer if not found.
    [[nodiscard]] ContainerId FindContainer(const std::string& name) const noexcept;

    /// @brief Transfer @p count of @p type from @p src to @p dst.
    /// @return True on success, false if src lacks items or containers invalid.
    bool Transfer(ContainerId src, ContainerId dst,
                  NF::Game::ResourceType type, uint32_t count);

    /// @brief Remove a container by ID.
    /// @return True if found and removed.
    bool RemoveContainer(ContainerId id);

    /// @brief Number of currently managed containers.
    [[nodiscard]] int ContainerCount() const noexcept {
        return static_cast<int>(m_Containers.size());
    }

private:
    struct ContainerEntry {
        ContainerId          id{0};
        std::string          name;
        NF::Game::Inventory  inventory;
    };

    std::vector<ContainerEntry> m_Containers;
    ContainerId                 m_NextId{1};

    [[nodiscard]] ContainerEntry*       FindEntry(ContainerId id) noexcept;
    [[nodiscard]] const ContainerEntry* FindEntry(ContainerId id) const noexcept;
};

} // namespace NF::Game::Gameplay

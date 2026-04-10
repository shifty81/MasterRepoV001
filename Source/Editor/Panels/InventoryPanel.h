#pragma once
#include "Game/Gameplay/Salvage/SalvageSystem.h"
#include "Game/Gameplay/Storage/StorageSystem.h"
#include "Game/Gameplay/Inventory/InventorySystem.h"
#include "Game/Interaction/Inventory.h"

namespace NF { class UIRenderer; }

namespace NF::Editor {

/// @brief Editor panel showing salvage sites, storage boxes, and inventory containers.
///
/// Provides a read-only view of:
///   - The player's backpack (from InventorySystem "Backpack" container)
///   - All world storage boxes and their contents
///   - All active wreck sites and their remaining loot
///
/// Wired to live Orchestrator-owned systems; pointers may be nullptr in
/// read-only / unloaded mode.
class InventoryPanel {
public:
    InventoryPanel() = default;

    // -------------------------------------------------------------------------
    // Wiring
    // -------------------------------------------------------------------------

    void SetUIRenderer(NF::UIRenderer* r) noexcept { m_Renderer = r; }

    void SetInventorySystem(NF::Game::Gameplay::InventorySystem* inv) noexcept {
        m_InventorySys = inv;
    }
    void SetStorageSystem(NF::Game::Gameplay::StorageSystem* storage) noexcept {
        m_Storage = storage;
    }
    void SetSalvageSystem(NF::Game::Gameplay::SalvageSystem* salvage) noexcept {
        m_Salvage = salvage;
    }
    /// @brief Optional: pointer to the player's live backpack inventory
    ///        (typically the InteractionLoop inventory).
    void SetPlayerInventory(const NF::Game::Inventory* playerInv) noexcept {
        m_PlayerInv = playerInv;
    }

    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------

    void Update(float dt) noexcept;
    void Draw(float x, float y, float w, float h);

private:
    NF::UIRenderer*                             m_Renderer{nullptr};
    NF::Game::Gameplay::InventorySystem*        m_InventorySys{nullptr};
    NF::Game::Gameplay::StorageSystem*          m_Storage{nullptr};
    NF::Game::Gameplay::SalvageSystem*          m_Salvage{nullptr};
    const NF::Game::Inventory*                  m_PlayerInv{nullptr};

    void DrawBackpack(float x, float& cy, [[maybe_unused]] float w, float maxY);
    void DrawStorageBoxes(float x, float& cy, [[maybe_unused]] float w, float maxY);
    void DrawWreckSites(float x, float& cy, [[maybe_unused]] float w, float maxY);
    void DrawInventory(const NF::Game::Inventory& inv,
                       float x, float& cy, [[maybe_unused]] float w, float maxY);
};

} // namespace NF::Editor

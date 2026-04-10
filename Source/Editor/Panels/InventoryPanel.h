#pragma once
#include "Game/Gameplay/Salvage/SalvageSystem.h"
#include "Game/Gameplay/Storage/StorageSystem.h"
#include "Game/Gameplay/Inventory/InventorySystem.h"
#include "Game/Interaction/Inventory.h"
#include "Game/Interaction/ResourceItem.h"

namespace NF { class UIRenderer; }

namespace NF::Editor {

struct EditorInputState;

/// @brief Editor panel showing salvage sites, storage boxes, and inventory containers.
///
/// Provides a live view of:
///   - The player's backpack (from InventorySystem "Backpack" container)
///   - All world storage boxes and their contents
///   - All active wreck sites and their remaining loot
///
/// When an input state and mutable player inventory are provided the panel is
/// interactive: the author can give test items directly from the editor,
/// transfer the backpack to storage, and withdraw items from storage.
class InventoryPanel {
public:
    InventoryPanel() = default;

    // -------------------------------------------------------------------------
    // Wiring
    // -------------------------------------------------------------------------

    void SetUIRenderer(NF::UIRenderer* r) noexcept { m_Renderer = r; }

    /// @brief Provide live input for interactive buttons.
    void SetInputState(const EditorInputState* input) noexcept { m_Input = input; }

    void SetInventorySystem(NF::Game::Gameplay::InventorySystem* inv) noexcept {
        m_InventorySys = inv;
    }
    void SetStorageSystem(NF::Game::Gameplay::StorageSystem* storage) noexcept {
        m_Storage = storage;
    }
    void SetSalvageSystem(NF::Game::Gameplay::SalvageSystem* salvage) noexcept {
        m_Salvage = salvage;
    }
    /// @brief Optional: mutable pointer to the player's live backpack inventory
    ///        (typically the InteractionLoop inventory).  Required for interactive
    ///        Give / Transfer buttons.
    void SetPlayerInventory(NF::Game::Inventory* playerInv) noexcept {
        m_PlayerInv = playerInv;
    }

    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------

    void Update(float dt) noexcept;
    void Draw(float x, float y, float w, float h);

private:
    NF::UIRenderer*                             m_Renderer{nullptr};
    const EditorInputState*                     m_Input{nullptr};
    NF::Game::Gameplay::InventorySystem*        m_InventorySys{nullptr};
    NF::Game::Gameplay::StorageSystem*          m_Storage{nullptr};
    NF::Game::Gameplay::SalvageSystem*          m_Salvage{nullptr};
    NF::Game::Inventory*                        m_PlayerInv{nullptr};

    /// @brief Draw a clickable button; returns true on the frame it was clicked.
    bool DrawButton(float x, float y, float w, float h, const char* label,
                    bool enabled = true);

    void DrawBackpack(float x, float& cy, float w, float maxY);
    void DrawStorageBoxes(float x, float& cy, float w, float maxY);
    void DrawWreckSites(float x, float& cy, [[maybe_unused]] float w, float maxY);
    void DrawInventoryReadOnly(const NF::Game::Inventory& inv,
                               float x, float& cy, float w, float maxY);
};

} // namespace NF::Editor

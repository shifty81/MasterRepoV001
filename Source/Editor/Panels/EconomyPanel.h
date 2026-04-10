#pragma once
#include "Game/Gameplay/Economy/ResourceRegistry.h"
#include "Game/Gameplay/Economy/TradeMarket.h"
#include "Game/Gameplay/Station/StationServices.h"
#include "Game/Interaction/Inventory.h"

namespace NF { class UIRenderer; }

namespace NF::Editor {

struct EditorInputState;

/// @brief Editor panel that displays and interacts with the live economy state.
///
/// Shows the ResourceRegistry catalog, current TradeMarket prices and stock,
/// player credits, station status, and pending manufacturing jobs.
///
/// When a player inventory and input state are provided the panel is fully
/// interactive: the player can dock/undock, buy/sell resources one unit at a
/// time, and enqueue crafting jobs — all from within the editor.
class EconomyPanel {
public:
    EconomyPanel() = default;

    // -------------------------------------------------------------------------
    // Wiring
    // -------------------------------------------------------------------------

    void SetUIRenderer(NF::UIRenderer* r) noexcept { m_Renderer = r; }

    /// @brief Provide live input for interactive buttons.
    void SetInputState(const EditorInputState* input) noexcept { m_Input = input; }

    /// @brief Provide live station data.  May be nullptr for read-only mode.
    void SetStation(NF::Game::Gameplay::StationServices* station) noexcept {
        m_Station = station;
    }

    /// @brief Provide the static resource registry.
    void SetRegistry(const NF::Game::Gameplay::ResourceRegistry* reg) noexcept {
        m_Registry = reg;
    }

    /// @brief Provide the mutable player inventory used for Buy/Sell/Craft.
    ///        When nullptr the market table is read-only.
    void SetPlayerInventory(NF::Game::Inventory* inv) noexcept {
        m_PlayerInventory = inv;
    }

    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------

    void Update(float dt) noexcept;
    void Draw(float x, float y, float w, float h);

private:
    NF::UIRenderer*                                m_Renderer{nullptr};
    const EditorInputState*                        m_Input{nullptr};
    NF::Game::Gameplay::StationServices*           m_Station{nullptr};
    const NF::Game::Gameplay::ResourceRegistry*    m_Registry{nullptr};
    NF::Game::Inventory*                           m_PlayerInventory{nullptr};

    float m_ScrollOffset{0.f};  ///< Vertical scroll position (reserved).

    /// @brief Draw a clickable button; returns true on the frame it was clicked.
    bool DrawButton(float x, float y, float w, float h, const char* label,
                    bool enabled = true);

    void DrawHeader(float x, float& cy, float w);
    void DrawMarketTable(float x, float& cy, float w, float maxY);
    void DrawManufacturing(float x, float& cy, float w, float maxY);
};

} // namespace NF::Editor

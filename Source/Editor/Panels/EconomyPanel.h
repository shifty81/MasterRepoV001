#pragma once
#include "Game/Gameplay/Economy/ResourceRegistry.h"
#include "Game/Gameplay/Economy/TradeMarket.h"
#include "Game/Gameplay/Station/StationServices.h"

namespace NF { class UIRenderer; }

namespace NF::Editor {

/// @brief Editor panel that displays live economy state.
///
/// Shows the ResourceRegistry catalog, current TradeMarket prices and stock,
/// player credits, station status, and pending manufacturing jobs.
/// Wired to an optional live StationServices pointer set by the owning
/// EditorApp.  When no station is available, the panel shows the static
/// resource registry in read-only mode.
class EconomyPanel {
public:
    EconomyPanel() = default;

    // -------------------------------------------------------------------------
    // Wiring
    // -------------------------------------------------------------------------

    void SetUIRenderer(NF::UIRenderer* r) noexcept { m_Renderer = r; }

    /// @brief Provide live station data.  May be nullptr for read-only mode.
    void SetStation(NF::Game::Gameplay::StationServices* station) noexcept {
        m_Station = station;
    }

    /// @brief Provide the static resource registry.
    void SetRegistry(const NF::Game::Gameplay::ResourceRegistry* reg) noexcept {
        m_Registry = reg;
    }

    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------

    void Update(float dt) noexcept;
    void Draw(float x, float y, float w, float h);

private:
    NF::UIRenderer*                                m_Renderer{nullptr};
    NF::Game::Gameplay::StationServices*           m_Station{nullptr};
    const NF::Game::Gameplay::ResourceRegistry*    m_Registry{nullptr};

    float m_ScrollOffset{0.f};  ///< Vertical scroll position (reserved).

    void DrawHeader(float x, float& cy, float w);
    void DrawMarketTable(float x, float& cy, float w, float maxY);
    void DrawManufacturing(float x, float& cy, float w, float maxY);
};

} // namespace NF::Editor

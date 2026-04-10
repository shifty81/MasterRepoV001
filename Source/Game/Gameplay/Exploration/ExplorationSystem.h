#pragma once
// ExplorationSystem.h — Skeleton for body-to-body travel state.
//
// Phase 5 implementation: owns the transit state machine that tracks which
// celestial body the player is currently on and manages body-switch requests.
// Full atmospheric simulation and travel duration are deferred.

#include <cstdint>
#include <functional>

namespace NF::Game::Gameplay {

/// @brief Lightweight skeleton for the inter-body travel system.
///
/// In Phase 5 the system just tracks which body the player is on and signals
/// the EditorWorldSession when a travel request completes.  Future phases will
/// add flight paths, fuel, and loading screens.
class ExplorationSystem {
public:
    ExplorationSystem() = default;

    // ---- Configuration -------------------------------------------------------

    /// @brief Callback fired when travel to a new body completes.
    ///        Receives the destination body ID.
    using TravelCallback = std::function<void(uint32_t destBodyId)>;

    /// @brief Register the callback that is invoked after a successful travel.
    void SetOnTravelComplete(TravelCallback cb) noexcept {
        m_OnTravelComplete = std::move(cb);
    }

    // ---- State ---------------------------------------------------------------

    /// @brief Request travel to the body with the given ID.
    ///        Returns true if the travel was accepted (source != destination).
    bool TravelToBody(uint32_t bodyId);

    /// @brief Return the ID of the body the player is currently on.
    ///        Returns 0 when no body has been set (start state / star).
    [[nodiscard]] uint32_t GetCurrentBodyId() const noexcept { return m_CurrentBodyId; }

    /// @brief Return the ID of the previous body (0 if none).
    [[nodiscard]] uint32_t GetPreviousBodyId() const noexcept { return m_PreviousBodyId; }

    /// @brief True if a travel is currently in progress (reserved for future async travel).
    [[nodiscard]] bool IsTraveling() const noexcept { return m_Traveling; }

    /// @brief Directly set the current body (e.g. on world load).
    void SetCurrentBodyId(uint32_t id) noexcept { m_CurrentBodyId = id; }

private:
    uint32_t       m_CurrentBodyId{0};
    uint32_t       m_PreviousBodyId{0};
    bool           m_Traveling{false};
    TravelCallback m_OnTravelComplete;
};

} // namespace NF::Game::Gameplay

#pragma once
#include "Game/Gameplay/Economy/TradeMarket.h"
#include "Game/Gameplay/Manufacturing/ManufacturingQueue.h"
#include "Game/Interaction/RigState.h"
#include <string>

namespace NF::Game::Gameplay {

/// @brief Result of a station service operation.
enum class StationServiceResult : uint8_t {
    Success,
    NotDocked,
    AlreadyDocked,
    NotDocked_,       ///< Alias kept for internal use — not used in API.
    AlreadyFullHealth,
    AlreadyFullEnergy,
    InsufficientFunds,
};

/// @brief Station docking, refuelling, repair, and manufacturing services.
///
/// A station owns a TradeMarket for credit-based pricing and a
/// ManufacturingQueue for on-station crafting.  Services (Repair / Refuel)
/// are only available while docked.
class StationServices {
public:
    explicit StationServices(std::string name);
    ~StationServices() = default;

    // -------------------------------------------------------------------------
    // Docking
    // -------------------------------------------------------------------------

    [[nodiscard]] bool IsDocked() const noexcept { return m_Docked; }

    StationServiceResult Dock() noexcept;
    StationServiceResult Undock() noexcept;

    // -------------------------------------------------------------------------
    // Services (require docked state)
    // -------------------------------------------------------------------------

    /// @brief Repair the rig to full health. Costs credits from the market.
    StationServiceResult Repair(NF::Game::RigState& rig);

    /// @brief Refuel the rig to full energy. Costs credits from the market.
    StationServiceResult Refuel(NF::Game::RigState& rig);

    // -------------------------------------------------------------------------
    // Tick
    // -------------------------------------------------------------------------

    /// @brief Advance the station's manufacturing queue by @p dt seconds.
    void Tick(float dt) noexcept;

    // -------------------------------------------------------------------------
    // Accessors
    // -------------------------------------------------------------------------

    [[nodiscard]] const std::string& GetName() const noexcept { return m_Name; }
    [[nodiscard]] TradeMarket&       GetMarket()   noexcept   { return m_Market; }
    [[nodiscard]] const TradeMarket& GetMarket()   const noexcept { return m_Market; }
    [[nodiscard]] ManufacturingQueue& GetFactory() noexcept   { return m_Factory; }
    [[nodiscard]] const ManufacturingQueue& GetFactory() const noexcept { return m_Factory; }

private:
    static constexpr float kRepairCostPerHp     = 0.5f;  ///< Credits per HP restored.
    static constexpr float kRefuelCostPerUnit    = 0.3f;  ///< Credits per energy unit.

    std::string        m_Name;
    bool               m_Docked{false};
    TradeMarket        m_Market{};
    ManufacturingQueue m_Factory{};
};

} // namespace NF::Game::Gameplay

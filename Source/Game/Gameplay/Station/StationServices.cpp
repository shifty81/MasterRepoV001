#include "Game/Gameplay/Station/StationServices.h"
#include <algorithm>
#include <utility>

namespace NF::Game::Gameplay {

StationServices::StationServices(std::string name)
    : m_Name(std::move(name))
{
}

// ---------------------------------------------------------------------------
// Docking
// ---------------------------------------------------------------------------

StationServiceResult StationServices::Dock() noexcept
{
    if (m_Docked) return StationServiceResult::AlreadyDocked;
    m_Docked = true;
    return StationServiceResult::Success;
}

StationServiceResult StationServices::Undock() noexcept
{
    if (!m_Docked) return StationServiceResult::NotDocked;
    m_Docked = false;
    return StationServiceResult::Success;
}

// ---------------------------------------------------------------------------
// Services
// ---------------------------------------------------------------------------

StationServiceResult StationServices::Repair(NF::Game::RigState& rig)
{
    if (!m_Docked) return StationServiceResult::NotDocked;

    const float missing = NF::Game::RigState::kMaxHealth - rig.GetHealth();
    if (missing <= 0.f) return StationServiceResult::AlreadyFullHealth;

    const float cost = missing * kRepairCostPerHp;
    if (m_Market.GetCredits() < cost)
        return StationServiceResult::InsufficientFunds;

    m_Market.SetCredits(m_Market.GetCredits() - cost);
    rig.RepairHealth(missing);
    return StationServiceResult::Success;
}

StationServiceResult StationServices::Refuel(NF::Game::RigState& rig)
{
    if (!m_Docked) return StationServiceResult::NotDocked;

    const float missing = NF::Game::RigState::kMaxEnergy - rig.GetEnergy();
    if (missing <= 0.f) return StationServiceResult::AlreadyFullEnergy;

    const float cost = missing * kRefuelCostPerUnit;
    if (m_Market.GetCredits() < cost)
        return StationServiceResult::InsufficientFunds;

    m_Market.SetCredits(m_Market.GetCredits() - cost);
    rig.RechargeEnergy(missing);
    return StationServiceResult::Success;
}

// ---------------------------------------------------------------------------
// Tick
// ---------------------------------------------------------------------------

void StationServices::Tick(float dt) noexcept
{
    m_Factory.Tick(dt);
}

} // namespace NF::Game::Gameplay

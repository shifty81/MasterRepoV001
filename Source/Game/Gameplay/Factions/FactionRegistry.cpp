#include "Game/Gameplay/Factions/FactionRegistry.h"
#include <algorithm>

namespace NF::Game::Gameplay {

// ---------------------------------------------------------------------------
// Construction / Init
// ---------------------------------------------------------------------------

FactionRegistry::FactionRegistry()
{
    Init();
}

void FactionRegistry::Init()
{
    m_Factions.clear();
    m_Factions.push_back({FactionId::MinersGuild,  "Miners Guild",  0});
    m_Factions.push_back({FactionId::TradersUnion, "Traders Union", 0});
    m_Factions.push_back({FactionId::Raiders,      "Raiders",       0});
}

// ---------------------------------------------------------------------------
// Event notifications
// ---------------------------------------------------------------------------

void FactionRegistry::NotifyMined()
{
    AddRep(FactionId::MinersGuild, 1);
}

void FactionRegistry::NotifySold()
{
    AddRep(FactionId::TradersUnion, 1);
}

void FactionRegistry::NotifyKill()
{
    // Defeating raiders lowers their standing score toward Hostile,
    // which is modelled as -5 to the Raiders rep.
    AddRep(FactionId::Raiders, -5);
}

void FactionRegistry::NotifyInvestigated()
{
    // Anomaly intel has trade value — Traders Union appreciates it.
    AddRep(FactionId::TradersUnion, 2);
}

// ---------------------------------------------------------------------------
// Reputation management
// ---------------------------------------------------------------------------

void FactionRegistry::AddRep(FactionId faction, int delta)
{
    FactionState* fs = Find(faction);
    if (!fs) return;
    fs->reputation = std::clamp(fs->reputation + delta, -100, 100);
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

int FactionRegistry::GetRep(FactionId faction) const noexcept
{
    const FactionState* fs = Find(faction);
    return fs ? fs->reputation : 0;
}

FactionStanding FactionRegistry::GetStanding(FactionId faction) const noexcept
{
    const int rep = GetRep(faction);
    if (rep >= 75)  return FactionStanding::Allied;
    if (rep >= 25)  return FactionStanding::Friendly;
    if (rep >= -25) return FactionStanding::Neutral;
    return FactionStanding::Hostile;
}

const char* FactionRegistry::StandingName(FactionStanding s) noexcept
{
    switch (s)
    {
    case FactionStanding::Allied:   return "Allied";
    case FactionStanding::Friendly: return "Friendly";
    case FactionStanding::Neutral:  return "Neutral";
    case FactionStanding::Hostile:  return "Hostile";
    }
    return "Unknown";
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

FactionState* FactionRegistry::Find(FactionId id) noexcept
{
    for (auto& fs : m_Factions)
        if (fs.id == id) return &fs;
    return nullptr;
}

const FactionState* FactionRegistry::Find(FactionId id) const noexcept
{
    for (const auto& fs : m_Factions)
        if (fs.id == id) return &fs;
    return nullptr;
}

} // namespace NF::Game::Gameplay

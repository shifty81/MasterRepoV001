#include "Game/Gameplay/Economy/TradeMarket.h"
#include <algorithm>

namespace NF::Game::Gameplay {

MarketEntry* TradeMarket::FindEntry(NF::Game::ResourceType type) noexcept
{
    const auto idx = static_cast<int>(type);
    if (idx >= 0 && idx < kEntryCount)
        return &m_Entries[idx];
    return nullptr;
}

const MarketEntry* TradeMarket::FindEntry(NF::Game::ResourceType type) const noexcept
{
    const auto idx = static_cast<int>(type);
    if (idx >= 0 && idx < kEntryCount)
        return &m_Entries[idx];
    return nullptr;
}

void TradeMarket::Initialize(const ResourceRegistry& reg, uint32_t initialStock)
{
    m_Reg = &reg;
    for (int i = 0; i < kEntryCount; ++i) {
        const auto& def = reg.Get(static_cast<NF::Game::ResourceType>(i));
        m_Entries[i].type         = def.type;
        m_Entries[i].stock        = (def.tradeable ? initialStock : 0);
        m_Entries[i].currentPrice = def.basePrice;
    }
}

float TradeMarket::GetPrice(NF::Game::ResourceType type) const noexcept
{
    const auto* e = FindEntry(type);
    return e ? e->currentPrice : 0.f;
}

uint32_t TradeMarket::GetStock(NF::Game::ResourceType type) const noexcept
{
    const auto* e = FindEntry(type);
    return e ? e->stock : 0;
}

void TradeMarket::AddCredits(float amount) noexcept
{
    m_PlayerCredits += amount;
}

void TradeMarket::AdjustPrice(MarketEntry& e, float basePrice, float direction) noexcept
{
    const float step = basePrice * kPriceStep * direction;
    e.currentPrice = std::clamp(e.currentPrice + step,
                                basePrice * kPriceFloor,
                                basePrice * kPriceCeiling);
}

TradeResult TradeMarket::Buy(NF::Game::ResourceType type, uint32_t count,
                             NF::Game::Inventory& inv)
{
    if (type == NF::Game::ResourceType::None) return TradeResult::InvalidType;
    if (!m_Reg) return TradeResult::InvalidType;

    const auto& def = m_Reg->Get(type);
    if (!def.tradeable) return TradeResult::NotTradeable;

    auto* entry = FindEntry(type);
    if (!entry) return TradeResult::InvalidType;
    if (entry->stock < count) return TradeResult::InsufficientStock;

    const float totalCost = entry->currentPrice * static_cast<float>(count);
    if (m_PlayerCredits < totalCost) return TradeResult::InsufficientFunds;

    // Execute trade.
    m_PlayerCredits -= totalCost;
    entry->stock    -= count;
    inv.AddItem(type, count);

    // Batch price adjustment: apply step for each unit in a single formula.
    const float totalStep = def.basePrice * kPriceStep * static_cast<float>(count);
    entry->currentPrice = std::clamp(entry->currentPrice + totalStep,
                                     def.basePrice * kPriceFloor,
                                     def.basePrice * kPriceCeiling);

    return TradeResult::Success;
}

TradeResult TradeMarket::Sell(NF::Game::ResourceType type, uint32_t count,
                              NF::Game::Inventory& inv)
{
    if (type == NF::Game::ResourceType::None) return TradeResult::InvalidType;
    if (!m_Reg) return TradeResult::InvalidType;

    const auto& def = m_Reg->Get(type);
    if (!def.tradeable) return TradeResult::NotTradeable;

    if (!inv.HasItem(type, count)) return TradeResult::InsufficientItems;

    auto* entry = FindEntry(type);
    if (!entry) return TradeResult::InvalidType;

    const float totalRevenue = entry->currentPrice * static_cast<float>(count);

    // Execute trade.
    inv.RemoveItem(type, count);
    entry->stock    += count;
    m_PlayerCredits += totalRevenue;

    // Batch price adjustment: apply step for each unit in a single formula.
    const float totalStep = def.basePrice * kPriceStep * static_cast<float>(count);
    entry->currentPrice = std::clamp(entry->currentPrice - totalStep,
                                     def.basePrice * kPriceFloor,
                                     def.basePrice * kPriceCeiling);

    return TradeResult::Success;
}

} // namespace NF::Game::Gameplay

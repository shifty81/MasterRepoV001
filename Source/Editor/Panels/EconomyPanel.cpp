#include "Editor/Panels/EconomyPanel.h"
#include "Editor/Panels/EditorTheme.h"
#include "UI/Rendering/UIRenderer.h"
#include "Game/Interaction/ResourceItem.h"
#include <string>
#include <algorithm>

namespace NF::Editor {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static std::string FormatFloat(float v, int decimals = 1)
{
    // Simple fixed-decimal formatter (avoids <sstream> in the hot path).
    const int scale = (decimals == 1) ? 10 : (decimals == 2 ? 100 : 1);
    const int whole = static_cast<int>(v);
    const int frac  = static_cast<int>((v - static_cast<float>(whole)) * static_cast<float>(scale) + 0.5f);
    return std::to_string(whole) + "." + std::to_string(frac);
}

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void EconomyPanel::Update([[maybe_unused]] float dt) noexcept {}

// ---------------------------------------------------------------------------
// Draw
// ---------------------------------------------------------------------------

void EconomyPanel::Draw(float x, float y, float w, float h)
{
    if (!m_Renderer) return;

    const auto& t     = ActiveTheme();
    const float dpi   = m_Renderer->GetDpiScale();
    const float padX  = 6.f * dpi;
    const float lineH = 16.f * dpi;

    float cy = y + 4.f * dpi;

    DrawHeader(x + padX, cy, w - 2.f * padX);

    m_Renderer->DrawRect({x, cy, w, 1.f}, t.separator);
    cy += 4.f * dpi;

    // Market price table
    m_Renderer->DrawText("Market Prices", x + padX, cy, t.textHeader, 1.f);
    cy += lineH;
    DrawMarketTable(x + padX, cy, w - 2.f * padX, y + h * 0.65f);

    m_Renderer->DrawRect({x, cy, w, 1.f}, t.separator);
    cy += 4.f * dpi;

    // Manufacturing
    m_Renderer->DrawText("Manufacturing", x + padX, cy, t.textHeader, 1.f);
    cy += lineH;
    DrawManufacturing(x + padX, cy, w - 2.f * padX, y + h);
}

// ---------------------------------------------------------------------------
// DrawHeader
// ---------------------------------------------------------------------------

void EconomyPanel::DrawHeader(float x, float& cy, float w)
{
    if (!m_Renderer) return;

    const auto& t     = ActiveTheme();
    const float dpi   = m_Renderer->GetDpiScale();
    const float lineH = 16.f * dpi;

    if (m_Station) {
        const float credits = m_Station->GetMarket().GetCredits();
        const bool  docked  = m_Station->IsDocked();

        std::string headerLine = "Station: " + m_Station->GetName()
            + "  |  Credits: " + FormatFloat(credits, 1)
            + "  |  " + (docked ? "DOCKED" : "Undocked");
        m_Renderer->DrawText(headerLine.c_str(), x, cy, t.textHeader, 1.f);
    } else {
        m_Renderer->DrawText("Economy  (read-only — no live station)", x, cy,
                             t.textSecondary, 1.f);
    }
    cy += lineH;
    (void)w;
}

// ---------------------------------------------------------------------------
// DrawMarketTable
// ---------------------------------------------------------------------------

void EconomyPanel::DrawMarketTable(float x, float& cy, float w, float maxY)
{
    if (!m_Renderer) return;

    const auto& t     = ActiveTheme();
    const float dpi   = m_Renderer->GetDpiScale();
    const float lineH = 15.f * dpi;
    const float colW  = w / 4.f;

    // Column headers
    m_Renderer->DrawText("Resource",  x,              cy, t.textSecondary, 1.f);
    m_Renderer->DrawText("BasePrice", x + colW,       cy, t.textSecondary, 1.f);
    m_Renderer->DrawText("CurPrice",  x + colW * 2.f, cy, t.textSecondary, 1.f);
    m_Renderer->DrawText("Stock",     x + colW * 3.f, cy, t.textSecondary, 1.f);
    cy += lineH;

    using RT = NF::Game::ResourceType;
    static constexpr RT kTypes[] = {
        RT::Stone, RT::Ore, RT::Dirt, RT::Rock,
        RT::Metal, RT::Ice, RT::Organic
    };

    for (const auto type : kTypes)
    {
        if (cy + lineH > maxY) break;

        const char* name = NF::Game::ResourceTypeName(type);

        float basePrice = 1.f;
        float curPrice  = 1.f;
        uint32_t stock  = 0;
        bool tradeable  = true;

        if (m_Registry) {
            const auto& def = m_Registry->Get(type);
            basePrice  = def.basePrice;
            tradeable  = def.tradeable;
        }
        if (m_Station) {
            curPrice = m_Station->GetMarket().GetPrice(type);
            stock    = m_Station->GetMarket().GetStock(type);
        }

        const uint32_t textCol = tradeable ? t.textPrimary : t.textSecondary;

        m_Renderer->DrawText(name,                         x,              cy, textCol, 1.f);
        m_Renderer->DrawText(FormatFloat(basePrice).c_str(), x + colW,     cy, textCol, 1.f);
        m_Renderer->DrawText(FormatFloat(curPrice).c_str(),  x + colW*2.f, cy, textCol, 1.f);
        m_Renderer->DrawText(std::to_string(stock).c_str(),  x + colW*3.f, cy, textCol, 1.f);
        cy += lineH;
    }
}

// ---------------------------------------------------------------------------
// DrawManufacturing
// ---------------------------------------------------------------------------

void EconomyPanel::DrawManufacturing(float x, float& cy, float w, float maxY)
{
    if (!m_Renderer) return;

    const auto& t     = ActiveTheme();
    const float dpi   = m_Renderer->GetDpiScale();
    const float lineH = 15.f * dpi;

    if (!m_Station) {
        m_Renderer->DrawText("(no live station)", x, cy, t.textSecondary, 1.f);
        cy += lineH;
        return;
    }

    const auto& factory = m_Station->GetFactory();
    const bool idle     = factory.IsIdle();
    const int  pending  = factory.PendingCount();

    std::string status = idle ? "Idle" : ("Active — " + std::to_string(pending) + " job(s)");
    m_Renderer->DrawText(status.c_str(), x, cy, idle ? t.textSecondary : t.textPrimary, 1.f);
    cy += lineH;

    // Show available recipes from the registry
    if (m_Registry && cy + lineH <= maxY) {
        m_Renderer->DrawText("Recipes:", x, cy, t.textSecondary, 1.f);
        cy += lineH;

        using RT = NF::Game::ResourceType;
        static constexpr RT kCraftable[] = { RT::Metal };
        for (const auto type : kCraftable) {
            if (cy + lineH > maxY) break;
            const auto* recipe = factory.FindRecipe(type);
            if (!recipe) continue;
            const std::string recipeStr = std::string("  ") + NF::Game::ResourceTypeName(type)
                + " (" + std::to_string(static_cast<int>(recipe->duration)) + "s)";
            m_Renderer->DrawText(recipeStr.c_str(), x, cy, t.textPrimary, 1.f);
            cy += lineH;
        }
    }
    (void)w;
}

} // namespace NF::Editor

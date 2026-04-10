#include "Editor/Panels/FactionPanel.h"
#include "Editor/Panels/EditorTheme.h"
#include "Editor/Application/EditorInputState.h"
#include "UI/Rendering/UIRenderer.h"
#include <string>
#include <algorithm>

namespace NF::Editor {

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void FactionPanel::Update([[maybe_unused]] float dt) noexcept {}

// ---------------------------------------------------------------------------
// Draw
// ---------------------------------------------------------------------------

void FactionPanel::Draw(float x, float y, float w, float h)
{
    if (!m_Renderer) return;

    using namespace NF::Game::Gameplay;

    const auto& t     = ActiveTheme();
    const float dpi   = m_Renderer->GetDpiScale();
    const float padX  = 6.f  * dpi;
    const float lineH = 16.f * dpi;
    const float maxY  = y + h;

    // Panel background
    m_Renderer->DrawRect({x, y, w, h}, t.panelBg);

    float cy = y + 4.f * dpi;

    // ---- Header ----
    m_Renderer->DrawText("Factions", x + padX, cy, t.textHeader, 1.f);
    cy += lineH;
    m_Renderer->DrawRect({x, cy, w, 1.f}, t.separator);
    cy += 4.f * dpi;

    if (!m_Registry)
    {
        m_Renderer->DrawText("(no faction registry)", x + padX, cy, t.textSecondary, 1.f);
        return;
    }

    // ---- Faction rows ----
    const float barH   = 10.f * dpi;
    const float barW   = w - padX * 2.f;
    const float colRep = barW * 0.55f; // x offset where rep bar starts

    for (const auto& fs : m_Registry->GetFactions())
    {
        if (cy + lineH * 3.f > maxY) break;

        // Faction name + standing text
        const FactionStanding standing = m_Registry->GetStanding(fs.id);
        const char* standingName       = FactionRegistry::StandingName(standing);

        // Standing colour
        uint32_t standingCol = t.textSecondary;
        switch (standing)
        {
        case FactionStanding::Allied:   standingCol = 0x44FF88FFu; break;
        case FactionStanding::Friendly: standingCol = 0x88CCFFFF;  break;
        case FactionStanding::Neutral:  standingCol = t.textPrimary; break;
        case FactionStanding::Hostile:  standingCol = 0xFF4444FFu; break;
        }

        m_Renderer->DrawText(fs.name.c_str(),  x + padX, cy, t.textPrimary, 1.f);
        m_Renderer->DrawText(standingName, x + padX + colRep, cy, standingCol, 1.f);
        cy += lineH;

        // Reputation bar  [-100 .. 100]  → bar fraction [0 .. 1]
        const float repFrac = (static_cast<float>(fs.reputation) + 100.f) / 200.f;
        const float barFill = std::clamp(repFrac, 0.f, 1.f) * barW;

        // Bar background
        m_Renderer->DrawRect({x + padX, cy, barW, barH}, t.panelBorder);

        // Filled portion (green for positive, red for negative)
        const uint32_t barCol = (fs.reputation >= 0) ? 0x44BB66FFu : 0xBB4444FFu;
        if (barFill > 0.f)
            m_Renderer->DrawRect({x + padX, cy, barFill, barH}, barCol);

        // Rep value label
        const std::string repStr = std::to_string(fs.reputation);
        m_Renderer->DrawText(repStr.c_str(),
                              x + padX + barW + 4.f * dpi, cy,
                              t.textSecondary, 1.f);
        cy += barH + 4.f * dpi;

        // Separator
        m_Renderer->DrawRect({x, cy, w, 1.f}, t.separator);
        cy += 4.f * dpi;
    }

    // ---- Legend ----
    if (cy + lineH * 2.f < maxY)
    {
        m_Renderer->DrawText("Hostile < -25 | Neutral | Friendly >= 25 | Allied >= 75",
                              x + padX, cy, t.textSecondary, 0.85f);
    }
}

} // namespace NF::Editor

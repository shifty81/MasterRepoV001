#include "Editor/Panels/WorldDebugPanel.h"
#include "Editor/Panels/EditorTheme.h"
#include "UI/Rendering/UIRenderer.h"
#include <string>

namespace NF::Editor {

void WorldDebugPanel::Update([[maybe_unused]] float dt) {}

void WorldDebugPanel::Draw(float x, float y, float w, float h)
{
    if (!m_Renderer) return;

    const uint32_t kHeaderColor = ActiveTheme().textHeader;
    const uint32_t kTextColor   = ActiveTheme().textConsole;
    const uint32_t kSepColor    = ActiveTheme().separator;

    const float dpi   = m_Renderer->GetDpiScale();
    const float lineH = 18.f * dpi;
    const float padX  = 6.f * dpi;
    const float scale = 1.f;
    float cy = y + 4.f * dpi;

    m_Renderer->DrawText("World Debug", x + padX, cy, kHeaderColor, 1.f);
    cy += 20.f * dpi;
    m_Renderer->DrawRect({x, cy, w, 1.f}, kSepColor);
    cy += 4.f * dpi;

    // ---- Player State (RigState) section -----------------------------------
    if (m_Rig) {
        m_Renderer->DrawText("Player State", x + padX, cy, kHeaderColor, scale);
        cy += lineH;

        auto fmtBar = [](float val, float max, int barLen) -> std::string {
            if (max <= 0.f) return "[??]";
            int filled = static_cast<int>((val / max) * barLen + 0.5f);
            filled = filled < 0 ? 0 : (filled > barLen ? barLen : filled);
            std::string bar(static_cast<size_t>(filled), '|');
            bar.resize(static_cast<size_t>(barLen), ' ');
            return "[" + bar + "]";
        };

        const float hp  = m_Rig->GetHealth();
        const float nrg = m_Rig->GetEnergy();
        const int   barW = 20;

        const std::string hpLine  = "  HP     " + fmtBar(hp,  NF::Game::RigState::kMaxHealth, barW)
                                    + " " + std::to_string(static_cast<int>(hp))
                                    + "/" + std::to_string(static_cast<int>(NF::Game::RigState::kMaxHealth));
        const std::string nrgLine = "  Energy " + fmtBar(nrg, NF::Game::RigState::kMaxEnergy, barW)
                                    + " " + std::to_string(static_cast<int>(nrg))
                                    + "/" + std::to_string(static_cast<int>(NF::Game::RigState::kMaxEnergy));

        const uint32_t hpColor  = (hp  < 30.f) ? 0xFF4444FF : kTextColor;
        const uint32_t nrgColor = (nrg < 20.f) ? 0xFF8800FF : kTextColor;

        if (cy + lineH <= y + h) {
            m_Renderer->DrawText(hpLine,  x + padX, cy, hpColor,  scale);
            cy += lineH;
        }
        if (cy + lineH <= y + h) {
            m_Renderer->DrawText(nrgLine, x + padX, cy, nrgColor, scale);
            cy += lineH;
        }

        cy += 4.f * dpi;
        m_Renderer->DrawRect({x, cy, w, 1.f}, kSepColor);
        cy += 4.f * dpi;
    }

    // ---- WorldDebugOverlay lines -------------------------------------------
    if (!m_Overlay || !m_Overlay->IsEnabled()) {
        m_Renderer->DrawText("(overlay disabled)", x + padX, cy, 0x808080FF, scale);
        return;
    }

    const auto& lines = m_Overlay->GetLines();
    for (const auto& line : lines) {
        if (cy + lineH > y + h) break;
        m_Renderer->DrawText(line, x + padX, cy, kTextColor, scale);
        cy += lineH;
    }
}

} // namespace NF::Editor

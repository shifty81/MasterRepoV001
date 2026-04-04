#include "Editor/Panels/WorldDebugPanel.h"
#include "Editor/Panels/EditorTheme.h"
#include "UI/Rendering/UIRenderer.h"

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

#include "Editor/Panels/WorldDebugPanel.h"
#include "UI/Rendering/UIRenderer.h"

namespace NF::Editor {

void WorldDebugPanel::Update([[maybe_unused]] float dt) {}

void WorldDebugPanel::Draw(float x, float y, float w, float h)
{
    if (!m_Renderer) return;

    static constexpr uint32_t kHeaderColor = 0xCCCCCCFF;
    static constexpr uint32_t kTextColor   = 0xA0D0A0FF;
    static constexpr uint32_t kSepColor    = 0x444444FF;

    const float dpi   = m_Renderer->GetDpiScale();
    const float lineH = 18.f * dpi;
    const float padX  = 6.f * dpi;
    const float scale = 1.8f;
    float cy = y + 4.f * dpi;

    m_Renderer->DrawText("World Debug", x + padX, cy, kHeaderColor, 2.f);
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

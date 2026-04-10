#include "Editor/Application/EditorModeManager.h"

namespace NF::Editor {

const char* EditorModeName(EditorMode mode) noexcept
{
    switch (mode) {
    case EditorMode::Select:   return "Select";
    case EditorMode::Voxels:   return "Voxels";
    case EditorMode::Entities: return "Entities";
    case EditorMode::World:    return "World";
    case EditorMode::Debug:    return "Debug";
    default:                   return "Unknown";
    }
}

void EditorModeManager::SetActiveMode(EditorMode mode)
{
    if (mode == m_ActiveMode) return;
    m_ActiveMode = mode;
    if (m_OnModeChanged) m_OnModeChanged(mode);
}

void EditorModeManager::Draw(float x, float y, float w, float h)
{
    if (!m_Renderer) return;

    const auto& theme = ActiveTheme();
    const float dpi   = m_Renderer->GetDpiScale();

    // Strip background — distinct from toolbar so the band is clearly visible.
    m_Renderer->DrawRect({x, y, w, h}, theme.modeTabBg);
    // Top separator to distinguish from Band 1 (toolbar).
    m_Renderer->DrawRect({x, y, w, 1.f}, theme.separator);
    // Bottom separator.
    m_Renderer->DrawRect({x, y + h - 1.f, w, 1.f}, theme.separator);

    // Tab layout.
    constexpr int modeCount = static_cast<int>(EditorMode::Count);
    const float tabW   = 90.f * dpi;
    const float tabH   = h - 4.f;
    const float padX   = 8.f * dpi;
    float cx = x + padX;

    for (int i = 0; i < modeCount; ++i) {
        const EditorMode mode = static_cast<EditorMode>(i);
        const bool active = (mode == m_ActiveMode);

        // Hit test.
        bool hovered = false;
        if (m_Input) {
            hovered = m_Input->mouseX >= cx && m_Input->mouseX < cx + tabW
                   && m_Input->mouseY >= y  && m_Input->mouseY < y + h;
            if (hovered && m_Input->leftJustPressed)
                SetActiveMode(mode);
        }

        // Tab background — always draw a subtle bg so tabs are visible.
        uint32_t bg = active  ? theme.selectBg
                    : hovered ? theme.hoverBg
                              : theme.buttonBg;
        m_Renderer->DrawRoundedRect({cx, y + 2.f, tabW, tabH}, bg,
                                    theme.tabCornerRadius * dpi);

        // Tab outline so each tab is visually distinct.
        m_Renderer->DrawRoundedOutlineRect({cx, y + 2.f, tabW, tabH}, theme.panelBorder,
                                           theme.tabCornerRadius * dpi);

        // Active accent bar at the bottom of the tab.
        if (active)
            m_Renderer->DrawRect({cx, y + h - 3.f, tabW, 3.f}, theme.worldAccent);

        // Label — scale 1.0 for the 8×14 bitmap font.
        const char* label = EditorModeName(mode);
        const uint32_t textCol = active ? theme.textHeader : theme.textPrimary;
        m_Renderer->DrawText(label,
                             cx + 8.f * dpi,
                             y + 5.f * dpi,
                             textCol, 1.f);

        cx += tabW + 2.f * dpi;
    }
}

} // namespace NF::Editor

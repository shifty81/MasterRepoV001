#include "Editor/Application/ContextToolShelf.h"
#include "Editor/Panels/EditorTheme.h"

namespace NF::Editor {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void ContextToolShelf::DrawLabel(float& cx, float y, float h,
                                 const char* text, uint32_t color)
{
    if (!m_Renderer) return;
    const float dpi = m_Renderer->GetDpiScale();
    m_Renderer->DrawText(text, cx, y + 3.f * dpi, color, 1.5f);
    // Estimate label width (approx 6px per char at scale 1.5)
    cx += static_cast<float>(std::char_traits<char>::length(text)) * 6.f * dpi * 1.5f + 8.f * dpi;
}

bool ContextToolShelf::DrawShelfButton(float& cx, float y, float h,
                                       const char* label, bool active)
{
    if (!m_Renderer) return false;

    const auto& theme = ActiveTheme();
    const float dpi   = m_Renderer->GetDpiScale();
    const float btnW  = static_cast<float>(std::char_traits<char>::length(label)) * 6.f * dpi * 1.5f + 12.f * dpi;
    const float btnH  = h - 4.f;
    const float btnY  = y + 2.f;

    bool hovered = false;
    bool clicked = false;
    if (m_Input) {
        hovered = m_Input->mouseX >= cx && m_Input->mouseX < cx + btnW
               && m_Input->mouseY >= btnY && m_Input->mouseY < btnY + btnH;
        clicked = hovered && m_Input->leftJustPressed;
    }

    uint32_t bg = active  ? theme.buttonActive
                : hovered ? theme.buttonHover
                          : theme.buttonBg;
    m_Renderer->DrawRect({cx, btnY, btnW, btnH}, bg);
    m_Renderer->DrawOutlineRect({cx, btnY, btnW, btnH}, theme.panelBorder);

    uint32_t textCol = active ? theme.textHeader : theme.textPrimary;
    m_Renderer->DrawText(label, cx + 4.f * dpi, btnY + 2.f * dpi, textCol, 1.5f);

    cx += btnW + 4.f * dpi;
    return clicked;
}

// ---------------------------------------------------------------------------
// Main Draw
// ---------------------------------------------------------------------------

void ContextToolShelf::Draw(float x, float y, float w, float h, EditorMode mode)
{
    if (!m_Renderer) return;

    const auto& theme = ActiveTheme();

    // Background.
    m_Renderer->DrawRect({x, y, w, h}, theme.menuBarBg);
    m_Renderer->DrawRect({x, y + h - 1.f, w, 1.f}, theme.separator);

    switch (mode) {
    case EditorMode::Select:   DrawSelectShelf(x, y, w, h); break;
    case EditorMode::Voxels:   DrawVoxelShelf(x, y, w, h);  break;
    case EditorMode::Entities: DrawEntityShelf(x, y, w, h); break;
    case EditorMode::World:    DrawWorldShelf(x, y, w, h);  break;
    case EditorMode::Debug:    DrawDebugShelf(x, y, w, h);  break;
    default: break;
    }
}

// ---------------------------------------------------------------------------
// Mode shelves
// ---------------------------------------------------------------------------

void ContextToolShelf::DrawSelectShelf(float x, float y, float w, float h)
{
    const auto& theme = ActiveTheme();
    const float dpi = m_Renderer->GetDpiScale();
    float cx = x + 8.f * dpi;

    DrawLabel(cx, y, h, "Select Mode", theme.textSecondary);

    // Transform tools — always relevant in Select mode.
    DrawShelfButton(cx, y, h, "Move", false);
    DrawShelfButton(cx, y, h, "Rotate", false);
    DrawShelfButton(cx, y, h, "Scale", false);

    // Separator.
    cx += 4.f * dpi;
    m_Renderer->DrawRect({cx, y + 4.f, 1.f, h - 8.f}, theme.separator);
    cx += 8.f * dpi;

    DrawShelfButton(cx, y, h, "Snap", false);
    DrawShelfButton(cx, y, h, "Grid", false);
}

void ContextToolShelf::DrawVoxelShelf(float x, float y, float w, float h)
{
    if (!m_ToolContext) return;

    const auto& theme = ActiveTheme();
    const float dpi = m_Renderer->GetDpiScale();
    float cx = x + 8.f * dpi;

    DrawLabel(cx, y, h, "Voxel Mode", theme.textSecondary);

    // Voxel tools.
    bool isAdd    = m_ToolContext->activeMode == nf::EditorToolMode::VoxelAdd;
    bool isRemove = m_ToolContext->activeMode == nf::EditorToolMode::VoxelRemove;
    bool isInspect = m_ToolContext->activeMode == nf::EditorToolMode::VoxelInspect;

    if (DrawShelfButton(cx, y, h, "Inspect", isInspect))
        m_ToolContext->activeMode = nf::EditorToolMode::VoxelInspect;
    if (DrawShelfButton(cx, y, h, "+Add", isAdd))
        m_ToolContext->activeMode = nf::EditorToolMode::VoxelAdd;
    if (DrawShelfButton(cx, y, h, "-Remove", isRemove))
        m_ToolContext->activeMode = nf::EditorToolMode::VoxelRemove;

    // Separator.
    cx += 4.f * dpi;
    m_Renderer->DrawRect({cx, y + 4.f, 1.f, h - 8.f}, theme.separator);
    cx += 8.f * dpi;

    // Voxel type display.
    DrawLabel(cx, y, h, "Type:", theme.textSecondary);
    std::string typeStr = std::to_string(m_ToolContext->selectedVoxelType);
    DrawLabel(cx, y, h, typeStr.c_str(), theme.textValue);
}

void ContextToolShelf::DrawEntityShelf(float x, float y, float w, float h)
{
    const auto& theme = ActiveTheme();
    const float dpi = m_Renderer->GetDpiScale();
    float cx = x + 8.f * dpi;

    DrawLabel(cx, y, h, "Entity Mode", theme.textSecondary);
    DrawShelfButton(cx, y, h, "Place", false);
    DrawShelfButton(cx, y, h, "Duplicate", false);
    DrawShelfButton(cx, y, h, "Group", false);
}

void ContextToolShelf::DrawWorldShelf(float x, float y, float w, float h)
{
    const auto& theme = ActiveTheme();
    const float dpi = m_Renderer->GetDpiScale();
    float cx = x + 8.f * dpi;

    DrawLabel(cx, y, h, "World Mode", theme.textSecondary);
    DrawShelfButton(cx, y, h, "Spawn", false);
    DrawShelfButton(cx, y, h, "Environment", false);
    DrawShelfButton(cx, y, h, "Validate", false);
}

void ContextToolShelf::DrawDebugShelf(float x, float y, float w, float h)
{
    const auto& theme = ActiveTheme();
    const float dpi = m_Renderer->GetDpiScale();
    float cx = x + 8.f * dpi;

    DrawLabel(cx, y, h, "Debug Mode", theme.textSecondary);
    DrawShelfButton(cx, y, h, "Collision", false);
    DrawShelfButton(cx, y, h, "Bounds", false);
    DrawShelfButton(cx, y, h, "Stats", false);
    DrawShelfButton(cx, y, h, "Chunks", false);
}

} // namespace NF::Editor

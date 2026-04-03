#include "Editor/Panels/Inspector.h"
#include "UI/Rendering/UIRenderer.h"
#include "Game/World/GameWorld.h"
#include "Game/Voxel/VoxelType.h"
#include <string>
#include <variant>

namespace NF::Editor {

void Inspector::Update([[maybe_unused]] float dt) {}

void Inspector::SetSelectedVoxel(const nf::SelectionHandle& handle,
                                  const NF::Game::GameWorld& gameWorld)
{
    m_SelectedEntity = NullEntity;
    m_World          = nullptr;
    m_VoxelSelected  = true;
    m_VoxelLabel     = handle.label;

    m_VoxelX = nf::UnpackVoxelCoord(handle.id, 0);
    m_VoxelY = nf::UnpackVoxelCoord(handle.id, nf::kVoxelCoordBits);
    m_VoxelZ = nf::UnpackVoxelCoord(handle.id, nf::kVoxelCoordBits * 2);

    m_VoxelType = static_cast<int>(
        gameWorld.GetVoxelEditApi().GetVoxel(m_VoxelX, m_VoxelY, m_VoxelZ));
}

// ---------------------------------------------------------------------------
// HandlePropertyClick — apply edits when user clicks on editable properties
// ---------------------------------------------------------------------------

void Inspector::HandlePropertyClick(float x, float /*y*/, float w, float rowY,
                                     float lineH, const nf::PropertyEntry& entry,
                                     size_t /*entryIndex*/)
{
    if (!m_Input || !m_PropSystem) return;
    if (!m_Input->leftJustPressed) return;
    if (entry.readOnly) return;

    const float mx = m_Input->mouseX;
    const float my = m_Input->mouseY;

    // Check if click is within the value column of this row.
    const float labelColW = w * 0.45f;
    const float valX = x + labelColW;
    if (mx < valX || mx > x + w) return;
    if (my < rowY || my > rowY + lineH) return;

    nf::PropertyValue newValue = entry.value;

    if (std::holds_alternative<bool>(entry.value)) {
        // Toggle boolean
        newValue = !std::get<bool>(entry.value);
    } else if (std::holds_alternative<int>(entry.value)) {
        // Left half of value column decrements, right half increments.
        int val = std::get<int>(entry.value);
        if (mx < valX + (w - labelColW) * 0.5f)
            val += 1;
        else
            val -= 1;
        newValue = val;
    } else if (std::holds_alternative<float>(entry.value)) {
        // Left half of value column steps up, right half steps down by 0.1.
        float val = std::get<float>(entry.value);
        if (mx < valX + (w - labelColW) * 0.5f)
            val += 0.1f;
        else
            val -= 0.1f;
        newValue = val;
    } else {
        // String and Vec3 editing requires text input — skip for now
        return;
    }

    if (m_PropSystem->ApplyEdit(entry.name, newValue)) {
        if (m_OnPropertyEdited) {
            m_OnPropertyEdited();
        }
    }
}

void Inspector::Draw(float x, float y, float w, float h) {
    if (!m_Renderer) return;

    static constexpr uint32_t kTextColor    = 0xB0B0B0FF;
    static constexpr uint32_t kLabelColor   = 0x808080FF;
    static constexpr uint32_t kValueColor   = 0xD0D0D0FF;
    static constexpr uint32_t kHeaderColor  = 0xCCCCCCFF;
    static constexpr uint32_t kDirtyColor   = 0xFFAA44FF;
    static constexpr uint32_t kSepColor     = 0x444444FF;
    static constexpr uint32_t kEditableCol  = 0x78C8FFFF;
    static constexpr uint32_t kHoverCol     = 0x90E0FFFF;
    const float dpi   = m_Renderer->GetDpiScale();
    const float lineH = 20.f * dpi;
    const float padX  = 6.f * dpi;
    const float scale = 2.f;
    float cy = y + 4.f * dpi;

    // ---- Structured property grid (from PropertyInspectorSystem) ------------
    if (m_PropSystem != nullptr && m_PropSystem->HasProperties()) {
        const auto& ps = m_PropSystem->GetPropertySet();

        // Title row
        std::string title = ps.title;
        if (ps.dirty) title += " [*]";
        m_Renderer->DrawText(title, x + padX, cy, kHeaderColor, scale);
        cy += lineH;
        m_Renderer->DrawRect({x, cy, w, 1.f}, kSepColor);
        cy += 4.f * dpi;

        const float labelColW = w * 0.45f;

        for (size_t i = 0; i < ps.entries.size(); ++i) {
            const auto& entry = ps.entries[i];
            if (cy + lineH > y + h) break;

            // Property name
            const uint32_t nameCol = entry.dirty ? kDirtyColor : kLabelColor;
            m_Renderer->DrawText(entry.name + ":", x + padX, cy, nameCol, scale);

            // Determine if mouse is hovering this editable row
            bool hovered = false;
            if (m_Input && !entry.readOnly) {
                const float mx = m_Input->mouseX;
                const float my = m_Input->mouseY;
                if (mx >= x && mx <= x + w && my >= cy && my < cy + lineH) {
                    hovered = true;
                }
            }

            // Property value
            std::string valStr = nf::PropertyInspectorSystem::ToDisplayString(entry);
            if (!entry.readOnly && std::holds_alternative<bool>(entry.value)) {
                // Show a clickable checkbox indicator for bools
                valStr = std::get<bool>(entry.value) ? "[x] true" : "[ ] false";
            }
            const uint32_t valCol = entry.readOnly ? kValueColor
                                  : (hovered ? kHoverCol : kEditableCol);
            m_Renderer->DrawText(valStr, x + padX + labelColW, cy, valCol, scale);

            // Handle click interaction
            HandlePropertyClick(x, y, w, cy, lineH, entry, i);

            cy += lineH;
        }
        return;
    }

    // ---- Fallback: voxel inspection -----------------------------------------
    if (m_VoxelSelected) {
        m_Renderer->DrawText("Voxel", x + padX, cy, kHeaderColor, scale);
        cy += lineH;
        m_Renderer->DrawRect({x, cy, w, 1.f}, kSepColor);
        cy += 4.f * dpi;

        m_Renderer->DrawText("Position:", x + padX, cy, kLabelColor, scale);
        cy += lineH;
        std::string posStr = "  X: " + std::to_string(m_VoxelX)
                           + "  Y: " + std::to_string(m_VoxelY)
                           + "  Z: " + std::to_string(m_VoxelZ);
        m_Renderer->DrawText(posStr, x + padX, cy, kValueColor, scale);
        cy += lineH;

        m_Renderer->DrawText("Type ID:", x + padX, cy, kLabelColor, scale);
        cy += lineH;
        const auto& typeInfo = NF::Game::GetVoxelTypeInfo(
            static_cast<NF::Game::VoxelId>(m_VoxelType));
        std::string typeStr = std::string("  ") + std::to_string(m_VoxelType)
                            + " (" + typeInfo.name + ")";
        m_Renderer->DrawText(typeStr, x + padX, cy, kValueColor, scale);
        cy += lineH;

        return;
    }

    // ---- Fallback: entity inspection ----------------------------------------
    if (m_SelectedEntity == NullEntity || !m_World) {
        m_Renderer->DrawText("No selection", x + padX, cy, kLabelColor, scale);
        return;
    }

    std::string label = "Entity " + std::to_string(m_SelectedEntity);
    m_Renderer->DrawText(label, x + padX, cy, kHeaderColor, scale);
    cy += lineH;
    m_Renderer->DrawRect({x, cy, w, 1.f}, kSepColor);
    cy += 4.f * dpi;

    m_Renderer->DrawText("Components:", x + padX, cy, kLabelColor, scale);
    cy += lineH;

    const auto& entities = m_World->GetLiveEntities();
    bool found = false;
    for (EntityId e : entities) {
        if (e == m_SelectedEntity) { found = true; break; }
    }

    if (found) {
        m_Renderer->DrawText("  (entity exists in world)", x + padX, cy, kValueColor, scale);
    } else {
        m_Renderer->DrawText("  (entity not found)", x + padX, cy, 0xFF4040FF, scale);
    }
}

} // namespace NF::Editor

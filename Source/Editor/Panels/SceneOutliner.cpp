#include "Editor/Panels/SceneOutliner.h"
#include "Editor/Panels/EditorTheme.h"
#include "UI/Rendering/UIRenderer.h"
#include <string>
#include <utility>

namespace NF::Editor {

void SceneOutliner::Update([[maybe_unused]] float dt) {}

void SceneOutliner::SetChunkData(std::string worldName, std::vector<RuntimeChunkMetadata> chunks)
{
    m_WorldName = std::move(worldName);
    m_Chunks    = std::move(chunks);
}

void SceneOutliner::Draw(float x, float y, float w, float h) {
    if (!m_Renderer) return;

    const auto& t = ActiveTheme();
    const uint32_t kTextColor   = t.textPrimary;
    const uint32_t kLabelColor  = t.textSecondary;
    const uint32_t kHoverColor  = t.hoverBg;
    const uint32_t kSelectColor = t.selectBg;
    const uint32_t kHeaderColor = t.textHeader;
    const uint32_t kSepColor    = t.separator;
    const uint32_t kDirtyColor  = t.dirty;
    const uint32_t kWorldColor  = t.worldAccent;

    const float dpi   = m_Renderer->GetDpiScale();
    const float lineH = 18.f * dpi;
    const float scale = 2.f;
    const float padX  = 6.f * dpi;
    const float indX  = 14.f * dpi;
    float cy = y + 4.f * dpi;

    // ---- World / chunk tree section -----------------------------------------
    if (!m_WorldName.empty()) {
        // World root node
        m_Renderer->DrawText(m_WorldName,
                             x + padX, cy, kWorldColor, scale);
        cy += lineH;

        if (!m_Chunks.empty()) {
            // "Chunks" folder node
            const std::string chunkHeader =
                "  Chunks (" + std::to_string(m_Chunks.size()) + ")";
            m_Renderer->DrawText(chunkHeader, x + padX, cy, kHeaderColor, scale);
            cy += lineH;

            for (const auto& meta : m_Chunks) {
                if (cy + lineH > y + h) break;

                std::string label = "    " + meta.label;
                if (meta.dirty) label += " [*]";

                const uint32_t col = meta.dirty ? kDirtyColor : kTextColor;
                m_Renderer->DrawText(label, x + padX, cy, col, scale);
                cy += lineH;
            }
        } else {
            m_Renderer->DrawText("  (no chunks)", x + padX, cy, kLabelColor, scale);
            cy += lineH;
        }

        // Separator between world tree and entity list
        if (cy + 6.f * dpi < y + h) {
            m_Renderer->DrawRect({x, cy + 2.f * dpi, w, 1.f}, kSepColor);
            cy += 8.f * dpi;
        }
    }

    // ---- ECS entity list section --------------------------------------------
    if (!m_World) {
        m_Renderer->DrawText("No world loaded", x + padX, cy, kLabelColor, scale);
        return;
    }

    const auto& entities = m_World->GetLiveEntities();
    if (entities.empty()) {
        m_Renderer->DrawText("(empty world)", x + padX, cy, kLabelColor, scale);
        return;
    }

    for (EntityId e : entities) {
        if (cy + lineH > y + h) break;

        const bool hovered  = m_Input &&
                              m_Input->mouseX >= x           && m_Input->mouseX < x + w &&
                              m_Input->mouseY >= cy          && m_Input->mouseY < cy + lineH;
        const bool selected = (e == m_SelectedEntity);

        if (selected) {
            m_Renderer->DrawRect({x, cy, w, lineH}, kSelectColor);
        } else if (hovered) {
            m_Renderer->DrawRect({x, cy, w, lineH}, kHoverColor);
        }

        if (hovered && m_Input->leftJustPressed) {
            m_SelectedEntity = e;
            if (m_OnSelectionChanged) m_OnSelectionChanged(e);
        }

        const uint32_t rowTextColor = selected ? 0xFFFFFFFF : kTextColor;
        std::string label = "Entity " + std::to_string(e);
        m_Renderer->DrawText(label, x + padX, cy + 2.f * dpi, rowTextColor, scale);
        cy += lineH;
    }
}

} // namespace NF::Editor

// SolarSystemPanel.cpp — 2D orbital map panel for the dev solar system.
//
// Draws celestial bodies as circles on a flat map.  Supports selection,
// zoom/pan, inline property editing, and placed-item visualization.

#include "Editor/Panels/SolarSystemPanel.h"
#include "Editor/Panels/EditorTheme.h"
#include "Editor/Application/EditorInputState.h"
#include "UI/Rendering/UIRenderer.h"
#include "Game/Interaction/ResourceItem.h"
#include <cmath>
#include <string>
#include <algorithm>

namespace NF::Editor {

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void SolarSystemPanel::Update([[maybe_unused]] float dt) {
    if (!m_Input || !m_System) return;

    // Zoom with mouse wheel when panel is hovered (simplified — always applies).
    if (m_Input->wheelDelta != 0.f) {
        m_Zoom *= (m_Input->wheelDelta > 0.f) ? 1.1f : 0.9f;
        m_Zoom = std::clamp(m_Zoom, 0.1f, 20.f);
    }

    // Pan with middle-mouse drag.
    if (m_Input->middleDown) {
        m_PanX += m_Input->mouseDeltaX;
        m_PanY += m_Input->mouseDeltaY;
    }
}

// ---------------------------------------------------------------------------
// World <-> Screen coordinate conversion
// ---------------------------------------------------------------------------

void SolarSystemPanel::WorldToScreen(float wx, float wy, float cx, float cy,
                                      float scale, float& sx, float& sy) const noexcept {
    sx = cx + (wx * scale * m_Zoom) + m_PanX;
    sy = cy + (wy * scale * m_Zoom) + m_PanY;
}

// ---------------------------------------------------------------------------
// Draw — entry point
// ---------------------------------------------------------------------------

void SolarSystemPanel::Draw(float x, float y, float w, float h) {
    if (!m_Renderer) return;

    const auto& theme = ActiveTheme();
    const float dpi   = m_Renderer->GetDpiScale();
    const float padX  = 6.f * dpi;
    float cy = y + 4.f * dpi;

    m_Renderer->DrawText("Solar System", x + padX, cy, theme.textHeader, 1.f);
    cy += 20.f * dpi;
    m_Renderer->DrawRect({x, cy, w, 1.f}, theme.separator);
    cy += 2.f * dpi;

    if (!m_System || m_System->BodyCount() == 0) {
        m_Renderer->DrawText("(no solar system generated)", x + padX, cy, theme.textSecondary, 1.f);
        return;
    }

    // Split panel: 65% orbital map, 35% inspector
    const float mapW = w * 0.65f;
    const float inspW = w - mapW;
    const float contentY = cy;
    const float contentH = (y + h) - contentY;

    DrawOrbitalMap(x, contentY, mapW, contentH);
    DrawBodyInspector(x + mapW, contentY, inspW, contentH);
}

// ---------------------------------------------------------------------------
// DrawOrbitalMap
// ---------------------------------------------------------------------------

void SolarSystemPanel::DrawOrbitalMap(float x, float y, float w, float h) {
    if (!m_Renderer || !m_System) return;

    const auto& theme = ActiveTheme();
    const float dpi = m_Renderer->GetDpiScale();

    // ---- Toolbar: toggle buttons for Orbits and Items ----
    {
        const float btnH  = 14.f * dpi;
        const float btnW  = 44.f * dpi;
        const float gap   = 4.f  * dpi;
        float bx = x + 4.f * dpi;
        const float by = y + 2.f * dpi;

        auto drawToggle = [&](const char* label, bool active, bool& flag) {
            const bool hovered = m_Input &&
                m_Input->mouseX >= bx && m_Input->mouseX < bx + btnW &&
                m_Input->mouseY >= by && m_Input->mouseY < by + btnH;
            if (hovered && m_Input && m_Input->leftJustPressed)
                flag = !flag;
            const uint32_t bg = active ? theme.worldAccent
                               : hovered ? theme.hoverBg : theme.titleBarBg;
            m_Renderer->DrawRect({bx, by, btnW, btnH}, bg);
            m_Renderer->DrawOutlineRect({bx, by, btnW, btnH}, theme.panelBorder);
            m_Renderer->DrawText(label, bx + 4.f * dpi, by + 2.f * dpi,
                                 active ? 0xFFFFFFFF : theme.textSecondary, 1.f);
            bx += btnW + gap;
        };

        drawToggle("Orbits", m_ShowOrbits, m_ShowOrbits);
        drawToggle("Items",  m_ShowItems,  m_ShowItems);
    }

    // Map centre — offset down by toolbar height
    const float toolbarH = 20.f * dpi;
    const float cx = x + w * 0.5f;
    const float cy = y + toolbarH + (h - toolbarH) * 0.5f;

    // Scale: map AU to pixels.  Fit the widest orbit into the panel.
    float maxOrbit = 1.f;
    for (const auto& body : m_System->GetBodies()) {
        if (body.type == NF::Game::Gameplay::CelestialBodyType::Planet)
            maxOrbit = std::max(maxOrbit, body.orbitRadius);
    }
    const float scale = std::min(w, h) * 0.4f / maxOrbit;

    // Draw orbit rings.
    if (m_ShowOrbits) {
        for (const auto& body : m_System->GetBodies()) {
            if (body.type != NF::Game::Gameplay::CelestialBodyType::Planet) continue;
            const float r = body.orbitRadius * scale * m_Zoom;
            // Approximate circle with line segments rendered as small rects.
            constexpr int kSegments = 48;
            for (int s = 0; s < kSegments; ++s) {
                const float a0 = static_cast<float>(s) * 6.2832f / static_cast<float>(kSegments);
                const float px = cx + m_PanX + std::cos(a0) * r;
                const float py = cy + m_PanY + std::sin(a0) * r;
                m_Renderer->DrawRect({px, py, 2.f * dpi, 2.f * dpi}, theme.graphGrid);
            }
        }
    }

    // Draw bodies.
    for (const auto& body : m_System->GetBodies()) {
        float sx, sy;

        if (body.type == NF::Game::Gameplay::CelestialBodyType::Star) {
            // Star at centre.
            sx = cx + m_PanX;
            sy = cy + m_PanY;
        } else if (body.type == NF::Game::Gameplay::CelestialBodyType::Moon) {
            // Moon orbits around its parent planet.
            const auto* parent = m_System->FindBody(body.parentId);
            if (!parent) continue;
            float parentSx, parentSy;
            WorldToScreen(parent->FlatX(), parent->FlatY(), cx, cy, scale, parentSx, parentSy);
            const float moonScale = scale * 0.3f;
            sx = parentSx + body.FlatX() * moonScale * m_Zoom;
            sy = parentSy + body.FlatY() * moonScale * m_Zoom;
        } else {
            WorldToScreen(body.FlatX(), body.FlatY(), cx, cy, scale, sx, sy);
        }

        // Determine draw radius based on body type.
        float drawRadius = 4.f * dpi;
        if (body.type == NF::Game::Gameplay::CelestialBodyType::Star)
            drawRadius = 10.f * dpi;
        else if (body.type == NF::Game::Gameplay::CelestialBodyType::Moon)
            drawRadius = 2.5f * dpi;

        // Draw body as a filled rect (circle approximation).
        const uint32_t bodyColor = (body.id == m_SelectedBodyId) ? theme.worldAccent : body.color;
        m_Renderer->DrawRect({sx - drawRadius, sy - drawRadius,
                              drawRadius * 2.f, drawRadius * 2.f}, bodyColor);

        // Selection highlight border.
        if (body.id == m_SelectedBodyId) {
            m_Renderer->DrawOutlineRect({sx - drawRadius - 1.f, sy - drawRadius - 1.f,
                                         drawRadius * 2.f + 2.f, drawRadius * 2.f + 2.f},
                                        theme.worldAccent);
        }

        // Label
        if (body.type != NF::Game::Gameplay::CelestialBodyType::Moon || body.id == m_SelectedBodyId) {
            m_Renderer->DrawText(body.name, sx + drawRadius + 3.f * dpi,
                                 sy - 6.f * dpi, theme.textPrimary, 1.f);
        }

        // Hit test for selection.
        if (m_Input && m_Input->leftJustPressed) {
            const float dx = m_Input->mouseX - sx;
            const float dy = m_Input->mouseY - sy;
            const float hitR = drawRadius + 4.f * dpi;
            if (dx * dx + dy * dy <= hitR * hitR) {
                m_SelectedBodyId = body.id;
                m_SelectedItemId = 0;
                // Notify editor that a celestial body was selected.
                if (m_OnBodySelected)
                    m_OnBodySelected(body.id);
            }
        }
    }

    // Draw placed items for selected body.
    if (m_ShowItems && m_SelectedBodyId > 0) {
        DrawItemOverlay(cx, cy, scale);
    }

    // Legend / status
    const std::string status = "Bodies: " + std::to_string(m_System->BodyCount())
                             + "  Zoom: " + std::to_string(static_cast<int>(m_Zoom * 100)) + "%";
    m_Renderer->DrawText(status, x + 4.f * dpi, y + h - 16.f * dpi, theme.textSecondary, 1.f);
}

// ---------------------------------------------------------------------------
// DrawItemOverlay — placed items for the selected body
// ---------------------------------------------------------------------------

void SolarSystemPanel::DrawItemOverlay(float cx, float cy, float scale) {
    if (!m_ItemGen || !m_Renderer || !m_Input) return;

    const auto& theme = ActiveTheme();
    const float dpi = m_Renderer->GetDpiScale();

    auto items = m_ItemGen->ItemsForBody(m_SelectedBodyId);

    // Draw items as small diamond markers near their body.
    const auto* body = m_System->FindBody(m_SelectedBodyId);
    if (!body) return;

    float bodySx, bodySy;
    if (body->type == NF::Game::Gameplay::CelestialBodyType::Star) {
        bodySx = cx + m_PanX;
        bodySy = cy + m_PanY;
    } else {
        WorldToScreen(body->FlatX(), body->FlatY(), cx, cy, scale, bodySx, bodySy);
    }

    // Items are drawn at offsets from body position, scaled down.
    const float itemScale = 0.3f * m_Zoom * dpi;

    for (const auto* item : items) {
        const float ix = bodySx + item->posX * itemScale;
        const float iy = bodySy + item->posZ * itemScale;
        const float r = item->iconRadius * dpi * 0.5f;

        const uint32_t itemColor = (item->id == m_SelectedItemId) ? theme.worldAccent : item->color;
        m_Renderer->DrawRect({ix - r, iy - r, r * 2.f, r * 2.f}, itemColor);

        // Hit test for item selection.
        if (m_Input->leftJustPressed) {
            const float dx = m_Input->mouseX - ix;
            const float dy = m_Input->mouseY - iy;
            if (dx * dx + dy * dy <= (r + 3.f) * (r + 3.f)) {
                m_SelectedItemId = item->id;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// DrawBodyInspector — property view for the selected body
// ---------------------------------------------------------------------------

void SolarSystemPanel::DrawBodyInspector(float x, float y, float w, float h) {
    if (!m_Renderer || !m_System) return;

    const auto& theme = ActiveTheme();
    const float dpi   = m_Renderer->GetDpiScale();
    const float lineH = 16.f * dpi;
    const float padX  = 6.f * dpi;
    float cy = y + 4.f * dpi;

    // Separator between map and inspector.
    m_Renderer->DrawRect({x, y, 1.f, h}, theme.separator);

    if (m_SelectedBodyId == 0) {
        m_Renderer->DrawText("Select a body", x + padX, cy, theme.textSecondary, 1.f);
        return;
    }

    auto* body = m_System->FindBody(m_SelectedBodyId);
    if (!body) {
        m_Renderer->DrawText("(body not found)", x + padX, cy, theme.textSecondary, 1.f);
        return;
    }

    // Header
    m_Renderer->DrawText(body->name, x + padX, cy, theme.textHeader, 1.f);
    cy += lineH + 4.f * dpi;
    m_Renderer->DrawRect({x, cy, w, 1.f}, theme.separator);
    cy += 4.f * dpi;

    // "Travel to Body" button
    {
        const float btnW = (w - padX * 2.f);
        const float btnH = 14.f * dpi;
        const bool hovered = m_Input &&
            m_Input->mouseX >= x + padX && m_Input->mouseX < x + padX + btnW &&
            m_Input->mouseY >= cy        && m_Input->mouseY < cy + btnH;
        if (hovered && m_Input && m_Input->leftJustPressed && m_OnTravelToBody)
            m_OnTravelToBody(m_SelectedBodyId);
        const uint32_t bg = hovered ? theme.worldAccent : theme.titleBarBg;
        m_Renderer->DrawRect({x + padX, cy, btnW, btnH}, bg);
        m_Renderer->DrawOutlineRect({x + padX, cy, btnW, btnH}, theme.panelBorder);
        m_Renderer->DrawText("Travel to Body",
                             x + padX + 4.f * dpi, cy + 2.f * dpi,
                             hovered ? 0xFFFFFFFF : theme.textSecondary, 1.f);
        cy += btnH + 4.f * dpi;
        m_Renderer->DrawRect({x, cy, w, 1.f}, theme.separator);
        cy += 4.f * dpi;
    }

    // Body properties
    auto drawProp = [&](const char* label, const std::string& value) {
        if (cy + lineH > y + h) return;
        m_Renderer->DrawText(label, x + padX, cy, theme.textSecondary, 1.f);
        m_Renderer->DrawText(value, x + padX + 110.f * dpi, cy, theme.textValue, 1.f);
        cy += lineH;
    };

    drawProp("Type",        NF::Game::Gameplay::CelestialBodyTypeName(body->type));
    drawProp("Orbit R",     std::to_string(body->orbitRadius).substr(0, 6) + " AU");
    drawProp("Period",      std::to_string(body->orbitalPeriod).substr(0, 6) + " yr");
    drawProp("Mass",        std::to_string(body->mass).substr(0, 6) + " Me");
    drawProp("Radius",      std::to_string(body->radius).substr(0, 6) + " Re");
    drawProp("Temp",        std::to_string(static_cast<int>(body->temperature)) + " K");
    drawProp("Angle",       std::to_string(body->orbitAngle).substr(0, 5) + " rad");

    cy += 4.f * dpi;
    m_Renderer->DrawRect({x, cy, w, 1.f}, theme.separator);
    cy += 4.f * dpi;

    // Deposits
    m_Renderer->DrawText("Deposits", x + padX, cy, theme.textHeader, 1.f);
    cy += lineH;

    if (body->deposits.empty()) {
        m_Renderer->DrawText("  (none)", x + padX, cy, theme.textSecondary, 1.f);
        cy += lineH;
    } else {
        for (size_t i = 0; i < body->deposits.size(); ++i) {
            if (cy + lineH > y + h) break;
            const auto& dep = body->deposits[i];
            const std::string depStr = std::string("  ") + NF::Game::ResourceTypeName(dep.type)
                                     + "  x" + std::to_string(dep.abundance).substr(0, 4)
                                     + "  @(" + std::to_string(static_cast<int>(dep.worldX))
                                     + "," + std::to_string(static_cast<int>(dep.worldZ)) + ")";
            m_Renderer->DrawText(depStr, x + padX, cy, theme.textConsole, 1.f);
            cy += lineH;
        }
    }

    // Placed items for this body
    if (m_ItemGen) {
        cy += 4.f * dpi;
        m_Renderer->DrawRect({x, cy, w, 1.f}, theme.separator);
        cy += 4.f * dpi;

        auto items = m_ItemGen->ItemsForBody(m_SelectedBodyId);
        const std::string itemHeader = "Items (" + std::to_string(items.size()) + ")";
        m_Renderer->DrawText(itemHeader, x + padX, cy, theme.textHeader, 1.f);
        cy += lineH;

        for (const auto* item : items) {
            if (cy + lineH > y + h) break;
            const bool selected = (item->id == m_SelectedItemId);
            const uint32_t textColor = selected ? theme.worldAccent : theme.textPrimary;
            const std::string itemStr = "  " + item->name
                                      + "  qty:" + std::to_string(item->quantity)
                                      + (item->userEdited ? " [edited]" : "");
            m_Renderer->DrawText(itemStr, x + padX, cy, textColor, 1.f);

            // Hit test for item selection from list.
            if (m_Input && m_Input->leftJustPressed) {
                if (m_Input->mouseX >= x && m_Input->mouseX < x + w &&
                    m_Input->mouseY >= cy && m_Input->mouseY < cy + lineH) {
                    m_SelectedItemId = item->id;
                }
            }
            cy += lineH;
        }

        // Selected item detail
        if (m_SelectedItemId > 0) {
            auto* selItem = m_ItemGen->FindItem(m_SelectedItemId);
            if (selItem && selItem->sourceBodyId == m_SelectedBodyId) {
                cy += 4.f * dpi;
                m_Renderer->DrawRect({x, cy, w, 1.f}, theme.separator);
                cy += 4.f * dpi;

                m_Renderer->DrawText("Item Detail", x + padX, cy, theme.textHeader, 1.f);
                cy += lineH;

                drawProp("Name",     selItem->name);
                drawProp("Resource", NF::Game::ResourceTypeName(selItem->resourceType));
                drawProp("Qty",      std::to_string(selItem->quantity));
                drawProp("Pos X",    std::to_string(selItem->posX).substr(0, 7));
                drawProp("Pos Z",    std::to_string(selItem->posZ).substr(0, 7));
                drawProp("Edited",   selItem->userEdited ? "Yes" : "No");
            }
        }
    }
}

} // namespace NF::Editor

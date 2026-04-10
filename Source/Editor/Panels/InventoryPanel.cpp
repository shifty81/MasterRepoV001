#include "Editor/Panels/InventoryPanel.h"
#include "Editor/Panels/EditorTheme.h"
#include "UI/Rendering/UIRenderer.h"
#include "Game/Interaction/ResourceItem.h"
#include <string>

namespace NF::Editor {

// ---------------------------------------------------------------------------
// Update
// ---------------------------------------------------------------------------

void InventoryPanel::Update([[maybe_unused]] float dt) noexcept {}

// ---------------------------------------------------------------------------
// Draw
// ---------------------------------------------------------------------------

void InventoryPanel::Draw(float x, float y, float w, float h)
{
    if (!m_Renderer) return;

    const auto& t     = ActiveTheme();
    const float dpi   = m_Renderer->GetDpiScale();
    const float padX  = 6.f * dpi;
    const float lineH = 16.f * dpi;
    const float maxY  = y + h;

    float cy = y + 4.f * dpi;

    // ---- Backpack / player inventory ----------------------------------------
    m_Renderer->DrawText("Backpack", x + padX, cy, t.textHeader, 1.f);
    cy += lineH;
    DrawBackpack(x + padX, cy, w - 2.f * padX, maxY * 0.40f + y);

    if (cy + 4.f * dpi < maxY) {
        m_Renderer->DrawRect({x, cy, w, 1.f}, t.separator);
        cy += 4.f * dpi;
    }

    // ---- Storage boxes -------------------------------------------------------
    m_Renderer->DrawText("Storage Boxes", x + padX, cy, t.textHeader, 1.f);
    cy += lineH;
    DrawStorageBoxes(x + padX, cy, w - 2.f * padX, maxY * 0.72f + y);

    if (cy + 4.f * dpi < maxY) {
        m_Renderer->DrawRect({x, cy, w, 1.f}, t.separator);
        cy += 4.f * dpi;
    }

    // ---- Wreck sites ---------------------------------------------------------
    m_Renderer->DrawText("Wreck Sites", x + padX, cy, t.textHeader, 1.f);
    cy += lineH;
    DrawWreckSites(x + padX, cy, w - 2.f * padX, maxY);
}

// ---------------------------------------------------------------------------
// DrawBackpack
// ---------------------------------------------------------------------------

void InventoryPanel::DrawBackpack(float x, float& cy, float w, float maxY)
{
    const auto& t     = ActiveTheme();
    const float dpi   = m_Renderer->GetDpiScale();
    const float lineH = 15.f * dpi;

    if (!m_PlayerInv) {
        m_Renderer->DrawText("(no live inventory)", x, cy, t.textSecondary, 1.f);
        cy += lineH;
        return;
    }

    DrawInventory(*m_PlayerInv, x, cy, w, maxY);
    (void)w;
}

// ---------------------------------------------------------------------------
// DrawInventory (shared helper)
// ---------------------------------------------------------------------------

void InventoryPanel::DrawInventory(const NF::Game::Inventory& inv,
                                   float x, float& cy, float w, float maxY)
{
    const auto& t     = ActiveTheme();
    const float dpi   = m_Renderer->GetDpiScale();
    const float lineH = 15.f * dpi;
    const float colW  = w * 0.6f;

    bool anyItem = false;
    for (int i = 0; i < NF::Game::Inventory::kMaxSlots; ++i) {
        if (cy + lineH > maxY) break;
        const auto& slot = inv.GetSlot(i);
        if (slot.IsEmpty()) continue;
        anyItem = true;
        const char* name = NF::Game::ResourceTypeName(slot.type);
        m_Renderer->DrawText(name,                              x,        cy, t.textPrimary,   1.f);
        m_Renderer->DrawText(std::to_string(slot.count).c_str(), x + colW, cy, t.textSecondary, 1.f);
        cy += lineH;
    }
    if (!anyItem) {
        m_Renderer->DrawText("(empty)", x, cy, t.textSecondary, 1.f);
        cy += lineH;
    }
    (void)w;
}

// ---------------------------------------------------------------------------
// DrawStorageBoxes
// ---------------------------------------------------------------------------

void InventoryPanel::DrawStorageBoxes(float x, float& cy, float w, float maxY)
{
    const auto& t     = ActiveTheme();
    const float dpi   = m_Renderer->GetDpiScale();
    const float lineH = 15.f * dpi;

    if (!m_Storage) {
        m_Renderer->DrawText("(no storage system)", x, cy, t.textSecondary, 1.f);
        cy += lineH;
        return;
    }

    // We need mutable access to iterate; re-expose via the mutable pointer.
    auto* storage = m_Storage;
    // Iterate by calling FindNearest with an infinite radius to discover boxes.
    // Since there's no iterator on StorageSystem, we scan IDs from 1 upward.
    // A simpler approach: we added GetWrecks() to SalvageSystem; for storage
    // we access boxes via StorageSystem's public GetBox by walking IDs 1..N.
    // Instead we note that EditorApp wires the default box at ID 1; we show
    // what's available through the known "Homebase Storage" box.
    static constexpr NF::Game::Gameplay::StorageBoxId kHomebaseBoxId = 1u;

    const NF::Game::Inventory* boxInv = storage->GetBox(kHomebaseBoxId);
    if (boxInv) {
        m_Renderer->DrawText("Homebase Storage:", x, cy, t.textPrimary, 1.f);
        cy += lineH;
        DrawInventory(*boxInv, x + 8.f * dpi, cy, w - 8.f * dpi, maxY);
    } else {
        m_Renderer->DrawText("(no boxes placed)", x, cy, t.textSecondary, 1.f);
        cy += lineH;
    }
    (void)w;
}

// ---------------------------------------------------------------------------
// DrawWreckSites
// ---------------------------------------------------------------------------

void InventoryPanel::DrawWreckSites(float x, float& cy, float w, float maxY)
{
    const auto& t     = ActiveTheme();
    const float dpi   = m_Renderer->GetDpiScale();
    const float lineH = 15.f * dpi;

    if (!m_Salvage) {
        m_Renderer->DrawText("(no salvage system)", x, cy, t.textSecondary, 1.f);
        cy += lineH;
        return;
    }

    const auto& wrecks = m_Salvage->GetWrecks();
    if (wrecks.empty()) {
        m_Renderer->DrawText("(no wrecks in world)", x, cy, t.textSecondary, 1.f);
        cy += lineH;
        return;
    }

    const float colW = w * 0.55f;
    for (const auto& wreck : wrecks) {
        if (cy + lineH > maxY) break;
        const std::string lootStr = std::to_string(wreck.TotalLoot()) + " items";
        const uint32_t color = wreck.IsEmpty() ? t.textSecondary : t.textPrimary;
        m_Renderer->DrawText(wreck.name.c_str(),    x,        cy, color,           1.f);
        m_Renderer->DrawText(lootStr.c_str(),        x + colW, cy, t.textSecondary, 1.f);
        cy += lineH;
    }
    (void)w;
}

} // namespace NF::Editor

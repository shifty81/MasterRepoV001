#include "Editor/Panels/InventoryPanel.h"
#include "Editor/Panels/EditorTheme.h"
#include "Editor/Application/EditorInputState.h"
#include "UI/Rendering/UIRenderer.h"
#include "Game/Interaction/ResourceItem.h"
#include <string>

namespace NF::Editor {

// ---------------------------------------------------------------------------
// Tradeable resource types shown in the Give buttons
// ---------------------------------------------------------------------------

static constexpr NF::Game::ResourceType kGiveTypes[] = {
    NF::Game::ResourceType::Ore,
    NF::Game::ResourceType::Stone,
    NF::Game::ResourceType::Metal,
    NF::Game::ResourceType::Rock,
    NF::Game::ResourceType::Ice,
    NF::Game::ResourceType::Organic,
};
static constexpr int kNumGiveTypes = static_cast<int>(
    sizeof(kGiveTypes) / sizeof(kGiveTypes[0]));

// ---------------------------------------------------------------------------
// DrawButton
// ---------------------------------------------------------------------------

bool InventoryPanel::DrawButton(float x, float y, float w, float h,
                                 const char* label, bool enabled)
{
    if (!m_Renderer) return false;

    const auto& t   = ActiveTheme();
    const float dpi = m_Renderer->GetDpiScale();

    const bool hovered = enabled && m_Input &&
        m_Input->mouseX >= x && m_Input->mouseX < x + w &&
        m_Input->mouseY >= y && m_Input->mouseY < y + h;
    const bool clicked = hovered && m_Input && m_Input->leftJustPressed;

    const uint32_t bg = !enabled  ? 0x33333388u
                      : hovered   ? t.worldAccent
                                  : t.titleBarBg;
    const uint32_t fg = !enabled  ? t.textSecondary
                      : hovered   ? 0xFFFFFFFFu
                                  : t.textPrimary;

    m_Renderer->DrawRect({x, y, w, h}, bg);
    m_Renderer->DrawOutlineRect({x, y, w, h}, t.panelBorder);
    m_Renderer->DrawText(label, x + 3.f * dpi, y + 2.f * dpi, fg, 1.f);

    return clicked;
}

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
    DrawBackpack(x + padX, cy, w - 2.f * padX, maxY * 0.38f + y);

    if (cy + 4.f * dpi < maxY) {
        m_Renderer->DrawRect({x, cy, w, 1.f}, t.separator);
        cy += 4.f * dpi;
    }

    // ---- Storage boxes -------------------------------------------------------
    m_Renderer->DrawText("Storage Boxes", x + padX, cy, t.textHeader, 1.f);
    cy += lineH;
    DrawStorageBoxes(x + padX, cy, w - 2.f * padX, maxY * 0.70f + y);

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

    // ---- Current contents ----------------------------------------------------
    DrawInventoryReadOnly(*m_PlayerInv, x, cy, w, maxY - lineH * 3.f);

    // ---- Transfer All button -------------------------------------------------
    if (m_Storage && cy + lineH <= maxY) {
        static constexpr NF::Game::Gameplay::StorageBoxId kHomebaseBoxId = 1u;
        const bool canTransfer = !m_PlayerInv->IsFull() ||
                                 m_Storage->GetBox(kHomebaseBoxId) != nullptr;

        const float btnW = 90.f * dpi;
        const float btnH = 14.f * dpi;
        if (DrawButton(x, cy, btnW, btnH, "Transfer All", canTransfer))
        {
            // Deposit all held items into Homebase Storage
            for (int s = 0; s < NF::Game::Inventory::kMaxSlots; ++s) {
                const auto& slot = m_PlayerInv->GetSlot(s);
                if (slot.IsEmpty()) continue;
                m_Storage->Deposit(kHomebaseBoxId, *m_PlayerInv,
                                   slot.type, slot.count);
            }
        }
        cy += btnH + 2.f * dpi;
    }

    // ---- Give-item row (test authoring buttons) --------------------------------
    if (cy + lineH <= maxY) {
        m_Renderer->DrawText("Give:", x, cy, t.textSecondary, 1.f);
        const float giveStartX = x + 30.f * dpi;
        const float btnW       = 24.f * dpi;
        const float btnH       = 13.f * dpi;
        const float gap        = 3.f * dpi;
        float bx = giveStartX;
        for (int i = 0; i < kNumGiveTypes && cy + btnH <= maxY; ++i) {
            if (bx + btnW > x + w) break;
            const char* name = NF::Game::ResourceTypeName(kGiveTypes[i]);
            // Abbreviate to first 2 chars so buttons stay small
            char abbr[3] = {name[0], name[1], '\0'};
            const bool canGive = !m_PlayerInv->IsFull()
                                 || m_PlayerInv->HasItem(kGiveTypes[i]);
            if (DrawButton(bx, cy, btnW, btnH, abbr, canGive))
                m_PlayerInv->AddItem(kGiveTypes[i], 5u);
            bx += btnW + gap;
        }
        cy += btnH + 2.f * dpi;
    }
}

// ---------------------------------------------------------------------------
// DrawInventoryReadOnly (shared helper for read-only slot display)
// ---------------------------------------------------------------------------

void InventoryPanel::DrawInventoryReadOnly(const NF::Game::Inventory& inv,
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

    static constexpr NF::Game::Gameplay::StorageBoxId kHomebaseBoxId = 1u;
    NF::Game::Inventory* boxInv = m_Storage->GetBox(kHomebaseBoxId);
    if (boxInv) {
        m_Renderer->DrawText("Homebase Storage:", x, cy, t.textPrimary, 1.f);
        cy += lineH;

        const float indent = 8.f * dpi;

        // Show each slot with a Withdraw button
        bool anyItem = false;
        for (int i = 0; i < NF::Game::Inventory::kMaxSlots; ++i) {
            if (cy + lineH > maxY) break;
            const auto& slot = boxInv->GetSlot(i);
            if (slot.IsEmpty()) continue;
            anyItem = true;

            const char* name = NF::Game::ResourceTypeName(slot.type);
            const float colW = w * 0.55f;
            m_Renderer->DrawText(name,                               x + indent,        cy, t.textPrimary,   1.f);
            m_Renderer->DrawText(std::to_string(slot.count).c_str(), x + indent + colW, cy, t.textSecondary, 1.f);

            // Withdraw -1 button when player inventory is wired
            if (m_PlayerInv) {
                const bool canWithdraw = slot.count > 0 && !m_PlayerInv->IsFull();
                const float btnX = x + w - 28.f * dpi;
                const float btnH = lineH - 2.f * dpi;
                if (DrawButton(btnX, cy, 26.f * dpi, btnH, "-1", canWithdraw))
                    m_Storage->Withdraw(kHomebaseBoxId, *m_PlayerInv, slot.type, 1u);
            }

            cy += lineH;
        }
        if (!anyItem) {
            m_Renderer->DrawText("(empty)", x + indent, cy, t.textSecondary, 1.f);
            cy += lineH;
        }
    } else {
        m_Renderer->DrawText("(no boxes placed)", x, cy, t.textSecondary, 1.f);
        cy += lineH;
    }
}

// ---------------------------------------------------------------------------
// DrawWreckSites
// ---------------------------------------------------------------------------

void InventoryPanel::DrawWreckSites(float x, float& cy, [[maybe_unused]] float w, float maxY)
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
}

} // namespace NF::Editor

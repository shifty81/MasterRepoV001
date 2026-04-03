#include "Editor/Panels/Inspector.h"
#include "UI/Rendering/UIRenderer.h"
#include "Game/World/GameWorld.h"
#include "Game/Voxel/VoxelType.h"
#include <string>

namespace NF::Editor {

void Inspector::Update([[maybe_unused]] float dt) {}

void Inspector::SetSelectedVoxel(const nf::SelectionHandle& handle,
                                  const NF::Game::GameWorld& gameWorld)
{
    m_SelectedEntity = NullEntity;
    m_World          = nullptr;
    m_VoxelSelected  = true;
    m_VoxelLabel     = handle.label;

    // Decode packed position from handle.id.
    auto unpack = [](uint64_t packed, int shift) -> int32_t {
        int32_t val = static_cast<int32_t>((packed >> shift) & 0xFFFFF);
        if (val & 0x80000) val |= ~0xFFFFF; // sign-extend
        return val;
    };
    m_VoxelX = unpack(handle.id, 0);
    m_VoxelY = unpack(handle.id, 20);
    m_VoxelZ = unpack(handle.id, 40);

    m_VoxelType = static_cast<int>(
        gameWorld.GetVoxelEditApi().GetVoxel(m_VoxelX, m_VoxelY, m_VoxelZ));
}

void Inspector::Draw(float x, float y, float w, float h) {
    if (!m_Renderer) return;

    static constexpr uint32_t kTextColor   = 0xB0B0B0FF;
    static constexpr uint32_t kLabelColor  = 0x808080FF;
    static constexpr uint32_t kValueColor  = 0xD0D0D0FF;
    static constexpr uint32_t kHeaderColor = 0xCCCCCCFF;
    static constexpr uint32_t kSepColor    = 0x444444FF;
    const float dpi   = m_Renderer->GetDpiScale();
    const float lineH = 20.f * dpi;
    const float padX  = 6.f * dpi;
    const float scale = 2.f;
    float cy = y + 4.f * dpi;

    if (m_VoxelSelected) {
        // ---- Voxel inspection ----
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

    if (m_SelectedEntity == NullEntity || !m_World) {
        m_Renderer->DrawText("No selection", x + padX, cy, kLabelColor, scale);
        return;
    }

    // ---- Entity inspection ----
    std::string label = "Entity " + std::to_string(m_SelectedEntity);
    m_Renderer->DrawText(label, x + padX, cy, kHeaderColor, scale);
    cy += lineH;
    m_Renderer->DrawRect({x, cy, w, 1.f}, kSepColor);
    cy += 4.f * dpi;

    m_Renderer->DrawText("Components:", x + padX, cy, kLabelColor, scale);
    cy += lineH;

    // List entity in the live entity set to confirm it exists.
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

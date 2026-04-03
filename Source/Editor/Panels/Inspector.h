#pragma once
#include "Engine/ECS/World.h"
#include "Editor/Selection/SelectionService.h"

namespace NF { class UIRenderer; }
namespace NF::Game { class GameWorld; }

namespace NF::Editor {

/// @brief Panel that displays and edits components on the selected entity or voxel.
class Inspector {
public:
    /// @brief Set the entity and world to inspect.
    /// @param id    Entity to inspect; pass NullEntity to clear.
    /// @param world World that owns the entity.
    void SetSelectedEntity(EntityId id, World* world) noexcept {
        m_SelectedEntity = id;
        m_World          = world;
        m_VoxelSelected  = false;
    }

    /// @brief Set a voxel selection for inspection.
    void SetSelectedVoxel(const nf::SelectionHandle& handle, const NF::Game::GameWorld& gameWorld);

    /// @brief Set the UIRenderer used for drawing.
    void SetUIRenderer(UIRenderer* r) noexcept { m_Renderer = r; }

    /// @brief Advance panel state.
    void Update(float dt);

    /// @brief Draw all components of the selected entity within the given region.
    void Draw(float x, float y, float w, float h);

private:
    EntityId     m_SelectedEntity{NullEntity};
    World*       m_World{nullptr};
    UIRenderer*  m_Renderer{nullptr};

    // Voxel selection data
    bool         m_VoxelSelected{false};
    int32_t      m_VoxelX{0}, m_VoxelY{0}, m_VoxelZ{0};
    int          m_VoxelType{0};
    std::string  m_VoxelLabel;
};

} // namespace NF::Editor

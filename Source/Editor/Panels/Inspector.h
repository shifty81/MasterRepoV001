#pragma once
#include "Engine/ECS/World.h"
#include "Editor/Application/EditorInputState.h"
#include "Editor/Inspector/PropertyInspectorSystem.h"
#include "Editor/Selection/SelectionService.h"
#include <functional>

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

    /// @brief Attach the property inspector system that supplies the structured
    ///        property grid.  When set and populated, it takes precedence over
    ///        the basic entity / voxel rendering paths.
    void SetPropertyInspectorSystem(nf::PropertyInspectorSystem* sys) noexcept { m_PropSystem = sys; }

    /// @brief Set the UIRenderer used for drawing.
    void SetUIRenderer(UIRenderer* r) noexcept { m_Renderer = r; }

    /// @brief Provide the current per-frame OS input state.
    void SetInputState(const EditorInputState* input) noexcept { m_Input = input; }

    /// @brief Set callback invoked when a property edit marks the world dirty.
    void SetOnPropertyEdited(std::function<void()> cb) { m_OnPropertyEdited = std::move(cb); }

    /// @brief Advance panel state.
    void Update(float dt);

    /// @brief Draw all components of the selected entity within the given region.
    void Draw(float x, float y, float w, float h);

private:
    EntityId     m_SelectedEntity{NullEntity};
    World*       m_World{nullptr};
    UIRenderer*  m_Renderer{nullptr};
    const EditorInputState* m_Input{nullptr};

    /// Non-owning pointer to the property system fed by EditorApp.
    nf::PropertyInspectorSystem* m_PropSystem{nullptr};

    /// Callback when a property is edited.
    std::function<void()> m_OnPropertyEdited;

    // Voxel selection data
    bool         m_VoxelSelected{false};
    int32_t      m_VoxelX{0}, m_VoxelY{0}, m_VoxelZ{0};
    int          m_VoxelType{0};
    std::string  m_VoxelLabel;

    /// @brief Check if a click hit a property row and apply the appropriate edit.
    void HandlePropertyClick(float x, float y, float w, float rowY, float lineH,
                             const nf::PropertyEntry& entry, size_t entryIndex);
};

} // namespace NF::Editor

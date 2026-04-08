#pragma once
#include "Editor/Application/EditorInputState.h"
#include "Engine/ECS/World.h"
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace NF { class UIRenderer; }

namespace NF::Editor {

/// @brief Chunk metadata fed from the live GameWorld each frame.
struct RuntimeChunkMetadata {
    std::uint64_t id     = 0;   ///< Stable chunk ID (packed from coord).
    std::string   label;        ///< Human-readable chunk label.
    int           coordX = 0;
    int           coordY = 0;
    int           coordZ = 0;
    bool          loaded = true;
    bool          dirty  = false;
};

/// @brief Panel that lists all entities in the active world and loaded chunks.
class SceneOutliner {
public:
    /// @brief Set the world to display.
    /// @param world Non-owning pointer; must outlive this panel.
    void SetWorld(World* world) noexcept { m_World = world; }

    /// @brief Set the UIRenderer used for drawing.
    void SetUIRenderer(UIRenderer* r) noexcept { m_Renderer = r; }

    /// @brief Provide the current per-frame OS input state.
    /// @param input Non-owning pointer updated by EditorApp each frame.
    void SetInputState(const EditorInputState* input) noexcept { m_Input = input; }

    /// @brief Feed live chunk metadata so the outliner can show the world tree.
    /// Called each frame from EditorApp::TickFrame.
    void SetChunkData(std::string worldName, std::vector<RuntimeChunkMetadata> chunks);

    /// @brief Advance panel state.
    void Update(float dt);

    /// @brief Draw the panel content within the given region.
    void Draw(float x, float y, float w, float h);

    /// @brief Return the currently selected entity.
    [[nodiscard]] EntityId GetSelectedEntity() const noexcept { return m_SelectedEntity; }

    /// @brief Register a callback invoked whenever the entity selection changes.
    void SetOnSelectionChanged(std::function<void(EntityId)> cb) { m_OnSelectionChanged = std::move(cb); }

    /// @brief Register a callback invoked when a chunk row is clicked.
    void SetOnChunkSelected(std::function<void(const RuntimeChunkMetadata&)> cb) { m_OnChunkSelected = std::move(cb); }

    /// @brief Register a callback invoked when the world root node is clicked.
    void SetOnWorldSelected(std::function<void()> cb) { m_OnWorldSelected = std::move(cb); }

private:
    World*                        m_World{nullptr};
    UIRenderer*                   m_Renderer{nullptr};
    const EditorInputState*       m_Input{nullptr};
    EntityId                      m_SelectedEntity{NullEntity};
    std::function<void(EntityId)>                   m_OnSelectionChanged;
    std::function<void(const RuntimeChunkMetadata&)> m_OnChunkSelected;
    std::function<void()>                            m_OnWorldSelected;

    std::uint64_t                    m_SelectedChunkId{0}; ///< 0 = no chunk selected.
    bool                             m_WorldNodeSelected{false};

    // World / chunk tree data (updated each frame via SetChunkData)
    std::string                      m_WorldName;
    std::vector<RuntimeChunkMetadata> m_Chunks;
};

} // namespace NF::Editor

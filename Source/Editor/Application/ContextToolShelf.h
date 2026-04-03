#pragma once

#include "UI/Rendering/UIRenderer.h"
#include "Editor/Application/EditorModeManager.h"
#include "Editor/Application/EditorInputState.h"
#include "Editor/Tools/EditorToolContext.h"
#include <cstdint>

namespace NF::Editor {

/// @brief A thin horizontal bar above the viewport that shows
///        mode-specific tool controls.
///
/// The shelf content changes when the active @c EditorMode changes.
/// Each mode populates the shelf with relevant quick-access controls
/// (e.g. brush shape in Voxel mode, snap settings in Select mode).
class ContextToolShelf {
public:
    /// @brief Logical height (pre-DPI) of the tool shelf.
    static constexpr float kShelfHeight = 22.f;

    ContextToolShelf() = default;

    void SetUIRenderer(UIRenderer* r) noexcept { m_Renderer = r; }
    void SetInputState(const EditorInputState* s) noexcept { m_Input = s; }
    void SetToolContext(nf::EditorToolContext* c) noexcept { m_ToolContext = c; }

    /// @brief Draw the shelf for the given mode.
    void Draw(float x, float y, float w, float h, EditorMode mode);

private:
    void DrawSelectShelf(float x, float y, float w, float h);
    void DrawVoxelShelf(float x, float y, float w, float h);
    void DrawEntityShelf(float x, float y, float w, float h);
    void DrawWorldShelf(float x, float y, float w, float h);
    void DrawDebugShelf(float x, float y, float w, float h);

    /// @brief Draw a compact label+value pair.
    void DrawLabel(float& cx, float y, float h, const char* text, uint32_t color);

    /// @brief Draw a small clickable tool button in the shelf.
    bool DrawShelfButton(float& cx, float y, float h,
                         const char* label, bool active);

    UIRenderer*             m_Renderer{nullptr};
    const EditorInputState* m_Input{nullptr};
    nf::EditorToolContext*  m_ToolContext{nullptr};
};

} // namespace NF::Editor

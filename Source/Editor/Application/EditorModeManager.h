#pragma once

#include "UI/Rendering/UIRenderer.h"
#include "Editor/Application/EditorInputState.h"
#include "Editor/Panels/EditorTheme.h"
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace NF::Editor {

/// @brief Top-level editor editing modes.
///
/// Each mode controls which context-tool-shelf controls are visible
/// and may influence inspector tab defaults.
enum class EditorMode : uint8_t {
    Select   = 0,
    Voxels   = 1,
    Entities = 2,
    World    = 3,
    Debug    = 4,
    Count
};

/// @brief Returns a human-readable name for the mode.
[[nodiscard]] const char* EditorModeName(EditorMode mode) noexcept;

/// @brief Manages the active editor mode and renders the mode tab strip.
///
/// The mode tab strip is a thin horizontal bar of clickable tabs.
/// When the active mode changes, a user-supplied callback fires so that
/// the host (EditorApp) can update the context tool shelf and other
/// mode-dependent UI.
class EditorModeManager {
public:
    /// @brief Height of the mode tab strip in logical (pre-DPI) pixels.
    static constexpr float kStripHeight = 24.f;

    EditorModeManager() = default;

    void SetUIRenderer(UIRenderer* r) noexcept { m_Renderer = r; }
    void SetInputState(const EditorInputState* s) noexcept { m_Input = s; }

    /// @brief Set a callback invoked when the active mode changes.
    void SetOnModeChanged(std::function<void(EditorMode)> cb) { m_OnModeChanged = std::move(cb); }

    /// @brief Returns the currently active mode.
    [[nodiscard]] EditorMode GetActiveMode() const noexcept { return m_ActiveMode; }

    /// @brief Programmatically switch to a mode (fires callback).
    void SetActiveMode(EditorMode mode);

    /// @brief Draw the mode tab strip at the given position and size.
    void Draw(float x, float y, float w, float h);

private:
    UIRenderer*              m_Renderer{nullptr};
    const EditorInputState*  m_Input{nullptr};
    EditorMode               m_ActiveMode{EditorMode::Select};
    std::function<void(EditorMode)> m_OnModeChanged;
};

} // namespace NF::Editor

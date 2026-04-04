#pragma once
#include "Editor/Panels/LiveProfilerBackend.h"
#include "Editor/Application/EditorInputState.h"
#include <cstdint>

namespace NF { class UIRenderer; }

namespace NF::Editor {

/// @brief Visual panel that displays frame-time history and per-scope
///        breakdown from the LiveProfilerBackend.
///
/// Renders a rolling frame-time graph with a horizontal 16.6 ms target
/// line and a text-based scope-duration list underneath.
class LiveProfilerPanel {
public:
    /// @brief Set the UIRenderer used for drawing.
    void SetUIRenderer(UIRenderer* r) noexcept { m_Renderer = r; }

    /// @brief Provide the current per-frame OS input state.
    void SetInputState(const EditorInputState* input) noexcept { m_Input = input; }

    /// @brief Attach the profiler backend whose data this panel displays.
    void SetBackend(const LiveProfilerBackend* backend) noexcept { m_Backend = backend; }

    /// @brief Advance panel state (no-op for now).
    void Update(float dt);

    /// @brief Draw the profiler panel within the given region.
    void Draw(float x, float y, float w, float h);

private:
    UIRenderer*                m_Renderer{nullptr};
    const EditorInputState*    m_Input{nullptr};
    const LiveProfilerBackend* m_Backend{nullptr};

    /// @brief Draw the rolling frame-time graph.
    void DrawGraph(float x, float y, float w, float h);

    /// @brief Draw the scope-duration list.
    void DrawScopes(float x, float y, float w, float h);
};

} // namespace NF::Editor

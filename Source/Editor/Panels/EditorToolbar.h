#pragma once
#include "Editor/Application/EditorInputState.h"
#include "UI/Rendering/UIRenderer.h"
#include "Game/Interaction/InteractionLoop.h"
#include "Editor/Tools/EditorToolContext.h"

namespace NF::Editor {

/// @brief Horizontal toolbar strip rendered above the docking panel region.
///
/// Provides Play/Reset, tool mode selection, and Launch Game buttons.
class EditorToolbar {
public:
    /// @brief Height of the toolbar strip in logical pixels.
    static constexpr float kHeight = 28.f;

    void SetUIRenderer(UIRenderer* r)            noexcept { m_Renderer = r; }
    void SetInteractionLoop(NF::Game::InteractionLoop* loop) noexcept { m_Loop = loop; }
    void SetInputState(EditorInputState* input)  noexcept { m_Input = input; }
    void SetToolContext(nf::EditorToolContext* ctx) noexcept { m_ToolContext = ctx; }

    /// @brief Draw the toolbar into the given rectangle.
    void Draw(float x, float y, float w, float h);

    /// @brief Returns the fixed toolbar height (same as kHeight).
    [[nodiscard]] float GetHeight() const noexcept { return kHeight; }

    /// @brief Returns true when Play-In-Editor is active.
    [[nodiscard]] bool IsPieActive() const noexcept { return m_PieActive; }

private:
    UIRenderer*                m_Renderer{nullptr};
    NF::Game::InteractionLoop* m_Loop{nullptr};
    EditorInputState*          m_Input{nullptr};
    nf::EditorToolContext*     m_ToolContext{nullptr};
    bool                       m_PieActive{false};  ///< True while Play-In-Editor is running.

    /// @brief Draw a labelled button; returns true when clicked this frame.
    bool DrawButton(float x, float y, float w, float h, const char* label,
                    uint32_t bgColor, uint32_t textColor);
};

} // namespace NF::Editor

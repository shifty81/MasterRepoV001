#pragma once
#include "Editor/Application/EditorInputState.h"
#include "Editor/Commands/EditorCommandRegistry.h"
#include "UI/Rendering/UIRenderer.h"
#include "Game/Interaction/InteractionLoop.h"
#include "Editor/Tools/EditorToolContext.h"

namespace NF::Editor {

/// @brief Two-row header strip: menu bar (File/Edit/View) + tool button row.
///
/// The menu bar occupies the top kMenuHeight logical pixels; the tool button
/// row fills the remaining kBtnHeight pixels.  Total height = kHeight.
class EditorToolbar {
public:
    static constexpr float kMenuHeight = 22.f;  ///< Menu bar row (logical pixels).
    static constexpr float kBtnHeight  = 28.f;  ///< Tool button row (logical pixels).
    static constexpr float kHeight     = kMenuHeight + kBtnHeight; ///< Total height.

    /// @brief Play-In-Editor simulation state.
    enum class PieState { Stopped, Playing, Paused };

    void SetUIRenderer(UIRenderer* r)                        noexcept { m_Renderer = r; }
    void SetInteractionLoop(NF::Game::InteractionLoop* loop) noexcept { m_Loop = loop; }
    void SetInputState(EditorInputState* input)              noexcept { m_Input = input; }
    void SetToolContext(nf::EditorToolContext* ctx)           noexcept { m_ToolContext = ctx; }
    void SetCommandRegistry(nf::EditorCommandRegistry* reg)  noexcept { m_CommandRegistry = reg; }

    /// @brief Draw the toolbar bar (menu headers + tool buttons).
    void Draw(float x, float y, float w, float h);

    /// @brief Draw any open drop-down menu on top of all other panels.
    ///        Must be called AFTER docking panel drawing so it appears on top.
    void DrawDropdown();

    /// @brief Returns the total toolbar height (same as kHeight).
    [[nodiscard]] float GetHeight() const noexcept { return kHeight; }

    /// @brief Returns true only when the simulation is actively ticking (not paused, not stopped).
    [[nodiscard]] bool IsPiePlaying() const noexcept { return m_PieState == PieState::Playing; }

    /// @brief Returns true when PIE is paused (simulation frozen but not reset).
    [[nodiscard]] bool IsPiePaused()  const noexcept { return m_PieState == PieState::Paused; }

    /// @brief Returns the full PIE state.
    [[nodiscard]] PieState GetPieState() const noexcept { return m_PieState; }

private:
    UIRenderer*                m_Renderer{nullptr};
    NF::Game::InteractionLoop* m_Loop{nullptr};
    EditorInputState*          m_Input{nullptr};
    nf::EditorToolContext*     m_ToolContext{nullptr};
    nf::EditorCommandRegistry* m_CommandRegistry{nullptr};
    PieState                   m_PieState{PieState::Stopped}; ///< Current PIE simulation state.

    // Menu state
    int   m_OpenMenuIdx{-1};  ///< Index of the currently open menu; -1 = none.
    float m_DrawX{0.f};       ///< Cached x origin from the most recent Draw() call.
    float m_DrawY{0.f};       ///< Cached y origin from the most recent Draw() call.

    /// @brief Draw a labelled button; returns true when clicked this frame.
    ///        Pass enabled=false to render the button dimmed and non-interactive.
    bool DrawButton(float x, float y, float w, float h, const char* label,
                    uint32_t bgColor, uint32_t textColor, bool enabled = true);
};

} // namespace NF::Editor

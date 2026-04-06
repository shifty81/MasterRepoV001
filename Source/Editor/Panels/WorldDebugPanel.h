#pragma once
#include "Game/World/WorldDebugOverlay.h"
#include "Game/Interaction/RigState.h"
#include <string>
#include <vector>

namespace NF { class UIRenderer; }

namespace NF::Editor {

/// @brief Editor panel that renders the WorldDebugOverlay text lines
///        inside the docking layout.
class WorldDebugPanel {
public:
    /// @brief Set the UIRenderer used for drawing.
    void SetUIRenderer(UIRenderer* r) noexcept { m_Renderer = r; }

    /// @brief Provide the debug overlay to display.
    void SetOverlay(NF::Game::WorldDebugOverlay* overlay) noexcept { m_Overlay = overlay; }

    /// @brief Provide the rig state to display (may be null).
    void SetRigState(const NF::Game::RigState* rig) noexcept { m_Rig = rig; }

    /// @brief Advance panel state.
    void Update(float dt);

    /// @brief Draw the debug overlay within the given region.
    void Draw(float x, float y, float w, float h);

private:
    UIRenderer*                   m_Renderer{nullptr};
    NF::Game::WorldDebugOverlay*  m_Overlay{nullptr};
    const NF::Game::RigState*     m_Rig{nullptr};
};

} // namespace NF::Editor

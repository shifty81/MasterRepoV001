#pragma once
#include "Game/Gameplay/Factions/FactionRegistry.h"

namespace NF { class UIRenderer; }

namespace NF::Editor {

struct EditorInputState;

/// @brief Editor panel showing player reputation with each faction.
///
/// Displays the three built-in factions (Miners Guild, Traders Union, Raiders)
/// with colour-coded reputation bars and standing text.  Values are read from
/// a non-owning pointer to a FactionRegistry which may be the Orchestrator's
/// m_Factions or an editor-local copy.
class FactionPanel {
public:
    FactionPanel() = default;

    // -------------------------------------------------------------------------
    // Wiring
    // -------------------------------------------------------------------------

    void SetUIRenderer(NF::UIRenderer* r)           noexcept { m_Renderer = r; }
    void SetInputState(const EditorInputState* inp) noexcept { m_Input = inp; }
    void SetFactionRegistry(NF::Game::Gameplay::FactionRegistry* reg) noexcept {
        m_Registry = reg;
    }

    // -------------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------------

    void Update(float dt) noexcept;
    void Draw(float x, float y, float w, float h);

private:
    NF::UIRenderer*                             m_Renderer{nullptr};
    const EditorInputState*                     m_Input{nullptr};
    NF::Game::Gameplay::FactionRegistry*        m_Registry{nullptr};
};

} // namespace NF::Editor

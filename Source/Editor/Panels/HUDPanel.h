#pragma once
#include "Editor/Application/EditorInputState.h"
#include "Game/Interaction/InteractionLoop.h"
#include "Game/Movement/PlayerMovement.h"

namespace NF { class UIRenderer; }
namespace NF::Game { class GameWorld; }

namespace NF::Editor {

class HUDPanel {
public:
    void SetUIRenderer(UIRenderer* r)                 noexcept { m_Renderer = r; }
    void SetInputState(const EditorInputState* i)     noexcept { m_Input = i; }
    void SetInteractionLoop(NF::Game::InteractionLoop* loop) noexcept { m_Loop = loop; }
    void SetGameWorld(const NF::Game::GameWorld* world) noexcept { m_GameWorld = world; }
    void SetPlayerMovement(const NF::Game::PlayerMovement* pm) noexcept { m_Player = pm; }

    /// @brief Signal that a mining action fired this frame; triggers the flash.
    void NotifyMineFired() noexcept { m_MineFlashTimer = kMineFlashDuration; }

    void Update(float dt);
    void Draw(float x, float y, float w, float h);

private:
    UIRenderer*                       m_Renderer{nullptr};
    const EditorInputState*           m_Input{nullptr};
    NF::Game::InteractionLoop*        m_Loop{nullptr};
    const NF::Game::GameWorld*        m_GameWorld{nullptr};
    const NF::Game::PlayerMovement*   m_Player{nullptr};

    float m_MineFlashTimer{0.f};
    static constexpr float kMineFlashDuration = 0.25f; ///< Seconds the flash stays visible.

    void DrawBar(float x, float y, float w, float barH,
                 float fraction, uint32_t fillColor,
                 const char* label) const;
};

} // namespace NF::Editor

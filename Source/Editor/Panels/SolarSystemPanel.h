#pragma once
// SolarSystemPanel.h — Editor panel showing the dev solar system as a
// flat 2D map with selectable, editable celestial bodies and placed items.

#include "Game/Gameplay/SolarSystem/DevSolarSystem.h"
#include "Game/Gameplay/PCG/PCGItemGen.h"
#include <cstdint>
#include <functional>

namespace NF { class UIRenderer; }

namespace NF::Editor {

struct EditorInputState;

/// @brief Editor panel that draws a 2D orbital map of the dev solar system.
///
/// Bodies are drawn as circles at their orbital positions.  Click to select
/// a body; its properties are shown inline and can be edited.  Resource
/// deposits and placed items are drawn for the selected body.
class SolarSystemPanel {
public:
    /// @brief Set the UIRenderer used for drawing.
    void SetUIRenderer(UIRenderer* r) noexcept { m_Renderer = r; }

    /// @brief Set the input state for hit-testing.
    void SetInputState(const EditorInputState* input) noexcept { m_Input = input; }

    /// @brief Set the solar system to visualize.
    void SetSolarSystem(NF::Game::Gameplay::DevSolarSystem* sys) noexcept { m_System = sys; }

    /// @brief Set the PCG item generator for displaying placed items.
    void SetItemGen(NF::Game::Gameplay::PCGItemGen* gen) noexcept { m_ItemGen = gen; }

    /// @brief Set a callback fired when the user selects a body.
    ///        The argument is the body's uint32_t ID (0 = deselected).
    void SetOnBodySelected(std::function<void(uint32_t)> cb) noexcept { m_OnBodySelected = std::move(cb); }

    /// @brief Set a callback fired when the user clicks "Travel to Body".
    ///        The argument is the body's uint32_t ID.
    void SetOnTravelToBody(std::function<void(uint32_t)> cb) noexcept { m_OnTravelToBody = std::move(cb); }

    /// @brief Advance panel state.
    void Update(float dt);

    /// @brief Draw the solar system flat map within the given region.
    void Draw(float x, float y, float w, float h);

    /// @brief ID of the currently selected body (0 = none).
    [[nodiscard]] uint32_t GetSelectedBodyId() const noexcept { return m_SelectedBodyId; }

private:
    UIRenderer*                              m_Renderer{nullptr};
    const EditorInputState*                  m_Input{nullptr};
    NF::Game::Gameplay::DevSolarSystem*      m_System{nullptr};
    NF::Game::Gameplay::PCGItemGen*          m_ItemGen{nullptr};

    std::function<void(uint32_t)>            m_OnBodySelected;  ///< Fired when a body is selected.
    std::function<void(uint32_t)>            m_OnTravelToBody;  ///< Fired when "Travel" is clicked.

    uint32_t  m_SelectedBodyId{0};      ///< Currently selected body.
    uint32_t  m_SelectedItemId{0};      ///< Currently selected placed item.
    float     m_Zoom{1.f};             ///< Zoom level for the flat map.
    float     m_PanX{0.f};            ///< Camera pan offset X.
    float     m_PanY{0.f};            ///< Camera pan offset Y.
    bool      m_ShowItems{true};       ///< Whether to draw placed items.
    bool      m_ShowOrbits{true};      ///< Whether to draw orbit circles.
    bool      m_EditMode{false};       ///< True when editing the selected body inline.

    /// @brief Draw the orbital map (left portion of the panel).
    void DrawOrbitalMap(float x, float y, float w, float h);

    /// @brief Draw the property inspector for the selected body (right portion).
    void DrawBodyInspector(float x, float y, float w, float h);

    /// @brief Draw placed items for the selected body on the flat map.
    void DrawItemOverlay(float cx, float cy, float scale);

    /// @brief Convert world coords to screen coords in the map.
    void WorldToScreen(float wx, float wy, float cx, float cy, float scale,
                       float& sx, float& sy) const noexcept;
};

} // namespace NF::Editor

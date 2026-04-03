#pragma once
#include "Editor/Application/EditorInputState.h"
#include "Core/Math/Matrix.h"
#include "Core/Math/Vector.h"
#include "Renderer/RHI/RenderDevice.h"

namespace NF { class UIRenderer; }

namespace NF::Editor {

/// @brief 3-D viewport with an orbit camera.
class EditorViewport {
public:
    /// @brief Initialise the viewport with the given render device.
    /// @param device Non-owning pointer; must outlive this viewport.
    void Init(RenderDevice* device);

    /// @brief Set the UIRenderer used for drawing overlays.
    void SetUIRenderer(UIRenderer* r) noexcept { m_Renderer = r; }

    /// @brief Provide the current per-frame OS input state.
    /// @param input Non-owning pointer updated by EditorApp each frame.
    void SetInputState(const EditorInputState* input) noexcept { m_Input = input; }

    /// @brief Resize the viewport framebuffer.
    /// @param width  New width in pixels.
    /// @param height New height in pixels.
    void Resize(int width, int height);

    /// @brief Advance camera state using mouse/keyboard input.
    /// @param dt Delta time in seconds.
    void Update(float dt);

    /// @brief Issue draw calls for this viewport within the given region.
    void Draw(float x, float y, float w, float h);

    /// @brief Return the current view matrix.
    [[nodiscard]] Matrix4x4 GetViewMatrix() const noexcept;

    /// @brief Return the current projection matrix.
    [[nodiscard]] Matrix4x4 GetProjectionMatrix() const noexcept;

    /// @brief Compute a world-space pick ray from viewport-local mouse coords.
    /// @param mouseX  Mouse X in window coordinates.
    /// @param mouseY  Mouse Y in window coordinates.
    /// @param outOrigin    Ray origin (camera eye).
    /// @param outDirection Ray direction (normalised).
    /// @return True if the mouse is inside the viewport bounds.
    [[nodiscard]] bool PickRay(float mouseX, float mouseY,
                               Vector3& outOrigin, Vector3& outDirection) const noexcept;

    /// @brief Whether the mouse cursor is currently inside the viewport.
    [[nodiscard]] bool IsMouseInside() const noexcept;

    /// @brief Mark the viewport as having real 3D scene content rendered.
    void SetSceneRendered(bool rendered) noexcept { m_SceneRendered = rendered; }

    /// @brief Cached viewport panel bounds (set each Draw call).
    [[nodiscard]] float GetBoundsX() const noexcept { return m_BoundsX; }
    [[nodiscard]] float GetBoundsY() const noexcept { return m_BoundsY; }
    [[nodiscard]] float GetBoundsW() const noexcept { return m_BoundsW; }
    [[nodiscard]] float GetBoundsH() const noexcept { return m_BoundsH; }

private:
    RenderDevice*           m_Device{nullptr};
    UIRenderer*             m_Renderer{nullptr};
    const EditorInputState* m_Input{nullptr};

    int   m_Width{1280};
    int   m_Height{720};

    // Orbit camera state
    float   m_Pitch{0.3f};   ///< Vertical angle (radians, clamped).
    float   m_Yaw{0.f};      ///< Horizontal angle (radians).
    float   m_Zoom{5.f};     ///< Distance from target.
    Vector3 m_Target{};      ///< Orbit pivot point.

    // Cached viewport bounds (set each Draw call, read in Update).
    float m_BoundsX{0.f};
    float m_BoundsY{0.f};
    float m_BoundsW{0.f};
    float m_BoundsH{0.f};

    bool  m_SceneRendered{false}; ///< True when real 3D content has been rendered.
};

} // namespace NF::Editor

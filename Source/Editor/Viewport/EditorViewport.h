#pragma once
#include "Editor/Application/EditorInputState.h"
#include "Core/Math/Matrix.h"
#include "Core/Math/Vector.h"
#include "Renderer/RHI/RenderDevice.h"
#include <cstdint>
#include <string>

namespace NF { class UIRenderer; }

namespace NF::Editor {

/// @brief Highlight state driven by the active selection.
/// Updated by EditorApp each time the selection changes.
struct ViewportHighlightState {
    std::uint64_t selectedWorldObjectId = 0; ///< Non-zero when a world object is selected.
    std::uint64_t selectedChunkId       = 0; ///< Non-zero when a chunk is selected.
    std::uint64_t selectedVoxelId       = 0; ///< Non-zero when a voxel is selected.
    std::string   highlightLabel;            ///< Human-readable label shown as overlay.
};

/// @brief Camera behaviour mode for the editor viewport.
enum class ViewportCameraMode : uint8_t {
    Orbit,      ///< Classic orbit: pivot stays fixed until manually panned.
    FocusOrbit, ///< Focus-orbit: pivot snaps to the selected object on every selection change.
};

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

    // ---- Selection highlight -----------------------------------------------

    /// @brief Update the selection highlight overlay shown during Draw().
    void SetHighlightState(ViewportHighlightState state) noexcept { m_HighlightState = std::move(state); }

    /// @brief Return the current highlight state.
    [[nodiscard]] const ViewportHighlightState& GetHighlightState() const noexcept { return m_HighlightState; }

    // ---- Camera control -------------------------------------------------

    /// @brief Set the orbit pivot (look-at target) in world space.
    void SetCameraTarget(const Vector3& target) noexcept { m_Target = target; }

    /// @brief Set the orbit distance (zoom) from the target.
    void SetCameraZoom(float zoom) noexcept { m_Zoom = zoom; }

    /// @brief Return the current orbit distance (zoom) from the target.
    [[nodiscard]] float GetCameraZoom() const noexcept { return m_Zoom; }

    /// @brief Set the vertical orbit angle in radians.
    void SetCameraPitch(float pitch) noexcept { m_Pitch = pitch; }

    /// @brief Return the current vertical orbit angle in radians.
    [[nodiscard]] float GetCameraPitch() const noexcept { return m_Pitch; }

    /// @brief Set the horizontal orbit angle in radians.
    void SetCameraYaw(float yaw) noexcept { m_Yaw = yaw; }

    /// @brief Return the current horizontal orbit angle in radians.
    [[nodiscard]] float GetCameraYaw() const noexcept { return m_Yaw; }

    /// @brief Return the current camera eye position in world space.
    [[nodiscard]] Vector3 GetCameraEye() const noexcept;

    /// @brief Return the current camera target (orbit pivot).
    [[nodiscard]] const Vector3& GetCameraTarget() const noexcept { return m_Target; }

    // ---- Camera mode -------------------------------------------------------

    /// @brief Return the active camera behaviour mode.
    [[nodiscard]] ViewportCameraMode GetCameraMode() const noexcept { return m_CameraMode; }

    /// @brief Set the camera behaviour mode.
    void SetCameraMode(ViewportCameraMode mode) noexcept { m_CameraMode = mode; }

    /// @brief Immediately snap the orbit pivot to the given world-space position.
    /// Use this to focus the camera on a selected object.
    void FocusOnPosition(const Vector3& worldPos) noexcept;

    /// @brief Enable or disable the fallback grid overlay.
    void SetShowGrid(bool show) noexcept { m_ShowGrid = show; }

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
    float   m_Pitch{0.6f};   ///< Vertical angle (radians, clamped).
    float   m_Yaw{0.f};      ///< Horizontal angle (radians).
    float   m_Zoom{100.f};   ///< Distance from target.
    Vector3 m_Target{};      ///< Orbit pivot point.

    ViewportCameraMode m_CameraMode{ViewportCameraMode::Orbit}; ///< Active camera behaviour.

    // Cached viewport bounds (set each Draw call, read in Update).
    float m_BoundsX{0.f};
    float m_BoundsY{0.f};
    float m_BoundsW{0.f};
    float m_BoundsH{0.f};

    bool  m_SceneRendered{false}; ///< True when real 3D content has been rendered.
    bool  m_ShowGrid{true};      ///< When false, suppress the fallback grid overlay.

    ViewportHighlightState m_HighlightState; ///< Current selection highlight, updated each frame.
};

} // namespace NF::Editor

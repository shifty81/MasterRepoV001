#pragma once
#include "Core/Math/Matrix.h"
#include "Core/Math/Vector.h"
#include "Engine/ECS/World.h"
#include "Renderer/RHI/RenderDevice.h"

namespace NF::Editor {

/// @brief The manipulation mode of the gizmo.
enum class GizmoMode { Translate, Rotate, Scale };

/// @brief Which axis the user is hovering or dragging.
enum class GizmoAxis { None, X, Y, Z };

/// @brief 3-D transform gizmo for manipulating entities in the viewport.
class TransformGizmo {
public:
    /// @brief Set the active gizmo mode.
    void SetMode(GizmoMode mode) noexcept { m_Mode = mode; }

    /// @brief Set the entity currently being manipulated.
    void SetSelectedEntity(EntityId id) noexcept { m_SelectedEntity = id; }

    /// @brief Set the gizmo world-space origin (typically the entity position).
    void SetPosition(const Vector3& pos) noexcept { m_Position = pos; }

    /// @brief Set whether the left mouse button is currently held.
    void SetMouseDown(bool down) noexcept { m_MouseDown = down; }

    /// @brief Supply the camera view and projection matrices so that axis
    ///        endpoints can be projected to screen space for hit testing.
    void SetCameraMatrices(const Matrix4x4& view, const Matrix4x4& proj) noexcept {
        m_ViewProj = proj * view;
    }

    /// @brief Supply the viewport pixel region so screen-space projection works.
    void SetViewportBounds(float x, float y, float w, float h) noexcept {
        m_VpX = x; m_VpY = y; m_VpW = w; m_VpH = h;
    }

    /// @brief Update gizmo interaction state.
    /// @param dt       Delta time in seconds.
    /// @param mousePos Current mouse position in window-space pixels.
    void Update(float dt, const Vector2& mousePos);

    /// @brief Draw the gizmo into the viewport.
    /// @param device Active render device.
    void Draw(RenderDevice& device);

    /// @brief Returns true while the user is dragging a gizmo handle.
    [[nodiscard]] bool IsActive() const noexcept { return m_Active; }

    /// @brief Return the accumulated translation delta since the drag started.
    [[nodiscard]] const Vector3& GetDragDelta() const noexcept { return m_DragDelta; }

    /// @brief Return which axis is currently hovered (useful for cursor feedback).
    [[nodiscard]] GizmoAxis GetHoveredAxis() const noexcept { return m_HoveredAxis; }

private:
    GizmoMode m_Mode{GizmoMode::Translate};
    EntityId  m_SelectedEntity{NullEntity};
    Vector3   m_Position{};        ///< World-space gizmo origin.
    Vector2   m_LastMousePos{};
    bool      m_Active{false};
    bool      m_MouseDown{false};
    bool      m_WasMouseDown{false};

    GizmoAxis m_HoveredAxis{GizmoAxis::None};
    GizmoAxis m_DragAxis{GizmoAxis::None};
    Vector3   m_DragDelta{};       ///< Accumulated drag displacement.

    Matrix4x4 m_ViewProj{Matrix4x4::Identity()};
    float     m_VpX{0.f}, m_VpY{0.f}, m_VpW{1280.f}, m_VpH{720.f};

    static constexpr float kAxisLength    = 2.0f;   ///< Visual axis length in world units.
    static constexpr float kHitRadiusPx   = 12.0f;  ///< Pixel proximity threshold for axis hit-test.

    /// @brief Project a world-space point to window-space (top-left origin).
    [[nodiscard]] Vector2 ProjectToScreen(const Vector3& worldPos) const noexcept;

    /// @brief Minimum distance from a point to a line segment (2D, in pixels).
    [[nodiscard]] static float PointToSegmentDist(const Vector2& p,
                                                   const Vector2& a,
                                                   const Vector2& b) noexcept;

    /// @brief Determine which axis the mouse is closest to.
    [[nodiscard]] GizmoAxis HitTestAxis(const Vector2& mousePos) const noexcept;
};

} // namespace NF::Editor

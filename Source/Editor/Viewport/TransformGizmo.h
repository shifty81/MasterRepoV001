#pragma once
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

    /// @brief Update gizmo interaction state.
    /// @param dt       Delta time in seconds.
    /// @param mousePos Current mouse position in viewport space.
    void Update(float dt, const Vector2& mousePos);

    /// @brief Draw the gizmo into the viewport.
    /// @param device Active render device.
    void Draw(RenderDevice& device);

    /// @brief Returns true while the user is dragging a gizmo handle.
    [[nodiscard]] bool IsActive() const noexcept { return m_Active; }

    /// @brief Return the accumulated translation delta since the drag started.
    [[nodiscard]] const Vector3& GetDragDelta() const noexcept { return m_DragDelta; }

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

    static constexpr float kAxisLength = 2.0f;  ///< Visual axis length in world units.
    static constexpr float kHitRadius  = 0.25f; ///< Proximity threshold for axis hit-test.

    /// @brief Determine which axis the mouse is closest to.
    [[nodiscard]] GizmoAxis HitTestAxis(const Vector2& mousePos) const noexcept;
};

} // namespace NF::Editor

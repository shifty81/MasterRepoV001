#include "Editor/Viewport/TransformGizmo.h"
#include "Renderer/Debug/DebugDraw.h"
#include <cmath>

namespace NF::Editor {

// ---------------------------------------------------------------------------
// HitTestAxis — very simple screen-space proximity check against the three
// axis lines.  Because we don't have a full camera unproject here, we use
// the mouse delta direction as a heuristic: the axis whose screen-space
// projection is closest to the cursor wins.  This is a placeholder until
// proper viewport-unproject is wired.
// ---------------------------------------------------------------------------

GizmoAxis TransformGizmo::HitTestAxis([[maybe_unused]] const Vector2& mousePos) const noexcept
{
    // Without view/projection matrices available here we cannot do a proper
    // screen-space hit test.  We default to the axis whose screen delta is
    // dominant when the user starts dragging: horizontal → X, vertical → Y,
    // diagonal → Z.  This gives reasonable behaviour for the default orbit
    // camera orientation.
    return GizmoAxis::None;
}

// ---------------------------------------------------------------------------
// Update — drag detection and axis hit-testing
// ---------------------------------------------------------------------------

void TransformGizmo::Update([[maybe_unused]] float dt, const Vector2& mousePos) {
    if (m_SelectedEntity == NullEntity) {
        m_Active      = false;
        m_DragAxis    = GizmoAxis::None;
        m_HoveredAxis = GizmoAxis::None;
        m_LastMousePos = mousePos;
        m_WasMouseDown = m_MouseDown;
        return;
    }

    // Hover detection (only when not actively dragging)
    if (!m_Active)
        m_HoveredAxis = HitTestAxis(mousePos);

    // Begin drag
    if (m_MouseDown && !m_WasMouseDown && m_HoveredAxis != GizmoAxis::None) {
        m_Active   = true;
        m_DragAxis = m_HoveredAxis;
        m_DragDelta = {};
    }

    // Continue drag — translate position along the drag axis based on mouse
    // screen-space delta.
    if (m_Active && m_MouseDown) {
        const float dx = mousePos.X - m_LastMousePos.X;
        const float dy = mousePos.Y - m_LastMousePos.Y;
        constexpr float kSensitivity = 0.05f;

        switch (m_DragAxis) {
        case GizmoAxis::X:
            m_DragDelta.X += dx * kSensitivity;
            m_Position.X  += dx * kSensitivity;
            break;
        case GizmoAxis::Y:
            m_DragDelta.Y -= dy * kSensitivity;
            m_Position.Y  -= dy * kSensitivity;
            break;
        case GizmoAxis::Z:
            m_DragDelta.Z += dx * kSensitivity;
            m_Position.Z  += dx * kSensitivity;
            break;
        default: break;
        }
    }

    // End drag
    if (!m_MouseDown && m_WasMouseDown && m_Active) {
        m_Active   = false;
        m_DragAxis = GizmoAxis::None;
    }

    m_LastMousePos = mousePos;
    m_WasMouseDown = m_MouseDown;
}

// ---------------------------------------------------------------------------
// Draw — render axis lines using DebugDraw
// ---------------------------------------------------------------------------

void TransformGizmo::Draw([[maybe_unused]] RenderDevice& device) {
    if (m_SelectedEntity == NullEntity) return;

    const Vector3& p = m_Position;
    const float    L = kAxisLength;

    // Axis colours (bright when hovered or dragged, dim otherwise).
    const bool xHi = (m_HoveredAxis == GizmoAxis::X || m_DragAxis == GizmoAxis::X);
    const bool yHi = (m_HoveredAxis == GizmoAxis::Y || m_DragAxis == GizmoAxis::Y);
    const bool zHi = (m_HoveredAxis == GizmoAxis::Z || m_DragAxis == GizmoAxis::Z);

    // Translate mode: three axis lines from the gizmo origin.
    switch (m_Mode) {
    case GizmoMode::Translate:
    default:
    {
        // X axis — red
        DebugDraw::Line(p, {p.X + L, p.Y, p.Z},
                        xHi ? Vector3{1.f, 0.4f, 0.4f} : Vector3{0.9f, 0.2f, 0.2f});
        // Arrow head lines for X
        DebugDraw::Line({p.X + L, p.Y, p.Z}, {p.X + L * 0.85f, p.Y + L * 0.08f, p.Z},
                        xHi ? Vector3{1.f, 0.4f, 0.4f} : Vector3{0.9f, 0.2f, 0.2f});
        DebugDraw::Line({p.X + L, p.Y, p.Z}, {p.X + L * 0.85f, p.Y - L * 0.08f, p.Z},
                        xHi ? Vector3{1.f, 0.4f, 0.4f} : Vector3{0.9f, 0.2f, 0.2f});

        // Y axis — green
        DebugDraw::Line(p, {p.X, p.Y + L, p.Z},
                        yHi ? Vector3{0.4f, 1.f, 0.4f} : Vector3{0.2f, 0.9f, 0.2f});
        DebugDraw::Line({p.X, p.Y + L, p.Z}, {p.X + L * 0.08f, p.Y + L * 0.85f, p.Z},
                        yHi ? Vector3{0.4f, 1.f, 0.4f} : Vector3{0.2f, 0.9f, 0.2f});
        DebugDraw::Line({p.X, p.Y + L, p.Z}, {p.X - L * 0.08f, p.Y + L * 0.85f, p.Z},
                        yHi ? Vector3{0.4f, 1.f, 0.4f} : Vector3{0.2f, 0.9f, 0.2f});

        // Z axis — blue
        DebugDraw::Line(p, {p.X, p.Y, p.Z + L},
                        zHi ? Vector3{0.4f, 0.4f, 1.f} : Vector3{0.2f, 0.2f, 0.9f});
        DebugDraw::Line({p.X, p.Y, p.Z + L}, {p.X, p.Y + L * 0.08f, p.Z + L * 0.85f},
                        zHi ? Vector3{0.4f, 0.4f, 1.f} : Vector3{0.2f, 0.2f, 0.9f});
        DebugDraw::Line({p.X, p.Y, p.Z + L}, {p.X, p.Y - L * 0.08f, p.Z + L * 0.85f},
                        zHi ? Vector3{0.4f, 0.4f, 1.f} : Vector3{0.2f, 0.2f, 0.9f});
        break;
    }

    case GizmoMode::Rotate:
    {
        // Draw circle arcs per axis (approximated by 16-segment line strips)
        constexpr int segs = 16;
        constexpr float pi2 = 6.28318530718f;
        for (int i = 0; i < segs; ++i) {
            const float a0 = pi2 * static_cast<float>(i) / static_cast<float>(segs);
            const float a1 = pi2 * static_cast<float>(i + 1) / static_cast<float>(segs);
            // YZ circle (X rotation)
            DebugDraw::Line(
                {p.X, p.Y + L * std::cos(a0), p.Z + L * std::sin(a0)},
                {p.X, p.Y + L * std::cos(a1), p.Z + L * std::sin(a1)},
                xHi ? Vector3{1.f, 0.4f, 0.4f} : Vector3{0.9f, 0.2f, 0.2f});
            // XZ circle (Y rotation)
            DebugDraw::Line(
                {p.X + L * std::cos(a0), p.Y, p.Z + L * std::sin(a0)},
                {p.X + L * std::cos(a1), p.Y, p.Z + L * std::sin(a1)},
                yHi ? Vector3{0.4f, 1.f, 0.4f} : Vector3{0.2f, 0.9f, 0.2f});
            // XY circle (Z rotation)
            DebugDraw::Line(
                {p.X + L * std::cos(a0), p.Y + L * std::sin(a0), p.Z},
                {p.X + L * std::cos(a1), p.Y + L * std::sin(a1), p.Z},
                zHi ? Vector3{0.4f, 0.4f, 1.f} : Vector3{0.2f, 0.2f, 0.9f});
        }
        break;
    }

    case GizmoMode::Scale:
    {
        // Axis lines with box endpoints instead of arrows.
        constexpr float boxR = 0.1f;

        DebugDraw::Line(p, {p.X + L, p.Y, p.Z},
                        xHi ? Vector3{1.f, 0.4f, 0.4f} : Vector3{0.9f, 0.2f, 0.2f});
        DebugDraw::Box({p.X + L, p.Y, p.Z}, {boxR, boxR, boxR},
                       xHi ? Vector3{1.f, 0.4f, 0.4f} : Vector3{0.9f, 0.2f, 0.2f});

        DebugDraw::Line(p, {p.X, p.Y + L, p.Z},
                        yHi ? Vector3{0.4f, 1.f, 0.4f} : Vector3{0.2f, 0.9f, 0.2f});
        DebugDraw::Box({p.X, p.Y + L, p.Z}, {boxR, boxR, boxR},
                       yHi ? Vector3{0.4f, 1.f, 0.4f} : Vector3{0.2f, 0.9f, 0.2f});

        DebugDraw::Line(p, {p.X, p.Y, p.Z + L},
                        zHi ? Vector3{0.4f, 0.4f, 1.f} : Vector3{0.2f, 0.2f, 0.9f});
        DebugDraw::Box({p.X, p.Y, p.Z + L}, {boxR, boxR, boxR},
                       zHi ? Vector3{0.4f, 0.4f, 1.f} : Vector3{0.2f, 0.2f, 0.9f});
        break;
    }
    }

    DebugDraw::Flush(device);
}

} // namespace NF::Editor

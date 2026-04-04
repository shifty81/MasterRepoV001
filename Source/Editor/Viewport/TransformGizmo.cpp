#include "Editor/Viewport/TransformGizmo.h"
#include "Renderer/Debug/DebugDraw.h"
#include <cmath>
#include <algorithm>

namespace NF::Editor {

// ---------------------------------------------------------------------------
// ProjectToScreen — world → window-space (top-left origin, pixel coords)
// ---------------------------------------------------------------------------

Vector2 TransformGizmo::ProjectToScreen(const Vector3& worldPos) const noexcept
{
    Vector4 clip = m_ViewProj * Vector4(worldPos, 1.f);
    if (std::abs(clip.W) < 1e-6f)
        return {-10000.f, -10000.f}; // Behind the camera / degenerate

    // Perspective divide → NDC in [-1, 1]
    const float ndcX = clip.X / clip.W;
    const float ndcY = clip.Y / clip.W;

    // Map NDC to viewport window-space (top-left origin).
    const float sx = m_VpX + (ndcX * 0.5f + 0.5f) * m_VpW;
    const float sy = m_VpY + (1.f - (ndcY * 0.5f + 0.5f)) * m_VpH; // flip Y
    return {sx, sy};
}

// ---------------------------------------------------------------------------
// PointToSegmentDist — 2D point-to-line-segment minimum distance
// ---------------------------------------------------------------------------

float TransformGizmo::PointToSegmentDist(const Vector2& p,
                                          const Vector2& a,
                                          const Vector2& b) noexcept
{
    const Vector2 ab = b - a;
    const float lenSq = ab.LengthSq();
    if (lenSq < 1e-8f) return (p - a).Length(); // Degenerate segment

    // Parameter t of closest point on infinite line, clamped to [0,1].
    float t = (p - a).Dot(ab) / lenSq;
    t = std::clamp(t, 0.f, 1.f);

    const Vector2 closest = a + ab * t;
    return (p - closest).Length();
}

// ---------------------------------------------------------------------------
// HitTestAxis — screen-space proximity check against the three axis lines
// ---------------------------------------------------------------------------

GizmoAxis TransformGizmo::HitTestAxis(const Vector2& mousePos) const noexcept
{
    const Vector2 origin = ProjectToScreen(m_Position);

    struct AxisCandidate { GizmoAxis axis; float dist; };
    AxisCandidate best{GizmoAxis::None, kHitRadiusPx};

    // X axis
    {
        const Vector2 tip = ProjectToScreen({m_Position.X + kAxisLength, m_Position.Y, m_Position.Z});
        const float d = PointToSegmentDist(mousePos, origin, tip);
        if (d < best.dist) { best = {GizmoAxis::X, d}; }
    }
    // Y axis
    {
        const Vector2 tip = ProjectToScreen({m_Position.X, m_Position.Y + kAxisLength, m_Position.Z});
        const float d = PointToSegmentDist(mousePos, origin, tip);
        if (d < best.dist) { best = {GizmoAxis::Y, d}; }
    }
    // Z axis
    {
        const Vector2 tip = ProjectToScreen({m_Position.X, m_Position.Y, m_Position.Z + kAxisLength});
        const float d = PointToSegmentDist(mousePos, origin, tip);
        if (d < best.dist) { best = {GizmoAxis::Z, d}; }
    }

    return best.axis;
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

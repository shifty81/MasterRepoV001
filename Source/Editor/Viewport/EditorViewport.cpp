#include "Editor/Viewport/EditorViewport.h"
#include "UI/Rendering/UIRenderer.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <string>

namespace NF::Editor {

// Camera sensitivity constants
static constexpr float kOrbitSensitivity = 0.005f; // radians per pixel
static constexpr float kPanSensitivity   = 0.003f; // world units per pixel per unit zoom
static constexpr float kZoomSensitivity  = 0.12f;  // fraction of current zoom per wheel tick
static constexpr float kMinZoom          = 0.2f;
static constexpr float kMaxZoom          = 500.f;
static constexpr float kMaxPitch         = 1.55f;  // ~89 degrees (avoid gimbal lock)
static constexpr float kFlySpeed         = 2.0f;   // world units per second per unit of zoom
static constexpr float kFlyShiftMult     = 3.0f;   // speed multiplier when Shift is held

// Win32 Virtual-Key codes needed for FPS fly movement (no <windows.h> required).
// These match the Win32 VK_* constants and are only meaningful on Windows;
// on other platforms the keysDown[] array stays all-false so the block is a no-op.
static constexpr uint8_t kVK_Shift = 0x10;
static constexpr uint8_t kVK_W     = 0x57;
static constexpr uint8_t kVK_A     = 0x41;
static constexpr uint8_t kVK_S     = 0x53;
static constexpr uint8_t kVK_D     = 0x44;
static constexpr uint8_t kVK_Q     = 0x51;
static constexpr uint8_t kVK_E     = 0x45;

void EditorViewport::Init(RenderDevice* device) {
    m_Device = device;
}

void EditorViewport::Resize(int width, int height) {
    m_Width  = width;
    m_Height = height;
}

void EditorViewport::Update(float dt) {
    if (!m_Input || m_BoundsW <= 0.f || m_BoundsH <= 0.f) return;

    if (!IsMouseInside()) return;

    const float dx = m_Input->mouseDeltaX;
    const float dy = m_Input->mouseDeltaY;

    // Pre-compute trig values shared by orbit, pan, and fly movement.
    const float cp = std::cos(m_Pitch);
    const float sp = std::sin(m_Pitch);
    const float cy = std::cos(m_Yaw);
    const float sy = std::sin(m_Yaw);

    // Camera basis vectors (world space).
    // forward: direction from eye toward target (unit length).
    const Vector3 forward{ -cp * sy, -sp, -cp * cy };
    // right: horizontal-only right direction.
    // When pitch is near ±90° cp approaches zero; fall back to the world-right
    // axis so the cross product never degenerates.
    Vector3 right;
    if (std::abs(cp) > 0.01f) {
        right = forward.Cross({0.f, 1.f, 0.f}).Normalized();
    } else {
        // Camera is looking nearly straight up or down — use a stable fallback.
        right = Vector3{ cy, 0.f, -sy };
    }
    // up: camera up (perpendicular to both forward and right).
    const Vector3 camUp = right.Cross(forward);

    // --- Look: right mouse drag ---
    // Rotates pitch/yaw in place; same as orbit but the target then moves
    // with WASD so the overall behaviour feels like a free-fly camera.
    if (m_Input->rightDown && (dx != 0.f || dy != 0.f)) {
        m_Yaw   += dx * kOrbitSensitivity;
        m_Pitch -= dy * kOrbitSensitivity; // invert Y for natural feel
        m_Pitch  = std::clamp(m_Pitch, -kMaxPitch, kMaxPitch);
    }

    // --- FPS-style WASD fly movement (only while RMB is held) ---
    // Translating m_Target by any vector moves the camera eye by the same
    // vector (zoom stays constant), giving a first-person fly feel.
    if (m_Input->rightDown) {
        const bool   shiftHeld = m_Input->keysDown[kVK_Shift];
        const float  speedMult = shiftHeld ? kFlyShiftMult : 1.0f;
        const float  flyDist   = kFlySpeed * m_Zoom * dt * speedMult;

        if (m_Input->keysDown[kVK_W]) m_Target += forward * flyDist;
        if (m_Input->keysDown[kVK_S]) m_Target -= forward * flyDist;
        if (m_Input->keysDown[kVK_D]) m_Target += right   * flyDist;
        if (m_Input->keysDown[kVK_A]) m_Target -= right   * flyDist;
        if (m_Input->keysDown[kVK_E]) m_Target += camUp   * flyDist;
        if (m_Input->keysDown[kVK_Q]) m_Target -= camUp   * flyDist;
    }

    // --- Pan: middle mouse drag ---
    if (m_Input->middleDown && (dx != 0.f || dy != 0.f)) {
        // Scale pan speed with zoom so distant scenes pan at a natural rate.
        const float panScale = kPanSensitivity * m_Zoom;
        m_Target = m_Target - right * (dx * panScale) + camUp * (dy * panScale);
    }

    // --- Zoom: mouse wheel ---
    if (m_Input->wheelDelta != 0.f) {
        m_Zoom *= 1.f - m_Input->wheelDelta * kZoomSensitivity;
        m_Zoom  = std::clamp(m_Zoom, kMinZoom, kMaxZoom);
    }
}

bool EditorViewport::IsMouseInside() const noexcept {
    if (!m_Input || m_BoundsW <= 0.f || m_BoundsH <= 0.f) return false;
    return m_Input->mouseX >= m_BoundsX && m_Input->mouseX < m_BoundsX + m_BoundsW &&
           m_Input->mouseY >= m_BoundsY && m_Input->mouseY < m_BoundsY + m_BoundsH;
}

void EditorViewport::Draw(float x, float y, float w, float h) {
    // Cache bounds so Update() can do mouse-in-viewport testing.
    m_BoundsX = x;
    m_BoundsY = y;
    m_BoundsW = w;
    m_BoundsH = h;

    if (!m_Renderer) return;

    const float dpi = m_Renderer->GetDpiScale();

    if (!m_SceneRendered) {
        // Fallback: draw placeholder grid when no 3D scene is being rendered.
        static constexpr uint32_t kViewportBg    = 0x1E1E1EFF;
        static constexpr uint32_t kGridLineColor = 0x333333FF;
        static constexpr uint32_t kLabelColor    = 0x606060FF;

        m_Renderer->DrawRect({x, y, w, h}, kViewportBg);

        const float gridSpacing = 40.f * dpi;
        for (float gx = x + gridSpacing; gx < x + w; gx += gridSpacing)
            m_Renderer->DrawRect({gx, y, 1.f, h}, kGridLineColor);
        for (float gy = y + gridSpacing; gy < y + h; gy += gridSpacing)
            m_Renderer->DrawRect({x, gy, w, 1.f}, kGridLineColor);

        m_Renderer->DrawText("3D Viewport",
                             x + w * 0.5f - 44.f * dpi,
                             y + h * 0.5f - 7.f  * dpi,
                             kLabelColor, 2.f);
    }

    // Camera info overlay (bottom-left corner of viewport).
    static constexpr uint32_t kCamInfoColor  = 0x808080FF;
    const float padX = 6.f * dpi;

    auto toDeg = [](float rad) -> int {
        return static_cast<int>(rad * (180.f / std::numbers::pi_v<float>));
    };
    const Vector3 eye = GetCameraEye();
    std::string camStr = "Yaw "   + std::to_string(toDeg(m_Yaw))
                       + "  Pitch " + std::to_string(toDeg(m_Pitch))
                       + "  Zoom "  + std::to_string(static_cast<int>(m_Zoom))
                       + "  Eye ("  + std::to_string(static_cast<int>(eye.X))
                       + ", "       + std::to_string(static_cast<int>(eye.Y))
                       + ", "       + std::to_string(static_cast<int>(eye.Z)) + ")";
    m_Renderer->DrawText(camStr,
                         x + padX,
                         y + h - 18.f * dpi,
                         kCamInfoColor, 1.5f);

    // Controls hint (bottom-right)
    static constexpr uint32_t kHintColor = 0x505050FF;
    m_Renderer->DrawText("RMB: Look+WASD/QE  Shift: Fast  MMB: Pan  Wheel: Zoom",
                         x + w - 420.f * dpi,
                         y + h - 18.f * dpi,
                         kHintColor, 1.5f);

    // ---- Selection highlight overlay (top-centre) --------------------------
    if (!m_HighlightState.highlightLabel.empty()) {
        static constexpr uint32_t kHighlightBg   = 0x1A3A5AE0;
        static constexpr uint32_t kHighlightText = 0x80C8FFFF;
        const float labelW = 260.f * dpi;
        const float labelH = 18.f  * dpi;
        const float lx = x + (w - labelW) * 0.5f;
        const float ly = y + 6.f * dpi;
        m_Renderer->DrawRect({lx - 4.f * dpi, ly, labelW + 8.f * dpi, labelH}, kHighlightBg);
        m_Renderer->DrawText(m_HighlightState.highlightLabel,
                             lx, ly + 2.f * dpi, kHighlightText, 1.5f);
    }
}

bool EditorViewport::PickRay(float mouseX, float mouseY,
                              Vector3& outOrigin, Vector3& outDirection) const noexcept {
    if (m_BoundsW <= 0.f || m_BoundsH <= 0.f) return false;

    // Check if mouse is in viewport bounds.
    if (mouseX < m_BoundsX || mouseX >= m_BoundsX + m_BoundsW ||
        mouseY < m_BoundsY || mouseY >= m_BoundsY + m_BoundsH)
        return false;

    // Convert to viewport-local coordinates [0, width) and [0, height).
    const float localX = mouseX - m_BoundsX;
    const float localY = mouseY - m_BoundsY;

    // Convert to NDC [-1, 1].  Y is flipped because UI coordinates have
    // Y=0 at the top of the window while NDC has Y=+1 at the top.
    const float ndcX = (localX / m_BoundsW) * 2.f - 1.f;
    const float ndcY = 1.f - (localY / m_BoundsH) * 2.f;

    // Get inverse projection to convert from clip space to view space.
    Matrix4x4 proj = GetProjectionMatrix();
    // For a standard perspective projection:
    //   clip.x = proj[0][0] * view.x
    //   clip.y = proj[1][1] * view.y
    //   clip.z/w encodes depth
    // So view-space ray direction at NDC (ndcX, ndcY, -1) is:
    const float viewX = ndcX / proj.M[0][0];
    const float viewY = ndcY / proj.M[1][1];
    Vector3 viewDir = Vector3{viewX, viewY, -1.f}.Normalized();

    // Build camera basis from orbit parameters.
    const float cp = std::cos(m_Pitch), sp = std::sin(m_Pitch);
    const float cy = std::cos(m_Yaw),  sy = std::sin(m_Yaw);

    Vector3 eye{
        m_Target.X + m_Zoom * cp * sy,
        m_Target.Y + m_Zoom * sp,
        m_Target.Z + m_Zoom * cp * cy
    };
    Vector3 forward = (m_Target - eye).Normalized();
    Vector3 right   = forward.Cross({0.f, 1.f, 0.f}).Normalized();
    Vector3 up      = right.Cross(forward);

    // Transform view-space direction to world space.
    outDirection = (right * viewDir.X + up * viewDir.Y - forward * viewDir.Z).Normalized();
    outOrigin    = eye;
    return true;
}

Matrix4x4 EditorViewport::GetViewMatrix() const noexcept {
    // Build simple orbit look-at from pitch/yaw/zoom.
    const float cp = std::cos(m_Pitch), sp = std::sin(m_Pitch);
    const float cy = std::cos(m_Yaw),  sy = std::sin(m_Yaw);
    Vector3 eye{
        m_Target.X + m_Zoom * cp * sy,
        m_Target.Y + m_Zoom * sp,
        m_Target.Z + m_Zoom * cp * cy
    };
    Vector3 forward = (m_Target - eye).Normalized();
    Vector3 right   = forward.Cross({0.f, 1.f, 0.f}).Normalized();
    Vector3 up      = right.Cross(forward);

    Matrix4x4 v = Matrix4x4::Identity();
    v.M[0][0] = right.X;    v.M[1][0] = right.Y;    v.M[2][0] = right.Z;
    v.M[0][1] = up.X;       v.M[1][1] = up.Y;       v.M[2][1] = up.Z;
    v.M[0][2] = -forward.X; v.M[1][2] = -forward.Y; v.M[2][2] = -forward.Z;
    v.M[3][0] = -right.Dot(eye);
    v.M[3][1] = -up.Dot(eye);
    v.M[3][2] =  forward.Dot(eye);
    return v;
}

Vector3 EditorViewport::GetCameraEye() const noexcept {
    const float cp = std::cos(m_Pitch), sp = std::sin(m_Pitch);
    const float cy = std::cos(m_Yaw),  sy = std::sin(m_Yaw);
    return {
        m_Target.X + m_Zoom * cp * sy,
        m_Target.Y + m_Zoom * sp,
        m_Target.Z + m_Zoom * cp * cy
    };
}

Matrix4x4 EditorViewport::GetProjectionMatrix() const noexcept {
    // Use viewport panel bounds for aspect ratio when available,
    // otherwise fall back to m_Width/m_Height.
    const float vpW = (m_BoundsW > 0.f) ? m_BoundsW : static_cast<float>(m_Width);
    const float vpH = (m_BoundsH > 0.f) ? m_BoundsH : static_cast<float>(m_Height);
    const float aspect = (vpH > 0.f) ? vpW / vpH : 1.f;
    const float fovY  = 0.7854f; // ~45 degrees in radians
    const float nearZ = 0.1f, farZ = 1000.f;
    const float f     = 1.f / std::tan(fovY * 0.5f);

    Matrix4x4 p{};
    p.M[0][0] = f / aspect;
    p.M[1][1] = f;
    p.M[2][2] = (farZ + nearZ) / (nearZ - farZ);
    p.M[2][3] = -1.f;
    p.M[3][2] = (2.f * farZ * nearZ) / (nearZ - farZ);
    return p;
}

} // namespace NF::Editor

#pragma once
#include "Core/Math/Vector.h"
#include "Core/Math/Matrix.h"
#include <vector>

namespace NF {

class RenderDevice;

/// @brief Immediate-mode debug visualisation helpers.
///
/// All methods accumulate geometry into an internal buffer; call Flush() once
/// per frame to submit them to the GPU.
class DebugDraw {
public:
    /// @brief Enqueue a line segment.
    static void Line(const Vector3& start, const Vector3& end, const Vector3& color);

    /// @brief Enqueue a wireframe axis-aligned box.
    static void Box(const Vector3& center, const Vector3& extents, const Vector3& color);

    /// @brief Enqueue a wireframe sphere approximated by latitude/longitude arcs.
    static void Sphere(const Vector3& center, float radius, const Vector3& color);

    /// @brief Submit all accumulated geometry to the GPU and clear the buffer.
    /// @param device   Active render device.
    /// @param viewProj Combined view-projection matrix (proj * view).
    static void Flush(RenderDevice& device, const Matrix4x4& viewProj);

private:
    struct LineSegment {
        Vector3 Start{};
        Vector3 End{};
        Vector3 Color{};
    };

    static std::vector<LineSegment>& GetBuffer();
};

} // namespace NF

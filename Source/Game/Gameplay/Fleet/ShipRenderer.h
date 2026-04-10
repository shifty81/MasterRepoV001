#pragma once
#include "Renderer/Mesh/MeshRenderer.h"
#include "Renderer/RHI/Shader.h"
#include "Core/Math/Vector.h"
#include "Core/Math/Matrix.h"

namespace NF {
class ForwardRenderer;
class RenderDevice;
}

namespace NF::Game::Gameplay {

// ---------------------------------------------------------------------------
// ShipRenderer
//
// Owns the GPU mesh and ship shader for a low-poly spacecraft.
// Call Init() once after the render device is ready, then call Render() each
// frame inside a ForwardRenderer scene.
//
// The ship is rendered at the given world-space centre position, rotated to
// the given yaw (Y-axis rotation in radians).
// ---------------------------------------------------------------------------
class ShipRenderer
{
public:
    /// @brief Compile the ship shader and upload the mesh to the GPU.
    ///        renderDevice is non-owning.
    bool Init(RenderDevice* renderDevice);

    /// @brief Release all GPU resources.
    void Shutdown();

    /// @brief Render the ship centred at @p worldPosition, rotated by @p yawRadians.
    ///        Must be called between ForwardRenderer::BeginScene() and EndScene().
    void Render(ForwardRenderer& renderer,
                const NF::Vector3& worldPosition,
                float yawRadians);

    [[nodiscard]] bool IsReady() const noexcept { return m_Ready; }

private:
    static Matrix4x4 BuildTransform(const NF::Vector3& pos, float yaw);

    Mesh    m_Mesh;
    Shader  m_Shader;
    bool    m_Ready{false};
};

} // namespace NF::Game::Gameplay

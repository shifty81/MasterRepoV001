#include "Renderer/Pipeline/ForwardRenderer.h"
#include "Renderer/RHI/RenderDevice.h"
#include "Renderer/RHI/Shader.h"
#include "Renderer/Mesh/MeshRenderer.h"
#include "Renderer/Materials/Material.h"
#include "Core/Logging/Log.h"
#include <vector>

#ifdef NF_HAS_OPENGL
#include "Renderer/RHI/GLHeaders.h"
#endif

namespace NF {

void ForwardRenderer::Init(RenderDevice* device) {
    m_Device = device;
    NF_LOG_INFO("Renderer", "ForwardRenderer initialised");
}

void ForwardRenderer::Shutdown() {
    m_Queue.clear();
    m_Device = nullptr;
    NF_LOG_INFO("Renderer", "ForwardRenderer shut down");
}

void ForwardRenderer::BeginScene(const Matrix4x4& viewMatrix, const Matrix4x4& projMatrix) {
    m_View = viewMatrix;
    m_Proj = projMatrix;
    m_Queue.clear();
}

void ForwardRenderer::Submit(Mesh& mesh, Material& material, const Matrix4x4& transform) {
    m_Queue.push_back({&mesh, material, transform});
}

void ForwardRenderer::EndScene() {
    Flush();
}

void ForwardRenderer::Flush() {
#ifdef NF_HAS_OPENGL
    // Always establish the required 3-D rendering state before issuing draw
    // calls.  UIRenderer::Flush() leaves GL_DEPTH_TEST disabled (it does not
    // save/restore) so we cannot rely on whatever state was left by the
    // previous UI pass.  Explicitly enabling here guarantees correct depth
    // sorting regardless of frame ordering.
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);

    // Backface culling: all voxel faces use CCW winding when viewed from
    // outside, so GL_BACK removes invisible interior faces.  Disabled
    // afterwards so the 2-D UI pass is unaffected.
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // Transparency off for opaque 3-D geometry.
    glDisable(GL_BLEND);
#endif

    for (auto& cmd : m_Queue) {
        cmd.Mat.Bind();

        // Upload camera and model matrices to the active shader.
        // The material's Bind() activates its shader, so we can set
        // uniforms immediately after.
        Shader* shader = cmd.Mat.GetShader();
        if (shader) {
            shader->SetMat4("uView",       m_View);
            shader->SetMat4("uProjection", m_Proj);
            shader->SetMat4("uModel",      cmd.Transform);
        }

        cmd.MeshPtr->Bind();
        cmd.MeshPtr->Draw();
    }
    m_Queue.clear();

#ifdef NF_HAS_OPENGL
    glDisable(GL_CULL_FACE);
    // Leave depth test enabled so DebugDraw (which runs after Flush) also
    // benefits from correct depth.  UIRenderer::EndFrame saves/restores
    // state, so the UI pass will temporarily disable it as needed.
#endif
}

} // namespace NF

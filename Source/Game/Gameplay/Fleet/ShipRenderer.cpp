#include "Game/Gameplay/Fleet/ShipRenderer.h"
#include "Game/Gameplay/Fleet/ShipMeshBuilder.h"
#include "Renderer/Pipeline/ForwardRenderer.h"
#include "Renderer/Materials/Material.h"
#include "Core/Logging/Log.h"
#include <cmath>

namespace NF::Game::Gameplay {

// ---------------------------------------------------------------------------
// Ship GLSL shaders
// Palette-based Phong lighting; TexCoord.X = ShipColor palette index (0–5).
// Mirrors the character shader pattern exactly.
// ---------------------------------------------------------------------------

static const char* kShipVertSrc = R"(
#version 330 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

uniform mat4 uView;
uniform mat4 uProjection;
uniform mat4 uModel;

out vec3 vNormal;
out vec3 vWorldPos;
out float vColorId;

void main() {
    vec4 worldPos = uModel * vec4(aPosition, 1.0);
    gl_Position   = uProjection * uView * worldPos;
    vNormal       = mat3(uModel) * aNormal;
    vWorldPos     = worldPos.xyz;
    vColorId      = aTexCoord.x;
}
)";

static const char* kShipFragSrc = R"(
#version 330 core
in  vec3  vNormal;
in  vec3  vWorldPos;
in  float vColorId;
out vec4  FragColor;

uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uAmbientColor;
uniform vec3 uViewPos;

// 6-entry ship colour palette (matches ShipColor enum order):
//  0 Hull    1 Cockpit  2 Wing    3 Engine  4 Exhaust  5 Trim
vec3 shipPalette(int id) {
    if (id == 0) return vec3(0.28, 0.28, 0.30);  // Hull    — dark metallic grey
    if (id == 1) return vec3(0.10, 0.14, 0.22);  // Cockpit — deep blue-grey glass
    if (id == 2) return vec3(0.35, 0.35, 0.37);  // Wing    — medium grey panels
    if (id == 3) return vec3(0.85, 0.35, 0.05);  // Engine  — orange-red nozzle
    if (id == 4) return vec3(1.00, 0.85, 0.40);  // Exhaust — bright yellow-white core
    if (id == 5) return vec3(0.70, 0.72, 0.75);  // Trim    — white-grey accent stripe
    return vec3(1.0, 0.0, 1.0);                   // Unknown — magenta debug
}

void main() {
    vec3  baseColor = shipPalette(int(vColorId + 0.5));
    vec3  N         = normalize(vNormal);
    vec3  L         = normalize(-uLightDir);
    float diff      = max(dot(N, L), 0.0);

    // Metallic specular highlight
    vec3  V         = normalize(uViewPos - vWorldPos);
    vec3  H         = normalize(L + V);
    float spec      = pow(max(dot(N, H), 0.0), 64.0) * 0.35;

    // Engine glow (id==3 or id==4) emits additional self-illumination
    float emit = 0.0;
    int   cid  = int(vColorId + 0.5);
    if (cid == 3) emit = 0.4;
    if (cid == 4) emit = 0.9;

    vec3  ambient   = uAmbientColor * baseColor;
    vec3  diffuse   = uLightColor   * baseColor * diff;
    vec3  specular  = uLightColor   * spec;
    vec3  emissive  = baseColor * emit;

    FragColor = vec4(ambient + diffuse + specular + emissive, 1.0);
}
)";

// ---------------------------------------------------------------------------
// Init
// ---------------------------------------------------------------------------

bool ShipRenderer::Init(RenderDevice* /*renderDevice*/)
{
    if (!m_Shader.LoadFromSource(kShipVertSrc, kShipFragSrc))
    {
        NF_LOG_ERROR("Ship", "ShipRenderer: shader compile failed");
        return false;
    }

    ShipMeshBuilder builder;
    MeshData meshData = builder.BuildFullShip();
    m_Mesh.Upload(meshData);

    NF_LOG_INFO("Ship",
                "ShipRenderer: ready ("
                + std::to_string(meshData.Vertices.size()) + " verts, "
                + std::to_string(meshData.Indices.size() / 3) + " tris)");

    m_Ready = true;
    return true;
}

// ---------------------------------------------------------------------------
// Shutdown
// ---------------------------------------------------------------------------

void ShipRenderer::Shutdown()
{
    m_Ready = false;
}

// ---------------------------------------------------------------------------
// Render
// ---------------------------------------------------------------------------

void ShipRenderer::Render(ForwardRenderer& renderer,
                           const NF::Vector3& worldPosition,
                           float yawRadians)
{
    if (!m_Ready)
        return;

    m_Shader.Bind();
    m_Shader.SetVec3("uLightDir",     { 0.5f, -1.0f,  0.3f});
    m_Shader.SetVec3("uLightColor",   { 1.0f,  0.97f, 0.90f});
    m_Shader.SetVec3("uAmbientColor", { 0.18f, 0.20f, 0.25f});
    m_Shader.SetVec3("uViewPos",      worldPosition);

    Material mat;
    mat.SetShader(&m_Shader);

    const Matrix4x4 transform = BuildTransform(worldPosition, yawRadians);
    renderer.Submit(m_Mesh, mat, transform);
}

// ---------------------------------------------------------------------------
// BuildTransform
// ---------------------------------------------------------------------------

Matrix4x4 ShipRenderer::BuildTransform(const NF::Vector3& pos, float yaw)
{
    Matrix4x4 T = Matrix4x4::Identity();
    T.M[3][0] = pos.X;
    T.M[3][1] = pos.Y;
    T.M[3][2] = pos.Z;

    const float s = std::sin(yaw);
    const float c = std::cos(yaw);
    Matrix4x4 R = Matrix4x4::Identity();
    R.M[0][0] =  c;  R.M[2][0] = s;
    R.M[0][2] = -s;  R.M[2][2] = c;

    return T.Multiply(R);
}

} // namespace NF::Game::Gameplay

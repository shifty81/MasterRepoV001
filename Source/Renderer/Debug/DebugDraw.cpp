#include "Renderer/Debug/DebugDraw.h"
#include "Renderer/RHI/RenderDevice.h"
#include <cmath>
#include <numbers>
#include <vector>

#ifdef NF_HAS_OPENGL
#include "Renderer/RHI/GLHeaders.h"
#endif

namespace NF {

// ---------------------------------------------------------------------------
// Line buffer
// ---------------------------------------------------------------------------

std::vector<DebugDraw::LineSegment>& DebugDraw::GetBuffer() {
    static std::vector<LineSegment> s_Lines;
    return s_Lines;
}

// ---------------------------------------------------------------------------
// Geometry accumulation
// ---------------------------------------------------------------------------

void DebugDraw::Line(const Vector3& start, const Vector3& end, const Vector3& color) {
    GetBuffer().push_back({start, end, color});
}

void DebugDraw::Box(const Vector3& center, const Vector3& extents, const Vector3& color) {
    const float ex = extents.X, ey = extents.Y, ez = extents.Z;
    Vector3 c[8] = {
        {center.X - ex, center.Y - ey, center.Z - ez},
        {center.X + ex, center.Y - ey, center.Z - ez},
        {center.X + ex, center.Y + ey, center.Z - ez},
        {center.X - ex, center.Y + ey, center.Z - ez},
        {center.X - ex, center.Y - ey, center.Z + ez},
        {center.X + ex, center.Y - ey, center.Z + ez},
        {center.X + ex, center.Y + ey, center.Z + ez},
        {center.X - ex, center.Y + ey, center.Z + ez},
    };
    // Bottom face
    Line(c[0], c[1], color); Line(c[1], c[2], color);
    Line(c[2], c[3], color); Line(c[3], c[0], color);
    // Top face
    Line(c[4], c[5], color); Line(c[5], c[6], color);
    Line(c[6], c[7], color); Line(c[7], c[4], color);
    // Vertical edges
    Line(c[0], c[4], color); Line(c[1], c[5], color);
    Line(c[2], c[6], color); Line(c[3], c[7], color);
}

void DebugDraw::Sphere(const Vector3& center, float radius, const Vector3& color) {
    constexpr int   kSegments = 16;
    constexpr float kStep     = 2.f * std::numbers::pi_v<float> / kSegments;

    for (int i = 0; i < kSegments; ++i) {
        float a0 = i * kStep, a1 = (i + 1) * kStep;
        // XY plane
        Line({center.X + radius * std::cos(a0), center.Y + radius * std::sin(a0), center.Z},
             {center.X + radius * std::cos(a1), center.Y + radius * std::sin(a1), center.Z},
             color);
        // XZ plane
        Line({center.X + radius * std::cos(a0), center.Y, center.Z + radius * std::sin(a0)},
             {center.X + radius * std::cos(a1), center.Y, center.Z + radius * std::sin(a1)},
             color);
        // YZ plane
        Line({center.X, center.Y + radius * std::cos(a0), center.Z + radius * std::sin(a0)},
             {center.X, center.Y + radius * std::cos(a1), center.Z + radius * std::sin(a1)},
             color);
    }
}

// ---------------------------------------------------------------------------
// GPU line rendering
// ---------------------------------------------------------------------------

#ifdef NF_HAS_OPENGL
namespace {

struct LineGpuState {
    GLuint vao{0};
    GLuint vbo{0};
    GLuint prog{0};
    GLint  vpLoc{-1};
    bool   ready{false};
};

static LineGpuState& LineState() {
    static LineGpuState s;
    return s;
}

static GLuint CompileStage(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    return s;
}

static void EnsureLineGpu() {
    LineGpuState& ls = LineState();
    if (ls.ready) return;

    static const char* kVert = R"GLSL(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aCol;
uniform mat4 uVP;
out vec3 vCol;
void main() {
    gl_Position = uVP * vec4(aPos, 1.0);
    vCol = aCol;
}
)GLSL";

    static const char* kFrag = R"GLSL(
#version 330 core
in  vec3 vCol;
out vec4 FragColor;
void main() { FragColor = vec4(vCol, 1.0); }
)GLSL";

    GLuint vert = CompileStage(GL_VERTEX_SHADER,   kVert);
    GLuint frag = CompileStage(GL_FRAGMENT_SHADER, kFrag);
    ls.prog = glCreateProgram();
    glAttachShader(ls.prog, vert);
    glAttachShader(ls.prog, frag);
    glLinkProgram(ls.prog);
    glDeleteShader(vert);
    glDeleteShader(frag);
    ls.vpLoc = glGetUniformLocation(ls.prog, "uVP");

    glGenVertexArrays(1, &ls.vao);
    glGenBuffers(1, &ls.vbo);
    glBindVertexArray(ls.vao);
    glBindBuffer(GL_ARRAY_BUFFER, ls.vbo);
    // Vertex layout: position (3 floats) then color (3 floats), stride = 24 bytes.
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          reinterpret_cast<const void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
                          reinterpret_cast<const void*>(3 * sizeof(float)));
    glBindVertexArray(0);
    ls.ready = true;
}

} // anonymous namespace
#endif // NF_HAS_OPENGL

void DebugDraw::Flush(RenderDevice& device, const Matrix4x4& viewProj) {
    auto& buf = GetBuffer();

#ifdef NF_HAS_OPENGL
    if (!buf.empty()) {
        EnsureLineGpu();
        LineGpuState& ls = LineState();
        if (ls.ready) {
            // Pack: 2 vertices per segment, 6 floats per vertex (pos + color).
            std::vector<float> vd;
            vd.reserve(buf.size() * 12);
            for (const auto& seg : buf) {
                vd.push_back(seg.Start.X); vd.push_back(seg.Start.Y); vd.push_back(seg.Start.Z);
                vd.push_back(seg.Color.X); vd.push_back(seg.Color.Y); vd.push_back(seg.Color.Z);
                vd.push_back(seg.End.X);   vd.push_back(seg.End.Y);   vd.push_back(seg.End.Z);
                vd.push_back(seg.Color.X); vd.push_back(seg.Color.Y); vd.push_back(seg.Color.Z);
            }
            glBindVertexArray(ls.vao);
            glBindBuffer(GL_ARRAY_BUFFER, ls.vbo);
            glBufferData(GL_ARRAY_BUFFER,
                         static_cast<GLsizeiptr>(vd.size() * sizeof(float)),
                         vd.data(), GL_DYNAMIC_DRAW);
            glUseProgram(ls.prog);
            // Matrix4x4 uses column-major storage M[col][row], matching OpenGL
            // column-major convention — pass directly with GL_FALSE (no transpose).
            glUniformMatrix4fv(ls.vpLoc, 1, GL_FALSE, &viewProj.M[0][0]);
            glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(buf.size() * 2));
            glUseProgram(0);
            glBindVertexArray(0);
        }
    }
#else
    (void)device;
    (void)viewProj;
#endif

    buf.clear();
}

} // namespace NF

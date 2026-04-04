#include "UI/Rendering/UIRenderer.h"
#include "UI/Rendering/BuiltinFont.h"
#include "Core/Logging/Log.h"

#ifdef NF_HAS_OPENGL
#include "Renderer/RHI/GLHeaders.h"
#endif

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

namespace NF {

// ---------------------------------------------------------------------------
// GLSL shaders — coloured-vertex pipeline (rects)
// ---------------------------------------------------------------------------

#ifdef NF_HAS_OPENGL
static const char* kUIVertSrc = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec4 aColor;
uniform mat4 uProjection;
out vec4 vColor;
void main() {
    gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
    vColor = aColor;
}
)";

static const char* kUIFragSrc = R"(
#version 330 core
in vec4 vColor;
out vec4 FragColor;
void main() {
    FragColor = vColor;
}
)";

// ---------------------------------------------------------------------------
// GLSL shaders — textured pipeline (font glyphs)
// ---------------------------------------------------------------------------

static const char* kTextVertSrc = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec4 aColor;
uniform mat4 uProjection;
out vec2 vTexCoord;
out vec4 vColor;
void main() {
    gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
    vTexCoord   = aTexCoord;
    vColor      = aColor;
}
)";

static const char* kTextFragSrc = R"(
#version 330 core
in vec2 vTexCoord;
in vec4 vColor;
out vec4 FragColor;
uniform sampler2D uFontAtlas;
void main() {
    float alpha = texture(uFontAtlas, vTexCoord).r;
    if (alpha < 0.5) discard;
    FragColor = vec4(vColor.rgb, vColor.a * alpha);
}
)";
#endif

// ---------------------------------------------------------------------------
// Helper — compile + link a shader program
// ---------------------------------------------------------------------------

#ifdef NF_HAS_OPENGL
static GLuint CompileProgram(const char* vertSrc, const char* fragSrc) {
    auto compileStage = [](GLenum type, const char* src) -> GLuint {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint ok = 0;
        glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char buf[512];
            glGetShaderInfoLog(s, sizeof(buf), nullptr, buf);
            NF_LOG_ERROR("UIRenderer", std::string("Shader compile error: ") + buf);
        }
        return s;
    };

    GLuint vert = compileStage(GL_VERTEX_SHADER,   vertSrc);
    GLuint frag = compileStage(GL_FRAGMENT_SHADER, fragSrc);

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert);
    glAttachShader(prog, frag);
    glLinkProgram(prog);
    glDeleteShader(vert);
    glDeleteShader(frag);

    GLint linked = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &linked);
    if (!linked) {
        char buf[512];
        glGetProgramInfoLog(prog, sizeof(buf), nullptr, buf);
        NF_LOG_ERROR("UIRenderer", std::string("Shader link error: ") + buf);
        glDeleteProgram(prog);
        return 0;
    }
    return prog;
}
#endif

// ---------------------------------------------------------------------------
// Init / Shutdown
// ---------------------------------------------------------------------------

bool UIRenderer::Init() {
#ifdef NF_HAS_OPENGL
    // ---- 1. Rect shader + VAO/VBO ----
    {
        GLuint prog = CompileProgram(kUIVertSrc, kUIFragSrc);
        if (!prog) return false;
        m_ShaderProgram = static_cast<uint32_t>(prog);

        GLuint vao = 0, vbo = 0;
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(UIVertex),
                              reinterpret_cast<void*>(offsetof(UIVertex, X)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(UIVertex),
                              reinterpret_cast<void*>(offsetof(UIVertex, R)));
        glBindVertexArray(0);
        m_VAO = static_cast<uint32_t>(vao);
        m_VBO = static_cast<uint32_t>(vbo);
    }

    // ---- 2. Text shader + VAO/VBO ----
    {
        GLuint prog = CompileProgram(kTextVertSrc, kTextFragSrc);
        if (!prog) return false;
        m_TextShaderProgram = static_cast<uint32_t>(prog);

        GLuint vao = 0, vbo = 0;
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        // Position — location 0
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(TextVertex),
                              reinterpret_cast<void*>(offsetof(TextVertex, X)));
        // TexCoord — location 1
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(TextVertex),
                              reinterpret_cast<void*>(offsetof(TextVertex, U)));
        // Color — location 2
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(TextVertex),
                              reinterpret_cast<void*>(offsetof(TextVertex, R)));
        glBindVertexArray(0);
        m_TextVAO = static_cast<uint32_t>(vao);
        m_TextVBO = static_cast<uint32_t>(vbo);
    }

    // ---- 3. Bake font atlas into a GL texture ----
    {
        // Layout: 16 columns × 6 rows of kFontCellW × kFontCellH cells.
        constexpr int kCols = 16;
        constexpr int kRows = (kFontGlyphCount + kCols - 1) / kCols;
        const int atlasW = kCols * kFontCellW;
        const int atlasH = kRows * kFontCellH;

        // Expand 1-bit-per-pixel glyph data into an 8-bit alpha texture.
        std::vector<uint8_t> pixels(static_cast<size_t>(atlasW) * atlasH, 0);

        for (int g = 0; g < kFontGlyphCount; ++g) {
            const int col = g % kCols;
            const int row = g / kCols;
            const int ox  = col * kFontCellW;
            const int oy  = row * kFontCellH;

            for (int y = 0; y < kFontCellH; ++y) {
                const uint8_t bits = kFontData[g * kFontCellH + y];
                for (int x = 0; x < kFontCellW; ++x) {
                    if (bits & (0x80 >> x)) {
                        pixels[static_cast<size_t>(oy + y) * atlasW + (ox + x)] = 255;
                    }
                }
            }
        }

        GLuint tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, atlasW, atlasH, 0,
                     GL_RED, GL_UNSIGNED_BYTE, pixels.data());
        glBindTexture(GL_TEXTURE_2D, 0);

        m_FontTexture = static_cast<uint32_t>(tex);
        m_AtlasW      = atlasW;
        m_AtlasH      = atlasH;
    }

    NF_LOG_INFO("UIRenderer", "UIRenderer initialised (OpenGL, bitmap font atlas)");
#else
    NF_LOG_INFO("UIRenderer", "UIRenderer initialised (stub — no OpenGL)");
#endif

    m_Initialised = true;
    return true;
}

void UIRenderer::Shutdown() {
#ifdef NF_HAS_OPENGL
    if (m_VAO) { GLuint v = m_VAO; glDeleteVertexArrays(1, &v); m_VAO = 0; }
    if (m_VBO) { GLuint v = m_VBO; glDeleteBuffers(1, &v);       m_VBO = 0; }
    if (m_ShaderProgram) {
        glDeleteProgram(static_cast<GLuint>(m_ShaderProgram));
        m_ShaderProgram = 0;
    }
    if (m_TextVAO) { GLuint v = m_TextVAO; glDeleteVertexArrays(1, &v); m_TextVAO = 0; }
    if (m_TextVBO) { GLuint v = m_TextVBO; glDeleteBuffers(1, &v);       m_TextVBO = 0; }
    if (m_TextShaderProgram) {
        glDeleteProgram(static_cast<GLuint>(m_TextShaderProgram));
        m_TextShaderProgram = 0;
    }
    if (m_FontTexture) {
        GLuint t = m_FontTexture;
        glDeleteTextures(1, &t);
        m_FontTexture = 0;
    }
#endif
    m_Initialised = false;
    NF_LOG_INFO("UIRenderer", "UIRenderer shut down");
}

// ---------------------------------------------------------------------------
// Per-frame
// ---------------------------------------------------------------------------

void UIRenderer::SetViewportSize(float width, float height) {
    m_ViewportWidth  = width;
    m_ViewportHeight = height;
}

void UIRenderer::BeginFrame() {
    m_Vertices.clear();
    m_TextVertices.clear();
    m_ClipStack.clear();
}

Rect UIRenderer::IntersectRect(const Rect& a, const Rect& b) const noexcept {
    const float x0 = std::max(a.X, b.X);
    const float y0 = std::max(a.Y, b.Y);
    const float x1 = std::min(a.X + a.Width,  b.X + b.Width);
    const float y1 = std::min(a.Y + a.Height, b.Y + b.Height);
    return {x0, y0, std::max(0.f, x1 - x0), std::max(0.f, y1 - y0)};
}

Rect UIRenderer::CurrentClipRect() const noexcept {
    if (m_ClipStack.empty()) {
        return {0.f, 0.f, m_ViewportWidth, m_ViewportHeight};
    }
    return m_ClipStack.back();
}

bool UIRenderer::ClipRectToCurrent(const Rect& input, Rect& output) const noexcept {
    output = input;
    if (!HasClipRect()) {
        return output.Width > 0.f && output.Height > 0.f;
    }
    output = IntersectRect(input, CurrentClipRect());
    return output.Width > 0.f && output.Height > 0.f;
}

void UIRenderer::PushClipRect(const Rect& rect) {
    Rect clipped = rect;
    if (HasClipRect()) {
        clipped = IntersectRect(CurrentClipRect(), rect);
    }
    if (clipped.Width < 0.f) clipped.Width = 0.f;
    if (clipped.Height < 0.f) clipped.Height = 0.f;
    m_ClipStack.push_back(clipped);
}

void UIRenderer::PopClipRect() {
    if (!m_ClipStack.empty()) {
        m_ClipStack.pop_back();
    }
}

void UIRenderer::DrawRect(const Rect& rect, uint32_t color) {
    Rect clipped{};
    if (!ClipRectToCurrent(rect, clipped)) {
        return;
    }

    const float r = static_cast<float>((color >> 24) & 0xFF) / 255.f;
    const float g = static_cast<float>((color >> 16) & 0xFF) / 255.f;
    const float b = static_cast<float>((color >>  8) & 0xFF) / 255.f;
    const float a = static_cast<float>((color      ) & 0xFF) / 255.f;

    const float x0 = clipped.X;
    const float y0 = clipped.Y;
    const float x1 = clipped.X + clipped.Width;
    const float y1 = clipped.Y + clipped.Height;

    m_Vertices.push_back({x0, y0, r, g, b, a});
    m_Vertices.push_back({x1, y0, r, g, b, a});
    m_Vertices.push_back({x1, y1, r, g, b, a});

    m_Vertices.push_back({x0, y0, r, g, b, a});
    m_Vertices.push_back({x1, y1, r, g, b, a});
    m_Vertices.push_back({x0, y1, r, g, b, a});
}

void UIRenderer::DrawOutlineRect(const Rect& rect, uint32_t color) {
    const float t = 1.0f;
    DrawRect({rect.X, rect.Y, rect.Width, t}, color);
    DrawRect({rect.X, rect.Y + rect.Height - t, rect.Width, t}, color);
    DrawRect({rect.X, rect.Y, t, rect.Height}, color);
    DrawRect({rect.X + rect.Width - t, rect.Y, t, rect.Height}, color);
}

// ---------------------------------------------------------------------------
// DrawText — bitmap-font atlas version
// ---------------------------------------------------------------------------

void UIRenderer::DrawText(std::string_view text, float x, float y,
                           uint32_t color, float scale) {
    if (text.empty()) return;

    const float effectiveScale = scale * m_DpiScale;
    const float charW = static_cast<float>(kFontCellW) * effectiveScale;
    const float charH = static_cast<float>(kFontCellH) * effectiveScale;

    // Early-out if entirely outside the clip region.
    if (HasClipRect()) {
        const Rect clip = CurrentClipRect();
        if (y > clip.Y + clip.Height || y + charH < clip.Y || x > clip.X + clip.Width) {
            return;
        }
    }

    const float r = static_cast<float>((color >> 24) & 0xFF) / 255.f;
    const float g = static_cast<float>((color >> 16) & 0xFF) / 255.f;
    const float b = static_cast<float>((color >>  8) & 0xFF) / 255.f;
    const float a = static_cast<float>((color      ) & 0xFF) / 255.f;

    constexpr int kCols = 16;
    const float invAtlasW = (m_AtlasW > 0) ? 1.f / static_cast<float>(m_AtlasW) : 0.f;
    const float invAtlasH = (m_AtlasH > 0) ? 1.f / static_cast<float>(m_AtlasH) : 0.f;

    float penX = x;
    for (const char ch : text) {
        // Map to atlas index; skip non-printable.
        int idx = static_cast<int>(static_cast<unsigned char>(ch)) - kFontFirstChar;
        if (idx < 0 || idx >= kFontGlyphCount) {
            penX += charW;
            continue;
        }

        // Clip glyph bounding box.
        Rect glyphRect{penX, y, charW, charH};
        Rect clipped{};
        if (!ClipRectToCurrent(glyphRect, clipped)) {
            penX += charW;
            continue;
        }

        // UV coordinates from atlas grid position.
        const int col = idx % kCols;
        const int row = idx / kCols;
        const float u0 = static_cast<float>(col * kFontCellW) * invAtlasW;
        const float v0 = static_cast<float>(row * kFontCellH) * invAtlasH;
        const float u1 = static_cast<float>((col + 1) * kFontCellW) * invAtlasW;
        const float v1 = static_cast<float>((row + 1) * kFontCellH) * invAtlasH;

        const float x0 = penX;
        const float y0 = y;
        const float x1 = penX + charW;
        const float y1 = y + charH;

        // Two triangles per glyph quad.
        m_TextVertices.push_back({x0, y0, u0, v0, r, g, b, a});
        m_TextVertices.push_back({x1, y0, u1, v0, r, g, b, a});
        m_TextVertices.push_back({x1, y1, u1, v1, r, g, b, a});

        m_TextVertices.push_back({x0, y0, u0, v0, r, g, b, a});
        m_TextVertices.push_back({x1, y1, u1, v1, r, g, b, a});
        m_TextVertices.push_back({x0, y1, u0, v1, r, g, b, a});

        penX += charW;
    }
}

// ---------------------------------------------------------------------------
// Flush helpers
// ---------------------------------------------------------------------------

void UIRenderer::FlushRects() {
#ifdef NF_HAS_OPENGL
    if (m_Vertices.empty()) return;

    glUseProgram(static_cast<GLuint>(m_ShaderProgram));

    float L = 0.f, Right = m_ViewportWidth;
    float T = 0.f, Bot   = m_ViewportHeight;
    float proj[16] = {
        2.f / (Right - L),          0.f,                    0.f, 0.f,
        0.f,                        2.f / (T - Bot),        0.f, 0.f,
        0.f,                        0.f,                   -1.f, 0.f,
       -(Right + L) / (Right - L), -(T + Bot) / (T - Bot), 0.f, 1.f
    };

    GLint loc = glGetUniformLocation(static_cast<GLuint>(m_ShaderProgram), "uProjection");
    glUniformMatrix4fv(loc, 1, GL_FALSE, proj);

    glBindVertexArray(static_cast<GLuint>(m_VAO));
    glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(m_VBO));
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(m_Vertices.size() * sizeof(UIVertex)),
                 m_Vertices.data(), GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(m_Vertices.size()));

    glBindVertexArray(0);
    glUseProgram(0);
#endif
}

void UIRenderer::FlushText() {
#ifdef NF_HAS_OPENGL
    if (m_TextVertices.empty()) return;

    glUseProgram(static_cast<GLuint>(m_TextShaderProgram));

    float L = 0.f, Right = m_ViewportWidth;
    float T = 0.f, Bot   = m_ViewportHeight;
    float proj[16] = {
        2.f / (Right - L),          0.f,                    0.f, 0.f,
        0.f,                        2.f / (T - Bot),        0.f, 0.f,
        0.f,                        0.f,                   -1.f, 0.f,
       -(Right + L) / (Right - L), -(T + Bot) / (T - Bot), 0.f, 1.f
    };

    GLint locProj = glGetUniformLocation(static_cast<GLuint>(m_TextShaderProgram), "uProjection");
    glUniformMatrix4fv(locProj, 1, GL_FALSE, proj);

    GLint locTex = glGetUniformLocation(static_cast<GLuint>(m_TextShaderProgram), "uFontAtlas");
    glUniform1i(locTex, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(m_FontTexture));

    glBindVertexArray(static_cast<GLuint>(m_TextVAO));
    glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(m_TextVBO));
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(m_TextVertices.size() * sizeof(TextVertex)),
                 m_TextVertices.data(), GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(m_TextVertices.size()));

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);
    glUseProgram(0);
#endif
}

void UIRenderer::EndFrame() {
#ifdef NF_HAS_OPENGL
    if (!m_Initialised) return;

    // Save GL state we modify
    GLboolean depthWasEnabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean blendWasEnabled = glIsEnabled(GL_BLEND);
    GLint prevBlendSrc = 0, prevBlendDst = 0;
    glGetIntegerv(GL_BLEND_SRC, &prevBlendSrc);
    glGetIntegerv(GL_BLEND_DST, &prevBlendDst);

    // Set 2-D rendering state
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    FlushRects();
    FlushText();

    // Restore previous GL state
    if (depthWasEnabled) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    if (blendWasEnabled) glEnable(GL_BLEND);      else glDisable(GL_BLEND);
    glBlendFunc(static_cast<GLenum>(prevBlendSrc), static_cast<GLenum>(prevBlendDst));
#endif

    m_Vertices.clear();
    m_TextVertices.clear();
    m_ClipStack.clear();
}

} // namespace NF

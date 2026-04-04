#pragma once
#include "UI/Framework/Widget.h"
#include <cstdint>
#include <string_view>
#include <vector>

namespace NF {

/// @brief Immediate-mode 2-D renderer used by the UI layer.
///
/// Batches coloured quads and textured text quads between
/// BeginFrame() / EndFrame().  Rect drawing uses a simple
/// coloured-vertex shader.  Text rendering uses an 8×14
/// embedded bitmap font baked into a GL texture atlas
/// (see BuiltinFont.h).
class UIRenderer {
public:
    /// @brief Initialise the renderer (compile shaders, allocate buffers,
    ///        bake font atlas texture).
    /// @return True on success.
    bool Init();

    /// @brief Release all renderer-owned resources.
    void Shutdown();

    /// @brief Set the DPI scale factor (monitor DPI / 96).  Applied as a
    /// multiplier to the @p scale argument in DrawText().
    /// @param scale DPI scale; 1.0 = standard 96 DPI, 1.5 = 144 DPI, etc.
    void SetDpiScale(float scale) noexcept { m_DpiScale = scale > 0.f ? scale : 1.f; }

    /// @brief Return the current DPI scale factor.
    [[nodiscard]] float GetDpiScale() const noexcept { return m_DpiScale; }

    /// @brief Set the viewport size used for the orthographic projection.
    /// @param width  Viewport width in pixels.
    /// @param height Viewport height in pixels.
    void SetViewportSize(float width, float height);

    void BeginFrame();

    /// @brief Push a clip rect in top-left UI coordinates.
    void PushClipRect(const Rect& rect);

    /// @brief Pop the most recently pushed clip rect.
    void PopClipRect();

    void DrawRect(const Rect& rect, uint32_t color);
    void DrawOutlineRect(const Rect& rect, uint32_t color);
    void DrawText(std::string_view text, float x, float y, uint32_t color,
                  float scale = 1.0f);

    /// @brief Measure the pixel dimensions of a text string without drawing.
    /// @param text    The string to measure.
    /// @param scale   Base text scale (further multiplied by DPI scale).
    /// @param outWidth  Receives the total width in pixels.
    /// @param outHeight Receives the total height in pixels.
    void MeasureText(std::string_view text, float scale,
                     float& outWidth, float& outHeight) const noexcept;

    /// @brief Flush all batched rects and text to the GPU immediately.
    ///
    /// Call this between drawing passes when later draw calls must appear
    /// visually on top of earlier ones (e.g. before rendering a dropdown
    /// menu overlay).  Without an explicit flush, all rects are rendered
    /// in one pass and all text in a second pass, which can cause text
    /// from earlier draw calls to paint over backgrounds from later ones.
    void Flush();

    void EndFrame();

private:
    // ---- Coloured-quad vertex (rects) ----
    struct UIVertex {
        float X, Y;
        float R, G, B, A;
    };

    // ---- Textured vertex (font glyphs) ----
    struct TextVertex {
        float X, Y;
        float U, V;
        float R, G, B, A;
    };

    [[nodiscard]] bool HasClipRect() const noexcept { return !m_ClipStack.empty(); }
    [[nodiscard]] Rect CurrentClipRect() const noexcept;
    [[nodiscard]] Rect IntersectRect(const Rect& a, const Rect& b) const noexcept;
    [[nodiscard]] bool ClipRectToCurrent(const Rect& input, Rect& output) const noexcept;
    void FlushRects();
    void FlushText();

    bool  m_Initialised{false};
    float m_ViewportWidth{1280.f};
    float m_ViewportHeight{720.f};
    float m_DpiScale{1.f};

    // ---- Rect pipeline (coloured quads) ----
    uint32_t m_ShaderProgram{0};
    uint32_t m_VAO{0};
    uint32_t m_VBO{0};

    // ---- Text pipeline (textured quads) ----
    uint32_t m_TextShaderProgram{0};
    uint32_t m_TextVAO{0};
    uint32_t m_TextVBO{0};
    uint32_t m_FontTexture{0};  ///< GL texture holding the font atlas.
    int      m_AtlasW{0};       ///< Atlas width in pixels.
    int      m_AtlasH{0};       ///< Atlas height in pixels.

    std::vector<UIVertex>   m_Vertices;      ///< Rect batch.
    std::vector<TextVertex> m_TextVertices;   ///< Text batch.
    std::vector<Rect>       m_ClipStack;
};

} // namespace NF

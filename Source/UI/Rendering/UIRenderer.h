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

    /// @brief Measure the pixel width and height of a text string at the given
    ///        scale.  Returns effective rendered dimensions including DPI scaling.
    ///        Only counts printable ASCII characters (the bitmap font covers 32–127).
    /// @param text  The ASCII string to measure.
    /// @param scale Text scale factor (same semantics as DrawText).
    /// @param outWidth  Receives the text width in pixels (DPI-scaled).
    /// @param outHeight Receives the text height in pixels (DPI-scaled).
    void MeasureText(std::string_view text, float scale,
                     float& outWidth, float& outHeight) const noexcept;

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

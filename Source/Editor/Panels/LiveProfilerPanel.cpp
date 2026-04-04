#include "Editor/Panels/LiveProfilerPanel.h"
#include "Editor/Panels/EditorTheme.h"
#include "UI/Rendering/UIRenderer.h"
#include <algorithm>
#include <string>

namespace NF::Editor {

void LiveProfilerPanel::Update([[maybe_unused]] float dt) {}

// ---------------------------------------------------------------------------
// Draw
// ---------------------------------------------------------------------------

void LiveProfilerPanel::Draw(float x, float y, float w, float h)
{
    if (!m_Renderer || !m_Backend) return;

    const auto& t = ActiveTheme();
    const float dpi   = m_Renderer->GetDpiScale();
    const float padX  = 6.f * dpi;
    const float lineH = 16.f * dpi;
    const float scale = 1.f;

    float cy = y + 4.f * dpi;

    // Title + summary stats
    {
        const double avgCpu = m_Backend->AverageCpuMs(60);
        const double peakCpu = m_Backend->PeakCpuMs();
        const double avgGpu = m_Backend->AverageGpuMs(60);

        // Helper: format a double to 1 decimal place, capped at 5 chars.
        auto fmtMs = [](double v) -> std::string {
            // Round to 1 decimal place then convert.
            int tenths = static_cast<int>(v * 10.0 + 0.5);
            std::string s = std::to_string(tenths / 10) + "." + std::to_string(tenths % 10);
            if (s.size() > 5) s.resize(5);
            return s;
        };

        std::string title = "Profiler  CPU avg:" + fmtMs(avgCpu) + "ms"
            + "  peak:" + fmtMs(peakCpu) + "ms";
        if (avgGpu > 0.0)
            title += "  GPU:" + fmtMs(avgGpu) + "ms";

        m_Renderer->DrawText(title, x + padX, cy, t.textHeader, scale);
        cy += lineH;
        m_Renderer->DrawRect({x, cy, w, 1.f}, t.separator);
        cy += 2.f * dpi;
    }

    // Frame-time graph occupies the top ~45% of remaining space
    const float graphH = std::max(40.f * dpi, (y + h - cy) * 0.45f);
    DrawGraph(x + padX, cy, w - 2.f * padX, graphH);
    cy += graphH + 4.f * dpi;

    m_Renderer->DrawRect({x, cy, w, 1.f}, t.separator);
    cy += 4.f * dpi;

    // Scope list fills the rest
    DrawScopes(x + padX, cy, w - 2.f * padX, y + h - cy);
}

// ---------------------------------------------------------------------------
// DrawGraph — rolling frame-time bar graph
// ---------------------------------------------------------------------------

void LiveProfilerPanel::DrawGraph(float x, float y, float w, float h)
{
    const auto& t = ActiveTheme();
    const float dpi = m_Renderer->GetDpiScale();
    const auto& samples = m_Backend->GetSamples();

    // Background
    m_Renderer->DrawRect({x, y, w, h}, 0x0A0A0AFF);

    if (samples.empty()) {
        m_Renderer->DrawText("(no samples)", x + 4.f * dpi, y + 4.f * dpi,
                             t.textSecondary, 1.f);
        return;
    }

    // Target line at 16.67 ms (60 FPS)
    constexpr double kTarget60 = 16.667;
    // Scale: graph top = 0 ms, graph bottom = maxMs (auto-range)
    double maxMs = kTarget60 * 2.0; // At least show 2× 60fps target
    for (const auto& s : samples)
        if (s.cpuMs > maxMs) maxMs = s.cpuMs;

    // Draw 60fps target line
    const float targetY = y + h - static_cast<float>(kTarget60 / maxMs) * h;
    if (targetY > y && targetY < y + h) {
        m_Renderer->DrawRect({x, targetY, w, 1.f}, 0x40A040FF);
        m_Renderer->DrawText("16.6ms", x + 2.f * dpi, targetY - 12.f * dpi,
                             0x40A040FF, 1.f);
    }

    // Draw bars from the right edge going left
    const size_t count = samples.size();
    const float barW = std::max(1.f, w / static_cast<float>(LiveProfilerBackend::kHistorySize));

    for (size_t i = 0; i < count; ++i) {
        const float barX = x + w - static_cast<float>(count - i) * barW;
        if (barX + barW < x) continue;

        const double ms = samples[i].cpuMs;
        const float  barH = std::max(1.f, static_cast<float>(ms / maxMs) * h);
        const float  barY = y + h - barH;

        // Colour: green when under 16.6ms, yellow up to 33ms, red above
        uint32_t barCol;
        if (ms <= kTarget60)         barCol = 0x30A030FF;
        else if (ms <= kTarget60 * 2) barCol = 0xC0A020FF;
        else                          barCol = 0xC03030FF;

        m_Renderer->DrawRect({barX, barY, barW - 1.f, barH}, barCol);
    }

    // Border
    m_Renderer->DrawOutlineRect({x, y, w, h}, t.panelBorder);
}

// ---------------------------------------------------------------------------
// DrawScopes — per-scope duration list from the last frame
// ---------------------------------------------------------------------------

void LiveProfilerPanel::DrawScopes(float x, float y, float w, float h)
{
    const auto& t = ActiveTheme();
    const float dpi   = m_Renderer->GetDpiScale();
    const float lineH = 14.f * dpi;
    const float scale = 1.f;

    const auto& scopes = m_Backend->GetLastFrameScopes();
    if (scopes.empty()) {
        m_Renderer->DrawText("(no scopes)", x, y, t.textSecondary, scale);
        return;
    }

    float cy = y;
    for (const auto& scope : scopes) {
        if (cy + lineH > y + h) break;

        // Indent based on depth
        const float indent = static_cast<float>(scope.depth) * 12.f * dpi;

        // Format: "ScopeName  1.23ms"
        std::string durStr = std::to_string(scope.durationMs);
        // Trim to 2 decimal places
        auto dot = durStr.find('.');
        if (dot != std::string::npos && durStr.size() > dot + 3)
            durStr.resize(dot + 3);
        std::string line = scope.name + "  " + durStr + "ms";

        // Colour: bright if > 1ms, dim otherwise
        const uint32_t col = (scope.durationMs > 1.0) ? t.textHeader : t.textSecondary;
        m_Renderer->DrawText(line, x + indent, cy, col, scale);
        cy += lineH;
    }
}

} // namespace NF::Editor

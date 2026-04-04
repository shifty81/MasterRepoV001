#include "Editor/Panels/PreferencesPanel.h"
#include "Editor/Panels/EditorTheme.h"
#include "Editor/Application/EditorInputState.h"
#include "UI/Rendering/UIRenderer.h"
#include "Core/Logging/Log.h"
#include <fstream>
#include <sstream>
#include <string>

namespace NF::Editor {

// ---------------------------------------------------------------------------
// Minimal INI helpers (no external dependency)
// ---------------------------------------------------------------------------

static std::string ThemeToString(EditorTheme t)
{
    switch (t) {
        case EditorTheme::Light:       return "Light";
        case EditorTheme::HighContrast: return "HighContrast";
        default:                       return "Dark";
    }
}

static EditorTheme ThemeFromString(const std::string& s)
{
    if (s == "Light")        return EditorTheme::Light;
    if (s == "HighContrast") return EditorTheme::HighContrast;
    return EditorTheme::Dark;
}

// ---------------------------------------------------------------------------

bool PreferencesPanel::Load(const std::string& path)
{
    std::ifstream f(path);
    if (!f.is_open()) return false;

    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#' || line[0] == '[') continue;
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);

        if (key == "theme")                m_Data.theme               = ThemeFromString(val);
        else if (key == "fpsLimit")        m_Data.fpsLimit            = static_cast<uint32_t>(std::stoul(val));
        else if (key == "autosaveEnabled") m_Data.autosaveEnabled     = (val == "1" || val == "true");
        else if (key == "autosaveInterval") m_Data.autosaveIntervalSec = static_cast<uint32_t>(std::stoul(val));
        else if (key == "showGrid")        m_Data.showGrid            = (val == "1" || val == "true");
        else if (key == "uiScale")         m_Data.uiScale             = std::stof(val);
        else if (key == "recentProjectsDir") m_Data.recentProjectsDir = val;
        else if (key == "dockLayout")        m_Data.dockLayout        = val;
    }
    Logger::Log(LogLevel::Info, "Preferences", "Loaded from " + path);
    return true;
}

bool PreferencesPanel::Save(const std::string& path) const
{
    std::ofstream f(path);
    if (!f.is_open()) return false;

    f << "[editor]\n";
    f << "theme="                << ThemeToString(m_Data.theme)            << "\n";
    f << "fpsLimit="             << m_Data.fpsLimit                        << "\n";
    f << "autosaveEnabled="      << (m_Data.autosaveEnabled ? "1" : "0")  << "\n";
    f << "autosaveInterval="     << m_Data.autosaveIntervalSec             << "\n";
    f << "showGrid="             << (m_Data.showGrid ? "1" : "0")         << "\n";
    f << "uiScale="              << m_Data.uiScale                         << "\n";
    f << "recentProjectsDir="    << m_Data.recentProjectsDir               << "\n";
    f << "dockLayout="           << m_Data.dockLayout                      << "\n";

    Logger::Log(LogLevel::Info, "Preferences", "Saved to " + path);
    return true;
}

void PreferencesPanel::Update(float dt)
{
    if (!m_Data.autosaveEnabled) return;
    m_AutosaveTimer += dt;
    if (m_AutosaveTimer >= static_cast<float>(m_Data.autosaveIntervalSec)) {
        m_AutosaveTimer = 0.f;
        Logger::Log(LogLevel::Info, "Preferences", "Auto-save triggered");
        Save("editor_prefs.ini");
    }
}

// ---------------------------------------------------------------------------
// Draw — render the preferences panel with real UI controls
// ---------------------------------------------------------------------------

void PreferencesPanel::Draw(float x, float y, float w, float h)
{
    if (!m_Open) return;
    if (!m_Renderer) return;

    const auto& t = ActiveTheme();
    const uint32_t kHeaderColor  = t.textHeader;
    const uint32_t kLabelColor   = t.textSecondary;
    const uint32_t kValueColor   = t.textValue;
    const uint32_t kBoolOnColor  = t.boolOn;
    const uint32_t kBoolOffColor = t.boolOff;
    const uint32_t kSepColor     = t.separator;
    const uint32_t kBtnBg        = t.buttonBg;
    const uint32_t kBtnHover     = t.buttonHover;
    const uint32_t kDirtyColor   = t.dirty;
    const uint32_t kPanelBg      = t.panelBg;

    const float dpi   = m_Renderer->GetDpiScale();
    const float lineH = 18.f * dpi;
    const float padX  = 10.f * dpi;
    const float scale = 1.f;
    const float colW  = w * 0.45f;
    float cy = y + 6.f * dpi;

    // Panel background
    m_Renderer->DrawRect({x, y, w, h}, kPanelBg);

    // Title
    std::string title = "Preferences";
    if (m_Dirty) title += " [*]";
    m_Renderer->DrawText(title, x + padX, cy, kHeaderColor, scale);
    cy += lineH;
    m_Renderer->DrawRect({x, cy, w, 1.f}, kSepColor);
    cy += 4.f * dpi;

    // --- Helper: test if mouse is inside a rectangular region ---
    auto isHovered = [&](float rx, float ry, float rw, float rh) -> bool {
        return m_Input &&
            m_Input->mouseX >= rx && m_Input->mouseX < rx + rw &&
            m_Input->mouseY >= ry && m_Input->mouseY < ry + rh;
    };

    // --- Helper lambdas for drawing rows ---
    auto drawLabel = [&](const std::string& label) {
        m_Renderer->DrawText(label + ":", x + padX, cy, kLabelColor, scale);
    };

    auto drawBoolRow = [&](const std::string& label, bool& value) {
        if (cy + lineH > y + h) return;
        drawLabel(label);
        const uint32_t color = value ? kBoolOnColor : kBoolOffColor;
        const std::string text = value ? "[ON]" : "[OFF]";
        const float bx = x + padX + colW;
        const float bw = 40.f * dpi;
        const bool hovered = isHovered(bx, cy, bw, lineH);
        if (hovered)
            m_Renderer->DrawRect({bx, cy, bw, lineH}, kBtnHover);
        m_Renderer->DrawText(text, bx + 2.f * dpi, cy + 2.f * dpi, color, scale);
        if (hovered && m_Input && m_Input->leftJustPressed) {
            value = !value;
            m_Dirty = true;
        }
        cy += lineH;
    };

    auto drawStringRow = [&](const std::string& label, const std::string& value) {
        if (cy + lineH > y + h) return;
        drawLabel(label);
        m_Renderer->DrawText(value.empty() ? "(empty)" : value,
                             x + padX + colW, cy + 2.f * dpi, kValueColor, scale);
        cy += lineH;
    };

    auto drawIntRow = [&](const std::string& label, uint32_t value) {
        if (cy + lineH > y + h) return;
        drawLabel(label);
        m_Renderer->DrawText(std::to_string(value),
                             x + padX + colW, cy + 2.f * dpi, kValueColor, scale);
        cy += lineH;
    };

    auto drawFloatRow = [&](const std::string& label, float value) {
        if (cy + lineH > y + h) return;
        drawLabel(label);
        std::string s = std::to_string(value);
        // Trim trailing zeros for readability
        if (s.find('.') != std::string::npos) {
            s.erase(s.find_last_not_of('0') + 1, std::string::npos);
            if (s.back() == '.') s += '0';
        }
        m_Renderer->DrawText(s, x + padX + colW, cy + 2.f * dpi, kValueColor, scale);
        cy += lineH;
    };

    // --- Draw preference rows ---

    // Theme row — clickable to cycle Dark → Light → HighContrast → Dark
    {
        if (cy + lineH <= y + h) {
            drawLabel("Theme");
            const std::string themeStr = ThemeToString(m_Data.theme);
            const float bx = x + padX + colW;
            const float bw = 100.f * dpi;
            const bool hovered = isHovered(bx, cy, bw, lineH);
            if (hovered)
                m_Renderer->DrawRect({bx, cy, bw, lineH}, kBtnHover);
            m_Renderer->DrawText("[" + themeStr + "]", bx + 2.f * dpi, cy + 2.f * dpi,
                                 kValueColor, scale);
            if (hovered && m_Input && m_Input->leftJustPressed) {
                switch (m_Data.theme) {
                case EditorTheme::Dark:         m_Data.theme = EditorTheme::Light;        break;
                case EditorTheme::Light:        m_Data.theme = EditorTheme::HighContrast; break;
                case EditorTheme::HighContrast: m_Data.theme = EditorTheme::Dark;         break;
                }
                m_Dirty = true;
            }
            cy += lineH;
        }
    }

    drawIntRow("FPS Limit",    m_Data.fpsLimit);
    drawBoolRow("Autosave",    m_Data.autosaveEnabled);
    drawIntRow("Autosave Sec", m_Data.autosaveIntervalSec);
    drawBoolRow("Show Grid",   m_Data.showGrid);
    drawFloatRow("UI Scale",   m_Data.uiScale);
    drawStringRow("Projects Dir", m_Data.recentProjectsDir);

    // --- Separator ---
    cy += 4.f * dpi;
    m_Renderer->DrawRect({x, cy, w, 1.f}, kSepColor);
    cy += 8.f * dpi;

    // --- Apply / Close buttons ---
    const float btnW = 64.f * dpi;
    const float btnH = 20.f * dpi;
    float bx = x + padX;

    // Apply button
    if (cy + btnH <= y + h) {
        const bool applyHov = isHovered(bx, cy, btnW, btnH);
        m_Renderer->DrawRect({bx, cy, btnW, btnH}, applyHov ? kBtnHover : kBtnBg);
        m_Renderer->DrawText("Apply", bx + 6.f * dpi, cy + 2.f * dpi,
                             m_Dirty ? kDirtyColor : kValueColor, scale);
        if (applyHov && m_Input && m_Input->leftJustPressed) {
            Save("editor_prefs.ini");
            m_Dirty = false;
        }
    }

    bx += btnW + 8.f * dpi;

    // Close button
    if (cy + btnH <= y + h) {
        const bool closeHov = isHovered(bx, cy, btnW, btnH);
        m_Renderer->DrawRect({bx, cy, btnW, btnH}, closeHov ? kBtnHover : kBtnBg);
        m_Renderer->DrawText("Close", bx + 6.f * dpi, cy + 2.f * dpi, kValueColor, scale);
        if (closeHov && m_Input && m_Input->leftJustPressed) {
            m_Open = false;
        }
    }
}

} // namespace NF::Editor

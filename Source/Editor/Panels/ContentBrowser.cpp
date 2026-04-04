#include "Editor/Panels/ContentBrowser.h"
#include "Editor/Panels/EditorTheme.h"
#include "UI/Rendering/UIRenderer.h"
#include "Core/Logging/Log.h"
#include <filesystem>
#include <algorithm>

namespace NF::Editor {

// ---------------------------------------------------------------------------
// ScanDirectory — recursively scan the content root up to 3 levels deep
// ---------------------------------------------------------------------------

void ContentBrowser::ScanDirectory()
{
    m_Entries.clear();
    m_Dirty = false;

    if (m_RootPath.empty()) return;

    namespace fs = std::filesystem;
    std::error_code ec;

    if (!fs::is_directory(m_RootPath, ec)) {
        Logger::Log(LogLevel::Warning, "ContentBrowser",
                    "Root path is not a directory: " + m_RootPath);
        return;
    }

    /// @brief Scan depth limit — keeps the tree manageable in both
    ///        performance and visual density terms.
    constexpr int kMaxDepth = 3;

    // Recursive helper using a std::function to avoid exposing internals.
    struct ScanCtx {
        std::vector<Entry>& entries;
        int maxDepth;

        void Scan(const fs::path& dir, int depth) {
            if (depth > maxDepth) return;
            std::error_code ec2;
            std::vector<fs::directory_entry> children;

            for (const auto& de : fs::directory_iterator(dir, ec2)) {
                children.push_back(de);
            }

            // Sort alphabetically; directories first.
            std::sort(children.begin(), children.end(),
                [](const fs::directory_entry& a, const fs::directory_entry& b) {
                    const bool aDir = a.is_directory();
                    const bool bDir = b.is_directory();
                    if (aDir != bDir) return aDir > bDir;
                    return a.path().filename().string() < b.path().filename().string();
                });

            for (const auto& child : children) {
                Entry entry;
                entry.depth = depth;
                const auto fname = child.path().filename().string();
                if (child.is_directory()) {
                    entry.name = fname + "/";
                    entry.isDirectory = true;
                } else {
                    entry.name = fname;
                    entry.isDirectory = false;
                }
                entries.push_back(entry);

                if (child.is_directory())
                    Scan(child.path(), depth + 1);
            }
        }
    };

    ScanCtx ctx{m_Entries, kMaxDepth};
    ctx.Scan(fs::path(m_RootPath), 0);

    Logger::Log(LogLevel::Debug, "ContentBrowser",
                "Scanned " + std::to_string(m_Entries.size()) + " entries in " + m_RootPath);
}

// ---------------------------------------------------------------------------
// Update — trigger rescan when dirty or on a periodic timer
// ---------------------------------------------------------------------------

void ContentBrowser::Update(float dt)
{
    m_RescanTimer += dt;
    if (m_Dirty || m_RescanTimer >= kRescanInterval) {
        ScanDirectory();
        m_RescanTimer = 0.f;
    }
}

// ---------------------------------------------------------------------------
// Draw — render the content tree with indentation, icons, hover & selection
// ---------------------------------------------------------------------------

void ContentBrowser::Draw(float x, float y, float w, float h) {
    if (!m_Renderer) return;

    const auto& t = ActiveTheme();
    const uint32_t kTextColor    = t.textPrimary;
    const uint32_t kLabelColor   = t.textSecondary;
    const uint32_t kHoverColor   = t.hoverBg;
    const uint32_t kSelectColor  = t.selectBg;
    const uint32_t kDirColor     = t.dirColor;

    const float dpi   = m_Renderer->GetDpiScale();
    const float lineH = 18.f * dpi;
    const float padX  = 6.f  * dpi;
    const float indent = 14.f * dpi;
    const float scale = 1.f;
    float cy = y + 4.f * dpi;

    // Root path label (non-clickable header)
    const std::string rootLabel = "Content: " + (m_RootPath.empty() ? "(none)" : m_RootPath);
    m_Renderer->DrawText(rootLabel, x + padX, cy, kLabelColor, scale);
    cy += lineH;
    m_Renderer->DrawRect({x, cy, w, 1.f}, 0x444444FF);
    cy += 2.f * dpi;

    // Draw scanned entries
    for (const auto& entry : m_Entries) {
        if (cy + lineH > y + h) break;

        const float entryX = x + padX + static_cast<float>(entry.depth) * indent;
        const bool hovered  = m_Input &&
                              m_Input->mouseX >= x          && m_Input->mouseX < x + w &&
                              m_Input->mouseY >= cy         && m_Input->mouseY < cy + lineH;
        const bool selected = (m_SelectedAsset == entry.name);

        if (selected) {
            m_Renderer->DrawRect({x, cy, w, lineH}, kSelectColor);
        } else if (hovered) {
            m_Renderer->DrawRect({x, cy, w, lineH}, kHoverColor);
        }

        if (hovered && m_Input && m_Input->leftJustPressed) {
            m_SelectedAsset = entry.name;
        }

        // Directory prefix indicator
        const std::string prefix = entry.isDirectory ? "> " : "  ";
        const uint32_t rowColor = selected ? 0xFFFFFFFF
                                : entry.isDirectory ? kDirColor : kTextColor;
        m_Renderer->DrawText(prefix + entry.name, entryX, cy + 2.f * dpi, rowColor, scale);
        cy += lineH;
    }

    if (m_Entries.empty()) {
        m_Renderer->DrawText("(empty)", x + padX, cy, kLabelColor, scale);
    }
}

} // namespace NF::Editor

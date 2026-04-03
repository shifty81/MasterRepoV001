#include "Editor/Panels/DockingSystem.h"
#include "Editor/Application/EditorInputState.h"
#include "Editor/Panels/EditorTheme.h"
#include "UI/Rendering/UIRenderer.h"
#include "Core/Logging/Log.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace NF::Editor {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Return the name of the currently active panel for a leaf node.
/// For tabbed nodes this is the selected tab; otherwise just panelName.
static const std::string& ActivePanelName(const DockNode& node) noexcept
{
    if (!node.tabNames.empty()) {
        const int idx = std::clamp(node.activeTabIdx, 0,
                                   static_cast<int>(node.tabNames.size()) - 1);
        return node.tabNames[static_cast<size_t>(idx)];
    }
    return node.panelName;
}

/// Approximate width of a single character at scale 1.0 for tab width estimation.
static constexpr float kTabCharWidth = 6.f;

/// Maximum width of a single tab header (pre-DPI).
static constexpr float kMaxTabWidth = 120.f;

uint32_t DockingSystem::AllocNode()
{
    DockNode node{};
    node.id = m_NextId++;
    m_Nodes.push_back(node);
    return node.id;
}

DockNode* DockingSystem::FindLeaf(const std::string& name)
{
    for (auto& n : m_Nodes)
        if (n.isLeaf && n.panelName == name)
            return &n;
    return nullptr;
}

DockingSystem::DrawFn* DockingSystem::FindDrawFn(const std::string& name)
{
    for (auto& p : m_Panels)
        if (p.name == name)
            return &p.drawFn;
    return nullptr;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void DockingSystem::RegisterPanel(const std::string& name, DrawFn drawFn)
{
    for (auto& p : m_Panels) {
        if (p.name == name) {
            p.drawFn = std::move(drawFn);
            return;
        }
    }
    m_Panels.push_back({name, std::move(drawFn)});
    Logger::Log(LogLevel::Debug, "DockingSystem",
                "Registered panel: " + name);
}

void DockingSystem::SetPanelTransparent(const std::string& name)
{
    for (const auto& n : m_TransparentPanels)
        if (n == name) return;
    m_TransparentPanels.push_back(name);
}

void DockingSystem::SetRootSplit(const std::string& firstPanel,
                                  const std::string& secondPanel,
                                  float ratio,
                                  SplitAxis axis)
{
    m_Nodes.clear();
    m_NextId = 1;

    // Root split node
    DockNode root{};
    root.id         = m_NextId++;
    root.isLeaf     = false;
    root.axis       = axis;
    root.splitRatio = std::clamp(ratio, 0.05f, 0.95f);
    root.firstChild = m_NextId;      // will be set after

    // Left/first leaf
    DockNode left{};
    left.id        = m_NextId++;
    left.isLeaf    = true;
    left.panelName = firstPanel;

    // Right/second leaf
    DockNode right{};
    right.id        = m_NextId++;
    right.isLeaf    = true;
    right.panelName = secondPanel;

    root.firstChild  = left.id;
    root.secondChild = right.id;

    m_Nodes.push_back(root);
    m_Nodes.push_back(left);
    m_Nodes.push_back(right);

    Logger::Log(LogLevel::Info, "DockingSystem",
                "Root split: " + firstPanel + " | " + secondPanel);
}

void DockingSystem::SplitPanel(const std::string& parentName,
                                const std::string& newName,
                                SplitAxis axis,
                                float ratio)
{
    DockNode* leaf = FindLeaf(parentName);
    if (!leaf) {
        Logger::Log(LogLevel::Warning, "DockingSystem",
                    "SplitPanel: panel not found: " + parentName);
        return;
    }

    // Convert leaf into a split
    std::string existingName = leaf->panelName;
    leaf->isLeaf             = false;
    leaf->axis               = axis;
    leaf->splitRatio         = std::clamp(ratio, 0.05f, 0.95f);
    leaf->panelName.clear();

    // First child keeps the original panel name
    DockNode first{};
    first.id        = m_NextId++;
    first.isLeaf    = true;
    first.panelName = existingName;

    // Second child holds the new panel
    DockNode second{};
    second.id        = m_NextId++;
    second.isLeaf    = true;
    second.panelName = newName;

    leaf->firstChild  = first.id;
    leaf->secondChild = second.id;

    m_Nodes.push_back(first);
    m_Nodes.push_back(second);

    Logger::Log(LogLevel::Info, "DockingSystem",
                "Split '" + existingName + "' → '" + newName + "'");
}

void DockingSystem::Update(float /*dt*/) {}

// ---------------------------------------------------------------------------
// AddTab
// ---------------------------------------------------------------------------

void DockingSystem::AddTab(const std::string& existingPanel,
                           const std::string& newPanel)
{
    DockNode* leaf = FindLeaf(existingPanel);
    if (!leaf) {
        Logger::Log(LogLevel::Warning, "DockingSystem",
                    "AddTab: panel not found: " + existingPanel);
        return;
    }

    // Initialize the tab list the first time we add a tab.
    if (leaf->tabNames.empty())
        leaf->tabNames.push_back(leaf->panelName);

    leaf->tabNames.push_back(newPanel);
    Logger::Log(LogLevel::Info, "DockingSystem",
                "Tab added: '" + newPanel + "' to '" + existingPanel + "'");
}

void DockingSystem::CachePanelRect(const std::string& name,
                                    float x, float y, float w, float h)
{
    for (auto& pr : m_LastPanelRects) {
        if (pr.name == name) {
            pr.x = x; pr.y = y; pr.w = w; pr.h = h;
            return;
        }
    }
    m_LastPanelRects.push_back({name, x, y, w, h});
}

void DockingSystem::LayoutNode(const DockNode& node,
                                float x, float y, float w, float h)
{
    if (node.isLeaf) {
        const float dpi       = m_Renderer ? m_Renderer->GetDpiScale() : 1.f;
        const float titleBarH = kPanelTitleBarHeight * dpi;
        const float contentY  = y + titleBarH;
        const float contentH  = h - titleBarH;
        CachePanelRect(node.panelName, x, contentY, w, contentH);

        // For tabbed nodes, cache the rect for every tab (they share bounds).
        for (const auto& tabName : node.tabNames) {
            if (tabName != node.panelName)
                CachePanelRect(tabName, x, contentY, w, contentH);
        }
        return;
    }

    DockNode* first  = nullptr;
    DockNode* second = nullptr;
    for (auto& n : m_Nodes) {
        if (n.id == node.firstChild)  first  = &n;
        if (n.id == node.secondChild) second = &n;
    }
    if (!first || !second) return;

    if (node.axis == SplitAxis::Horizontal) {
        float leftW = w * node.splitRatio;
        LayoutNode(*first,  x,         y, leftW,     h);
        LayoutNode(*second, x + leftW, y, w - leftW, h);
    } else {
        float topH = h * node.splitRatio;
        LayoutNode(*first,  x, y,        w, topH);
        LayoutNode(*second, x, y + topH, w, h - topH);
    }
}

void DockingSystem::BuildLayout(float x, float y, float totalWidth, float totalHeight)
{
    m_LastPanelRects.clear();
    if (m_Nodes.empty()) return;
    LayoutNode(m_Nodes[0], x, y, totalWidth, totalHeight);
}

bool DockingSystem::GetPanelRect(const std::string& name,
                                  float& x, float& y, float& w, float& h) const noexcept
{
    for (const auto& pr : m_LastPanelRects) {
        if (pr.name == name) {
            x = pr.x; y = pr.y; w = pr.w; h = pr.h;
            return true;
        }
    }
    return false;
}

void DockingSystem::DrawNode(DockNode& node,
                              float x, float y, float w, float h)
{
    if (node.isLeaf) {
        // Draw panel chrome via UIRenderer if available
        if (m_Renderer && w > 0.f && h > 0.f) {
            const float dpi       = m_Renderer->GetDpiScale();
            const float titleBarH = kPanelTitleBarHeight * dpi;
            const float contentY  = y + titleBarH;
            const float contentH  = h - titleBarH;
            const auto& theme     = ActiveTheme();

            // Determine which panel name to draw (active tab or the only panel).
            const bool hasTabs   = !node.tabNames.empty();
            const int  tabCount  = hasTabs ? static_cast<int>(node.tabNames.size()) : 0;
            const std::string& activeName = ActivePanelName(node);

            // Check whether the active panel has opted out of opaque backgrounds
            bool isTransparent = false;
            for (const auto& n : m_TransparentPanels)
                if (n == activeName) { isTransparent = true; break; }

            if (!isTransparent) {
                // Panel background
                m_Renderer->DrawRect({x, y, w, h}, theme.panelBg);
                // Content shade
                if (contentH > 0.f)
                    m_Renderer->DrawRect({x, contentY, w, contentH}, theme.contentShade);
            }

            // ---- Title bar / tab bar ----
            m_Renderer->DrawRect({x, y, w, titleBarH}, theme.titleBarBg);

            if (hasTabs) {
                // Draw clickable tab headers.
                const float tabPad  = 4.f * dpi;
                const float tabH    = titleBarH - 2.f * dpi; // leave 2px for top accent
                const float tabY    = y + 2.f * dpi;
                float       tabX    = x + tabPad;
                const float maxTabW = kMaxTabWidth * dpi;

                for (int ti = 0; ti < tabCount; ++ti) {
                    const auto& tabName = node.tabNames[static_cast<size_t>(ti)];
                    const bool  isActive = (ti == node.activeTabIdx);

                    // Measure approximate tab width from label length.
                    const float labelW = static_cast<float>(tabName.size()) * kTabCharWidth * dpi * 1.8f;
                    const float tabW   = std::min(labelW + 16.f * dpi, maxTabW);

                    // Hit test
                    const bool hovered = m_Input &&
                        m_Input->mouseX >= tabX       && m_Input->mouseX < tabX + tabW &&
                        m_Input->mouseY >= tabY       && m_Input->mouseY < tabY + tabH;
                    if (hovered && m_Input && m_Input->leftJustPressed)
                        node.activeTabIdx = ti;

                    // Tab background
                    uint32_t tabBg = isActive ? theme.panelBg
                                  : hovered  ? theme.hoverBg
                                             : theme.titleBarBg;
                    m_Renderer->DrawRect({tabX, tabY, tabW, tabH}, tabBg);

                    // Active accent bar at the top of the tab
                    if (isActive)
                        m_Renderer->DrawRect({tabX, y, tabW, 2.f * dpi}, theme.worldAccent);

                    // Tab label
                    const uint32_t textCol = isActive ? theme.textTitle : theme.textSecondary;
                    m_Renderer->DrawText(tabName, tabX + 6.f * dpi, tabY + 3.f * dpi,
                                         textCol, 1.8f);

                    // Thin separator between tabs
                    if (ti < tabCount - 1)
                        m_Renderer->DrawRect({tabX + tabW, tabY + 2.f * dpi,
                                              1.f, tabH - 4.f * dpi}, theme.separator);

                    tabX += tabW + 2.f * dpi;
                }
            } else {
                // Single-panel title bar (original behaviour).
                m_Renderer->DrawRect({x, y, w, 2.f * dpi}, theme.titleBarAccent);
                if (!node.panelName.empty()) {
                    m_Renderer->DrawText(node.panelName,
                                         x + 6.f * dpi, y + 4.f * dpi,
                                         theme.textTitle, 2.f);
                }
            }

            // Panel border
            m_Renderer->DrawOutlineRect({x, y, w, h}, theme.panelBorder);
        }

        // Call the active panel's content draw callback.
        const std::string& drawName = ActivePanelName(node);

        if (DrawFn* fn = FindDrawFn(drawName)) {
            const float dpi       = m_Renderer ? m_Renderer->GetDpiScale() : 1.f;
            const float titleBarH = kPanelTitleBarHeight * dpi;
            const float contentY  = y + titleBarH;
            const float contentH  = h - titleBarH;
            if (contentH > 0.f) {
                if (m_Renderer) m_Renderer->PushClipRect({x, contentY, w, contentH});
                (*fn)(x, contentY, w, contentH);
                if (m_Renderer) m_Renderer->PopClipRect();
            }
        }
        return;
    }

    // Find children
    DockNode* first  = nullptr;
    DockNode* second = nullptr;
    for (auto& n : m_Nodes) {
        if (n.id == node.firstChild)  first  = &n;
        if (n.id == node.secondChild) second = &n;
    }
    if (!first || !second) return;

    if (node.axis == SplitAxis::Horizontal) {
        float leftW = w * node.splitRatio;
        DrawNode(*first,  x,          y, leftW,      h);
        DrawNode(*second, x + leftW,  y, w - leftW,  h);
    } else {
        float topH = h * node.splitRatio;
        DrawNode(*first,  x, y,          w, topH);
        DrawNode(*second, x, y + topH,   w, h - topH);
    }
}

void DockingSystem::Draw(float x, float y, float totalWidth, float totalHeight)
{
    if (m_Nodes.empty()) return;
    BuildLayout(x, y, totalWidth, totalHeight);
    DrawNode(m_Nodes[0], x, y, totalWidth, totalHeight);
}

} // namespace NF::Editor

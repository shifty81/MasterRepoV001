#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace NF { class UIRenderer; }

namespace NF::Editor {

struct EditorInputState;

enum class SplitAxis { Horizontal, Vertical };

struct DockNode {
    uint32_t    id{0};
    SplitAxis   axis{SplitAxis::Horizontal};
    float       splitRatio{0.5f};
    uint32_t    firstChild{0};
    uint32_t    secondChild{0};
    std::string panelName;
    bool        isLeaf{true};

    // ---- Tab support ----
    /// When non-empty the leaf hosts multiple panels as tabs.
    /// The first entry matches `panelName` (the original panel).
    std::vector<std::string> tabNames;
    int activeTabIdx{0};
};

constexpr float kPanelTitleBarHeight = 22.f;
class DockingSystem {
public:
    using DrawFn        = std::function<void(float x, float y, float w, float h)>;
    /// @brief Optional overlay drawn inside the title bar of a single (non-tabbed) panel.
    /// Receives the full title-bar rect; the callback is responsible for right-aligning
    /// any controls it draws within that region.
    using HeaderExtrasFn = std::function<void(float x, float y, float w, float h)>;

    struct PanelRect {
        std::string name;
        float x{0.f};
        float y{0.f};
        float w{0.f};
        float h{0.f};
    };

    DockingSystem() = default;

    void SetUIRenderer(UIRenderer* renderer) noexcept { m_Renderer = renderer; }
    [[nodiscard]] UIRenderer* GetUIRenderer() const noexcept { return m_Renderer; }
    void SetInputState(const EditorInputState* input) noexcept { m_Input = input; }
    void RegisterPanel(const std::string& name, DrawFn drawFn);

    /// @brief Mark a panel as transparent so the docking system does not draw
    ///        an opaque background behind it.  Use this for the 3-D Viewport
    ///        panel so the OpenGL scene is visible through the chrome.
    void SetPanelTransparent(const std::string& name);

    /// @brief Register an optional overlay drawn inside the title bar of a single
    ///        (non-tabbed) panel.  The callback receives the full title-bar rect and
    ///        is responsible for right-aligning any controls within that region.
    void SetPanelHeaderExtras(const std::string& panelName, HeaderExtrasFn fn);

    /// @brief Create the root split.
    /// @param axis  Split direction (default Horizontal = left|right).
    void SetRootSplit(const std::string& firstPanel,
                      const std::string& secondPanel,
                      float ratio = 0.25f,
                      SplitAxis axis = SplitAxis::Horizontal);

    void SplitPanel(const std::string& parentName,
                    const std::string& newName,
                    SplitAxis axis,
                    float ratio = 0.5f);

    /// @brief Add a panel as a new tab alongside an existing panel.
    ///        Both panels must already be registered.
    void AddTab(const std::string& existingPanel, const std::string& newPanel);

    void Update(float dt);

    /// Build and cache panel content rects for the current root region.
    void BuildLayout(float x, float y, float totalWidth, float totalHeight);

    /// Traverse the tree, draw panel chrome, and invoke each panel draw callback.
    void Draw(float x, float y, float totalWidth, float totalHeight);

    /// Return the last computed content rect for a panel.
    [[nodiscard]] bool GetPanelRect(const std::string& name,
                                    float& x, float& y, float& w, float& h) const noexcept;

private:
    struct PanelEntry {
        std::string name;
        DrawFn      drawFn;
    };

    std::vector<DockNode>    m_Nodes;
    std::vector<PanelEntry>  m_Panels;
    std::vector<PanelRect>   m_LastPanelRects;
    std::vector<std::string> m_TransparentPanels; ///< Panels that skip background drawing.

    struct HeaderExtrasEntry { std::string name; HeaderExtrasFn fn; };
    std::vector<HeaderExtrasEntry> m_HeaderExtras; ///< Optional title-bar overlays per panel.
    uint32_t                 m_NextId{1};
    UIRenderer*              m_Renderer{nullptr};
    const EditorInputState*  m_Input{nullptr};

    uint32_t    AllocNode();
    DockNode*   FindLeaf(const std::string& name);
    DrawFn*     FindDrawFn(const std::string& name);
    void        LayoutNode(const DockNode& node, float x, float y, float w, float h);
    void        DrawNode(DockNode& node, float x, float y, float w, float h);
    void        CachePanelRect(const std::string& name, float x, float y, float w, float h);
};

} // namespace NF::Editor

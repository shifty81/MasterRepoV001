#pragma once
#include "Editor/Application/EditorInputState.h"
#include <string>
#include <vector>

namespace NF { class UIRenderer; }

namespace NF::Editor {

/// @brief Panel for browsing project content assets.
class ContentBrowser {
public:
    /// @brief Set the root directory to browse.
    /// @param path Filesystem path to the content root.
    void SetRootPath(std::string path) { m_RootPath = std::move(path); m_Dirty = true; }

    /// @brief Set the UIRenderer used for drawing.
    void SetUIRenderer(UIRenderer* r) noexcept { m_Renderer = r; }

    /// @brief Provide the current per-frame OS input state.
    /// @param input Non-owning pointer updated by EditorApp each frame.
    void SetInputState(const EditorInputState* input) noexcept { m_Input = input; }

    /// @brief Advance panel state (scans filesystem when dirty).
    void Update(float dt);

    /// @brief Draw the content browser panel within the given region.
    void Draw(float x, float y, float w, float h);

    /// @brief Return the path of the currently selected asset, or empty if none.
    [[nodiscard]] const std::string& GetSelectedAsset() const noexcept { return m_SelectedAsset; }

private:
    /// @brief One entry in the directory listing.
    struct Entry {
        std::string name;
        bool isDirectory{false};
        int depth{0};
    };

    void ScanDirectory();

    std::string             m_RootPath;
    std::string             m_SelectedAsset;
    UIRenderer*             m_Renderer{nullptr};
    const EditorInputState* m_Input{nullptr};
    std::vector<Entry>      m_Entries;
    bool                    m_Dirty{true};     ///< True when directory needs rescan.
    float                   m_RescanTimer{0.f}; ///< Seconds until next periodic rescan.

    static constexpr float kRescanInterval = 5.0f; ///< Seconds between automatic rescans.
};

} // namespace NF::Editor

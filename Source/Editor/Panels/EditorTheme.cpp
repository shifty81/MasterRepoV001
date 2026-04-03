#include "Editor/Panels/EditorTheme.h"

namespace NF::Editor {

static const Theme* s_ActiveTheme = &kDarkTheme;

const Theme& ActiveTheme() noexcept { return *s_ActiveTheme; }

void SetTheme(EditorTheme t) noexcept {
    switch (t) {
    case EditorTheme::Dark:         s_ActiveTheme = &kDarkTheme;         break;
    case EditorTheme::Light:        s_ActiveTheme = &kLightTheme;        break;
    case EditorTheme::HighContrast: s_ActiveTheme = &kHighContrastTheme; break;
    }
}

} // namespace NF::Editor

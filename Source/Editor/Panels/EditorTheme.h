#pragma once
#include <cstdint>

namespace NF::Editor {

/// @brief Editor colour theme selection.
enum class EditorTheme { Dark, Light, HighContrast };

// ---------------------------------------------------------------------------
// Centralised colour palette for the entire editor UI.
//
// Every panel should reference the colours exposed here instead of defining
// local `constexpr uint32_t` constants.  When the active EditorTheme
// changes, call SetTheme() and all panels will pick up the new values on
// the next Draw().
//
// Colour format: 0xRRGGBBAA  (same as UIRenderer).
// ---------------------------------------------------------------------------
struct Theme {
    // -- Backgrounds --------------------------------------------------------
    uint32_t panelBg;          ///< Panel content background.
    uint32_t titleBarBg;       ///< Panel title bar fill.
    uint32_t titleBarAccent;   ///< Thin accent strip at the top of a title bar.
    uint32_t toolbarBg;        ///< Toolbar row background.
    uint32_t modeTabBg;        ///< Mode-tab strip background (Band 2).
    uint32_t shelfBg;          ///< Context tool-shelf background (Band 3).
    uint32_t menuBarBg;        ///< Menu bar row background.
    uint32_t menuOpenBg;       ///< Open menu header highlight.
    uint32_t dropdownBg;       ///< Drop-down panel background.
    uint32_t statusBarBg;      ///< Status bar background.
    uint32_t graphBg;          ///< Node-graph canvas background.
    uint32_t graphGrid;        ///< Node-graph grid lines.
    uint32_t contentShade;     ///< Very-dark overlay inside content area.

    // -- Text ---------------------------------------------------------------
    uint32_t textPrimary;      ///< Main readable text.
    uint32_t textSecondary;    ///< Dimmed / secondary labels.
    uint32_t textHeader;       ///< Section headers & titles.
    uint32_t textTitle;        ///< Panel title bar text.
    uint32_t textValue;        ///< Editable value colour.
    uint32_t textDisabled;     ///< Disabled / greyed-out text.
    uint32_t textConsole;      ///< Console / log text colour.

    // -- Interactive --------------------------------------------------------
    uint32_t hoverBg;          ///< Hovered-row highlight.
    uint32_t selectBg;         ///< Selected-row highlight.
    uint32_t hoverText;        ///< Hovered-value text.
    uint32_t editableText;     ///< Editable field text colour.

    // -- Buttons ------------------------------------------------------------
    uint32_t buttonBg;         ///< Normal button background.
    uint32_t buttonHover;      ///< Hovered button background.
    uint32_t buttonActive;     ///< Active / pressed tool button.
    uint32_t buttonPlay;       ///< Play-mode button background.
    uint32_t buttonStop;       ///< Stop-mode button background.
    uint32_t buttonLaunch;     ///< Launch button background.

    // -- Borders & separators -----------------------------------------------
    uint32_t panelBorder;      ///< Panel outline.
    uint32_t separator;        ///< Horizontal / vertical separators.

    // -- Semantic colours ---------------------------------------------------
    uint32_t dirty;            ///< Unsaved-changes indicator (orange).
    uint32_t worldAccent;      ///< World / selection accent (light blue).
    uint32_t toggleOn;         ///< Toggle-on indicator (green).
    uint32_t toggleOff;        ///< Toggle-off indicator (red).
    uint32_t boolOn;           ///< Boolean ON (preferences-style green).
    uint32_t boolOff;          ///< Boolean OFF (preferences-style grey).

    // -- HUD status colours -------------------------------------------------
    uint32_t healthBar;        ///< Health bar fill colour.
    uint32_t energyBar;        ///< Energy bar fill colour.
    uint32_t itemColor;        ///< Item / gold colour.
    uint32_t readyColor;       ///< Ready status colour.
    uint32_t warnColor;        ///< Warning status colour.
    uint32_t hudBg;            ///< HUD bar track background.
    uint32_t hudText;          ///< HUD primary text.
    uint32_t hudLabel;         ///< HUD secondary label.
    uint32_t hudSep;           ///< HUD separator.
    uint32_t hudGlow;          ///< Subtle glow overlay on HUD.

    // -- Material node graph ------------------------------------------------
    uint32_t nodeBg;           ///< Node body background.
    uint32_t nodeHeader;       ///< Node header bar.
    uint32_t nodeBorder;       ///< Node outline border.
    uint32_t nodeLink;         ///< Connection link line colour.
    uint32_t pinFloat;         ///< Float-pin colour.
    uint32_t pinVec3;          ///< Vec3-pin colour.
    uint32_t pinColor;         ///< Color-pin colour.
    uint32_t pinTexture;       ///< Texture-pin colour.
    uint32_t pinBool;          ///< Bool-pin colour.

    // -- Content browser ----------------------------------------------------
    uint32_t dirColor;         ///< Directory/folder accent colour.
};

// ---------------------------------------------------------------------------
// Built-in theme presets
// ---------------------------------------------------------------------------

/// Dark theme — the default NovaForge editor palette.
inline constexpr Theme kDarkTheme {
    // Backgrounds
    .panelBg        = 0x24262BFF,
    .titleBarBg     = 0x2F343DFF,
    .titleBarAccent = 0x5E89B8FF,
    .toolbarBg      = 0x252527FF,
    .modeTabBg      = 0x2A2D33FF,
    .shelfBg        = 0x30333AFF,
    .menuBarBg      = 0x2D2D30FF,
    .menuOpenBg     = 0x3E3E42FF,
    .dropdownBg     = 0x1E1E1EFF,
    .statusBarBg    = 0x1E1E2EFF,
    .graphBg        = 0x1A1A1AFF,
    .graphGrid      = 0x252525FF,
    .contentShade   = 0x1A1C20AA,

    // Text
    .textPrimary    = 0xB0B0B0FF,
    .textSecondary  = 0x808080FF,
    .textHeader     = 0xCCCCCCFF,
    .textTitle      = 0xD7DCE3FF,
    .textValue      = 0xD0D0D0FF,
    .textDisabled   = 0x555555FF,
    .textConsole    = 0x9CDCFEFF,

    // Interactive
    .hoverBg        = 0x3F3F50FF,
    .selectBg       = 0x264F78FF,
    .hoverText      = 0x90E0FFFF,
    .editableText   = 0x78C8FFFF,

    // Buttons
    .buttonBg       = 0x3C3C3CFF,
    .buttonHover    = 0x505053FF,
    .buttonActive   = 0x3B6EA5FF,
    .buttonPlay     = 0x2D6A2DFF,
    .buttonStop     = 0x6A2D2DFF,
    .buttonLaunch   = 0x1E4B7AFF,

    // Borders & separators
    .panelBorder    = 0x4B5563FF,
    .separator      = 0x444444FF,

    // Semantic
    .dirty          = 0xFFAA44FF,
    .worldAccent    = 0x78C8FFFF,
    .toggleOn       = 0x3B8A3BFF,
    .toggleOff      = 0x8A3B3BFF,
    .boolOn         = 0x44CC66FF,
    .boolOff        = 0x666666FF,

    // HUD
    .healthBar      = 0x4FCB74FF,
    .energyBar      = 0x4FA3FFFF,
    .itemColor      = 0xE2B36AFF,
    .readyColor     = 0x58C27DFF,
    .warnColor      = 0xE0A84EFF,
    .hudBg          = 0x1F2329FF,
    .hudText        = 0xD5DCE4FF,
    .hudLabel       = 0x93A1B2FF,
    .hudSep         = 0x3F4A57FF,
    .hudGlow        = 0xFFFFFF08,

    // Material node graph
    .nodeBg         = 0x2D2D30FF,
    .nodeHeader     = 0x3B6EA5FF,
    .nodeBorder     = 0x555555FF,
    .nodeLink       = 0x888888FF,
    .pinFloat       = 0x88CC88FF,
    .pinVec3        = 0xCC88CCFF,
    .pinColor       = 0xFFCC44FF,
    .pinTexture     = 0x4488CCFF,
    .pinBool        = 0xCC4444FF,

    // Content browser
    .dirColor       = 0xD0B870FF,
};

/// Light theme — higher-contrast light-background palette.
inline constexpr Theme kLightTheme {
    // Backgrounds
    .panelBg        = 0xF0F0F0FF,
    .titleBarBg     = 0xE0E0E4FF,
    .titleBarAccent = 0x3574B0FF,
    .toolbarBg      = 0xE8E8E8FF,
    .modeTabBg      = 0xDDDDE0FF,
    .shelfBg        = 0xD4D4D8FF,
    .menuBarBg      = 0xF2F2F2FF,
    .menuOpenBg     = 0xCCDDEEFF,
    .dropdownBg     = 0xFFFFFFFF,
    .statusBarBg    = 0xE6E8EEFF,
    .graphBg        = 0xF8F8F8FF,
    .graphGrid      = 0xE0E0E0FF,
    .contentShade   = 0xE8E8E8AA,

    // Text
    .textPrimary    = 0x1E1E1EFF,
    .textSecondary  = 0x606060FF,
    .textHeader     = 0x111111FF,
    .textTitle      = 0x1A1A1AFF,
    .textValue      = 0x333333FF,
    .textDisabled   = 0xAAAAAAFF,
    .textConsole    = 0x006080FF,

    // Interactive
    .hoverBg        = 0xD0D4DDFF,
    .selectBg       = 0xA4C8E8FF,
    .hoverText      = 0x005F8CFF,
    .editableText   = 0x0070A8FF,

    // Buttons
    .buttonBg       = 0xD4D4D4FF,
    .buttonHover    = 0xBEBEC0FF,
    .buttonActive   = 0x3574B0FF,
    .buttonPlay     = 0x328832FF,
    .buttonStop     = 0xB03030FF,
    .buttonLaunch   = 0x1E5A96FF,

    // Borders & separators
    .panelBorder    = 0xB0B0B0FF,
    .separator      = 0xC0C0C0FF,

    // Semantic
    .dirty          = 0xE08820FF,
    .worldAccent    = 0x2080C0FF,
    .toggleOn       = 0x2E8A2EFF,
    .toggleOff      = 0xC03030FF,
    .boolOn         = 0x2E9E44FF,
    .boolOff        = 0x999999FF,

    // HUD
    .healthBar      = 0x38B05AFF,
    .energyBar      = 0x3080D0FF,
    .itemColor      = 0xC09040FF,
    .readyColor     = 0x30A050FF,
    .warnColor      = 0xD09030FF,
    .hudBg          = 0xE4E4E8FF,
    .hudText        = 0x1A1A1AFF,
    .hudLabel       = 0x4A5A6AFF,
    .hudSep         = 0xC0C0C0FF,
    .hudGlow        = 0x00000008,

    // Material node graph
    .nodeBg         = 0xEAEAECFF,
    .nodeHeader     = 0x3574B0FF,
    .nodeBorder     = 0x999999FF,
    .nodeLink       = 0x888888FF,
    .pinFloat       = 0x40A040FF,
    .pinVec3        = 0x9040A0FF,
    .pinColor       = 0xC09020FF,
    .pinTexture     = 0x2060A0FF,
    .pinBool        = 0xB03030FF,

    // Content browser
    .dirColor       = 0x907830FF,
};

/// High-contrast theme — maximised contrast for accessibility.
inline constexpr Theme kHighContrastTheme {
    // Backgrounds
    .panelBg        = 0x000000FF,
    .titleBarBg     = 0x1A1A1AFF,
    .titleBarAccent = 0xFFFF00FF,
    .toolbarBg      = 0x0A0A0AFF,
    .modeTabBg      = 0x141414FF,
    .shelfBg        = 0x1A1A1AFF,
    .menuBarBg      = 0x111111FF,
    .menuOpenBg     = 0x333333FF,
    .dropdownBg     = 0x0A0A0AFF,
    .statusBarBg    = 0x0A0A0AFF,
    .graphBg        = 0x000000FF,
    .graphGrid      = 0x1A1A1AFF,
    .contentShade   = 0x000000AA,

    // Text
    .textPrimary    = 0xFFFFFFFF,
    .textSecondary  = 0xCCCCCCFF,
    .textHeader     = 0xFFFF00FF,
    .textTitle      = 0xFFFFFFFF,
    .textValue      = 0xFFFFFFFF,
    .textDisabled   = 0x888888FF,
    .textConsole    = 0x00FF00FF,

    // Interactive
    .hoverBg        = 0x444444FF,
    .selectBg       = 0x0060C0FF,
    .hoverText      = 0x00FFFFFF,
    .editableText   = 0x00FFFFFF,

    // Buttons
    .buttonBg       = 0x333333FF,
    .buttonHover    = 0x555555FF,
    .buttonActive   = 0x0060C0FF,
    .buttonPlay     = 0x00AA00FF,
    .buttonStop     = 0xFF0000FF,
    .buttonLaunch   = 0x0060C0FF,

    // Borders & separators
    .panelBorder    = 0xFFFFFFFF,
    .separator      = 0xAAAAAAFF,

    // Semantic
    .dirty          = 0xFFAA00FF,
    .worldAccent    = 0x00FFFFFF,
    .toggleOn       = 0x00FF00FF,
    .toggleOff      = 0xFF0000FF,
    .boolOn         = 0x00FF00FF,
    .boolOff        = 0xAAAAAAFF,

    // HUD
    .healthBar      = 0x00FF00FF,
    .energyBar      = 0x00AAFFFF,
    .itemColor      = 0xFFCC00FF,
    .readyColor     = 0x00FF00FF,
    .warnColor      = 0xFFAA00FF,
    .hudBg          = 0x111111FF,
    .hudText        = 0xFFFFFFFF,
    .hudLabel       = 0xCCCCCCFF,
    .hudSep         = 0xAAAAAAFF,
    .hudGlow        = 0xFFFFFF10,

    // Material node graph
    .nodeBg         = 0x1A1A1AFF,
    .nodeHeader     = 0x0060C0FF,
    .nodeBorder     = 0xFFFFFFFF,
    .nodeLink       = 0xCCCCCCFF,
    .pinFloat       = 0x00FF00FF,
    .pinVec3        = 0xFF00FFFF,
    .pinColor       = 0xFFFF00FF,
    .pinTexture     = 0x00AAFFFF,
    .pinBool        = 0xFF0000FF,

    // Content browser
    .dirColor       = 0xFFCC00FF,
};

// ---------------------------------------------------------------------------
// Active-theme accessor — one global instance, switchable at runtime.
// ---------------------------------------------------------------------------

/// Returns the currently active theme.
const Theme& ActiveTheme() noexcept;

/// Switch to one of the built-in presets.
void SetTheme(EditorTheme t) noexcept;

} // namespace NF::Editor

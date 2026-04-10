#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace NF::Editor {

// ---------------------------------------------------------------------------
// Input binding domains
//
// Separates keybindings into three logical areas so overlapping keys can be
// context-disambiguated at runtime:
//
//   EditorGlobal       — Always active (e.g. Ctrl+S save, Ctrl+Z undo).
//   ViewportNavigation — Active only while RMB is held inside the viewport.
//   PieGameplay        — Active only while PIE is in the Playing state.
//
// v1 provides read-only default bindings.  A future version may add an
// interactive rebinding UI; the structure is designed to allow that.
// ---------------------------------------------------------------------------

enum class InputBindingDomain : uint8_t {
    EditorGlobal,       ///< Editor-wide shortcuts (always active).
    ViewportNavigation, ///< Viewport fly-cam navigation (RMB + inside viewport).
    PieGameplay,        ///< In-game input when PIE is playing.
};

/// @brief A single key binding entry.
struct InputBinding {
    InputBindingDomain domain;   ///< Which domain this binding belongs to.
    std::string        action;   ///< Logical action name (e.g. "MoveForward").
    uint8_t            vkCode;   ///< Win32 Virtual-Key code.
    bool               needCtrl  {false}; ///< Requires Ctrl held.
    bool               needShift {false}; ///< Requires Shift held.
    bool               needAlt   {false}; ///< Requires Alt held.
};

// ---------------------------------------------------------------------------
// Default binding table
// ---------------------------------------------------------------------------

/// @brief Returns the built-in default input binding table.
///
/// The returned vector is the authoritative source for all default keybinds.
/// Panels that display keybind information should iterate this table.
inline std::vector<InputBinding> DefaultInputBindings()
{
    using D = InputBindingDomain;
    return {
        // ---- EditorGlobal -------------------------------------------------
        { D::EditorGlobal, "Save",           'S',  true,  false, false },
        { D::EditorGlobal, "Undo",           'Z',  true,  false, false },
        { D::EditorGlobal, "Redo",           'Z',  true,  true,  false },
        { D::EditorGlobal, "OpenPrefs",      0xBC, true,  false, false }, // Ctrl+,
        { D::EditorGlobal, "ToggleConsole",  0xC0, false, false, false }, // `
        { D::EditorGlobal, "FocusViewport",  'F',  false, false, false },

        // ---- ViewportNavigation -------------------------------------------
        // (All require RMB held + mouse inside viewport — enforced in code.)
        { D::ViewportNavigation, "FlyForward",  'W',  false, false, false },
        { D::ViewportNavigation, "FlyBackward", 'S',  false, false, false },
        { D::ViewportNavigation, "FlyRight",    'D',  false, false, false },
        { D::ViewportNavigation, "FlyLeft",     'A',  false, false, false },
        { D::ViewportNavigation, "FlyUp",       'E',  false, false, false },
        { D::ViewportNavigation, "FlyDown",     'Q',  false, false, false },
        { D::ViewportNavigation, "FastFly",     0x10, false, false, false }, // Shift

        // ---- PieGameplay --------------------------------------------------
        // (Active only when PIE is in the Playing state.)
        { D::PieGameplay, "MoveForward",  'W',    false, false, false },
        { D::PieGameplay, "MoveBackward", 'S',    false, false, false },
        { D::PieGameplay, "MoveRight",    'D',    false, false, false },
        { D::PieGameplay, "MoveLeft",     'A',    false, false, false },
        { D::PieGameplay, "Jump",         0x20,   false, false, false }, // Space
        { D::PieGameplay, "Sprint",       0x10,   false, false, false }, // Shift
        { D::PieGameplay, "StopPIE",      0x1B,   false, false, false }, // Escape
    };
}

} // namespace NF::Editor

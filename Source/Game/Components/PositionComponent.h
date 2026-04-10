#pragma once
#include "Core/Math/Vector.h"

namespace NF::Game {

/// @brief World-space position component attached to an entity.
///
/// Any entity that has a physical location in the world carries this
/// component.  The editor TransformGizmo reads and writes this component
/// to move entities interactively in the viewport.
struct PositionComponent {
    NF::Vector3 position{};
};

} // namespace NF::Game

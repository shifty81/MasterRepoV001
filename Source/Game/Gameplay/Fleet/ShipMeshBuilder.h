#pragma once
#include "Renderer/Mesh/MeshRenderer.h"
#include "Core/Math/Vector.h"

namespace NF::Game::Gameplay {

// ---------------------------------------------------------------------------
// ShipColor — palette indices used by the ship shader.
// ---------------------------------------------------------------------------

enum class ShipColor : int {
    Hull    = 0,  // dark metallic grey
    Cockpit = 1,  // dark tinted cockpit glass (deep blue-grey)
    Wing    = 2,  // medium grey panels
    Engine  = 3,  // orange-red engine nozzle
    Exhaust = 4,  // bright yellow-white engine core
    Trim    = 5,  // white-grey accent stripe / hull markings
};

/// @brief World-space RGB for a ShipColor palette entry (tools/previews).
inline NF::Vector3 ShipColorRGB(ShipColor c) noexcept
{
    switch (c)
    {
    case ShipColor::Hull:    return {0.28f, 0.28f, 0.30f};
    case ShipColor::Cockpit: return {0.10f, 0.14f, 0.22f};
    case ShipColor::Wing:    return {0.35f, 0.35f, 0.37f};
    case ShipColor::Engine:  return {0.85f, 0.35f, 0.05f};
    case ShipColor::Exhaust: return {1.00f, 0.85f, 0.40f};
    case ShipColor::Trim:    return {0.70f, 0.72f, 0.75f};
    default:                 return {1.0f,  0.0f,  1.0f};
    }
}

// ---------------------------------------------------------------------------
// ShipMeshBuilder
//
// Builds CPU-side MeshData for a simple low-poly spacecraft.
//
// Coordinate convention (matches character builder):
//   Origin = ship centre of mass.
//   +Y = up, +X = right, -Z = forward (nose points toward -Z).
//
// Total length ≈ 4.2 m (-2.1 to +2.1 on Z axis).
// Wingspan ≈ 6.0 m.
// ---------------------------------------------------------------------------

class ShipMeshBuilder
{
public:
    /// @brief Build a single combined MeshData for the whole ship.
    [[nodiscard]] MeshData BuildFullShip() const;

    // Individual part builders (for tools / LOD generation).
    [[nodiscard]] MeshData BuildHull()         const;
    [[nodiscard]] MeshData BuildCockpit()      const;
    [[nodiscard]] MeshData BuildWings()        const;
    [[nodiscard]] MeshData BuildEnginePods()   const;
    [[nodiscard]] MeshData BuildEngineGlow()   const;
    [[nodiscard]] MeshData BuildTrimStripes()  const;

private:
    // Append one AABB box (6 faces, 12 triangles) to an existing MeshData.
    static void AppendBox(MeshData& out,
                          NF::Vector3 min,
                          NF::Vector3 max,
                          ShipColor color);
};

} // namespace NF::Game::Gameplay

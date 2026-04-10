#include "Game/Gameplay/Fleet/ShipMeshBuilder.h"

namespace NF::Game::Gameplay {

// ---------------------------------------------------------------------------
// AppendBox — emit 6 faces (12 triangles) for one AABB.
// Same winding convention as CharacterMeshBuilder::AppendBox.
// ---------------------------------------------------------------------------

void ShipMeshBuilder::AppendBox(MeshData& out,
                                 NF::Vector3 min,
                                 NF::Vector3 max,
                                 ShipColor color)
{
    const float cid = static_cast<float>(static_cast<int>(color));

    auto addFace = [&](NF::Vector3 v0, NF::Vector3 v1,
                       NF::Vector3 v2, NF::Vector3 v3,
                       NF::Vector3 normal)
    {
        const uint32_t i = static_cast<uint32_t>(out.Vertices.size());
        out.Vertices.push_back({v0, normal, {cid, 0.f}});
        out.Vertices.push_back({v1, normal, {cid, 0.f}});
        out.Vertices.push_back({v2, normal, {cid, 0.f}});
        out.Vertices.push_back({v3, normal, {cid, 0.f}});
        out.Indices.insert(out.Indices.end(), {i, i+1, i+2, i+2, i+3, i});
    };

    const float x0 = min.X, x1 = max.X;
    const float y0 = min.Y, y1 = max.Y;
    const float z0 = min.Z, z1 = max.Z;

    addFace({x0,y0,z1}, {x0,y1,z1}, {x0,y1,z0}, {x0,y0,z0}, {-1, 0, 0}); // -X
    addFace({x1,y0,z0}, {x1,y1,z0}, {x1,y1,z1}, {x1,y0,z1}, { 1, 0, 0}); // +X
    addFace({x0,y0,z0}, {x1,y0,z0}, {x1,y0,z1}, {x0,y0,z1}, { 0,-1, 0}); // -Y
    addFace({x0,y1,z1}, {x1,y1,z1}, {x1,y1,z0}, {x0,y1,z0}, { 0, 1, 0}); // +Y
    addFace({x1,y0,z0}, {x0,y0,z0}, {x0,y1,z0}, {x1,y1,z0}, { 0, 0,-1}); // -Z
    addFace({x0,y0,z1}, {x1,y0,z1}, {x1,y1,z1}, {x0,y1,z1}, { 0, 0, 1}); // +Z
}

// ---------------------------------------------------------------------------
// Hull — main elongated body, nose slightly narrowed at -Z
// ---------------------------------------------------------------------------

MeshData ShipMeshBuilder::BuildHull() const
{
    MeshData md;
    // Main body section
    AppendBox(md, {-0.30f, 0.00f, -1.80f}, { 0.30f, 0.28f,  1.20f}, ShipColor::Hull);
    // Nose section (narrower for a tapered look)
    AppendBox(md, {-0.18f, 0.02f, -2.10f}, { 0.18f, 0.24f, -1.80f}, ShipColor::Hull);
    // Tail section (slightly wider, connects to engine pods)
    AppendBox(md, {-0.32f, 0.00f,  1.20f}, { 0.32f, 0.26f,  1.60f}, ShipColor::Hull);
    return md;
}

// ---------------------------------------------------------------------------
// Cockpit — raised canopy over the forward hull section
// ---------------------------------------------------------------------------

MeshData ShipMeshBuilder::BuildCockpit() const
{
    MeshData md;
    // Canopy frame
    AppendBox(md, {-0.22f, 0.28f, -1.40f}, { 0.22f, 0.50f, -0.10f}, ShipColor::Cockpit);
    // Canopy rim strip (trim color)
    AppendBox(md, {-0.23f, 0.26f, -1.42f}, { 0.23f, 0.30f,  0.00f}, ShipColor::Trim);
    return md;
}

// ---------------------------------------------------------------------------
// Wings — swept delta wings, one per side
// ---------------------------------------------------------------------------

MeshData ShipMeshBuilder::BuildWings() const
{
    MeshData md;
    // Right wing — swept and flat
    AppendBox(md, { 0.30f, 0.02f, -0.50f}, { 1.80f, 0.10f,  0.80f}, ShipColor::Wing);
    // Right wing leading edge accent
    AppendBox(md, { 0.30f, 0.08f, -0.52f}, { 1.50f, 0.12f, -0.48f}, ShipColor::Trim);
    // Left wing (mirror of right)
    AppendBox(md, {-1.80f, 0.02f, -0.50f}, {-0.30f, 0.10f,  0.80f}, ShipColor::Wing);
    // Left wing leading edge accent
    AppendBox(md, {-1.50f, 0.08f, -0.52f}, {-0.30f, 0.12f, -0.48f}, ShipColor::Trim);
    return md;
}

// ---------------------------------------------------------------------------
// Engine pods — mounted on wing tips
// ---------------------------------------------------------------------------

MeshData ShipMeshBuilder::BuildEnginePods() const
{
    MeshData md;
    // Right engine pod (bottom of wing tip)
    AppendBox(md, { 1.30f, -0.14f,  0.10f}, { 1.70f, 0.08f,  1.30f}, ShipColor::Hull);
    // Right engine nozzle ring
    AppendBox(md, { 1.35f, -0.10f,  1.25f}, { 1.65f, 0.05f,  1.35f}, ShipColor::Engine);
    // Left engine pod
    AppendBox(md, {-1.70f, -0.14f,  0.10f}, {-1.30f, 0.08f,  1.30f}, ShipColor::Hull);
    // Left engine nozzle ring
    AppendBox(md, {-1.65f, -0.10f,  1.25f}, {-1.35f, 0.05f,  1.35f}, ShipColor::Engine);
    // Centre engine pod (integrated into hull tail)
    AppendBox(md, {-0.16f, -0.06f,  1.20f}, { 0.16f, 0.18f,  1.70f}, ShipColor::Hull);
    // Centre nozzle
    AppendBox(md, {-0.14f, -0.04f,  1.68f}, { 0.14f, 0.16f,  1.78f}, ShipColor::Engine);
    return md;
}

// ---------------------------------------------------------------------------
// Engine glow — bright exhaust inside nozzles
// ---------------------------------------------------------------------------

MeshData ShipMeshBuilder::BuildEngineGlow() const
{
    MeshData md;
    // Right exhaust core
    AppendBox(md, { 1.40f, -0.06f,  1.30f}, { 1.60f, 0.00f,  1.38f}, ShipColor::Exhaust);
    // Left exhaust core
    AppendBox(md, {-1.60f, -0.06f,  1.30f}, {-1.40f, 0.00f,  1.38f}, ShipColor::Exhaust);
    // Centre exhaust core
    AppendBox(md, {-0.10f,  0.01f,  1.74f}, { 0.10f, 0.12f,  1.82f}, ShipColor::Exhaust);
    return md;
}

// ---------------------------------------------------------------------------
// Trim stripes — hull markings / accent lines
// ---------------------------------------------------------------------------

MeshData ShipMeshBuilder::BuildTrimStripes() const
{
    MeshData md;
    // Side accent stripe along the hull
    AppendBox(md, {-0.305f, 0.12f, -1.80f}, {-0.295f, 0.18f, 1.20f}, ShipColor::Trim);
    AppendBox(md, { 0.295f, 0.12f, -1.80f}, { 0.305f, 0.18f, 1.20f}, ShipColor::Trim);
    return md;
}

// ---------------------------------------------------------------------------
// BuildFullShip — merge all parts
// ---------------------------------------------------------------------------

MeshData ShipMeshBuilder::BuildFullShip() const
{
    MeshData md;

    auto merge = [&](MeshData part)
    {
        const uint32_t indexOffset = static_cast<uint32_t>(md.Vertices.size());
        for (auto& v : part.Vertices)
            md.Vertices.push_back(v);
        for (auto idx : part.Indices)
            md.Indices.push_back(idx + indexOffset);
    };

    merge(BuildHull());
    merge(BuildCockpit());
    merge(BuildWings());
    merge(BuildEnginePods());
    merge(BuildEngineGlow());
    merge(BuildTrimStripes());

    return md;
}

} // namespace NF::Game::Gameplay

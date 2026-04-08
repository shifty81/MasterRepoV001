#pragma once
// DevSolarSystem.h — Dev-mode solar system with seed-based PCG generation.
//
// Defines the celestial body hierarchy (star, planets, moons) and a
// procedural generator that populates the system from a seed value.
// Every body's properties are mutable so the editor can override PCG output.

#include "Game/Interaction/ResourceItem.h"
#include <cstdint>
#include <string>
#include <vector>

namespace NF::Game::Gameplay {

// ---------------------------------------------------------------------------
// Celestial body type
// ---------------------------------------------------------------------------

enum class CelestialBodyType : uint8_t {
    Star    = 0,
    Planet  = 1,
    Moon    = 2,
};

/// @brief Human-readable name for a body type.
[[nodiscard]] inline const char* CelestialBodyTypeName(CelestialBodyType t) noexcept {
    switch (t) {
        case CelestialBodyType::Star:   return "Star";
        case CelestialBodyType::Planet: return "Planet";
        case CelestialBodyType::Moon:   return "Moon";
        default:                        return "Unknown";
    }
}

// ---------------------------------------------------------------------------
// Resource deposit — a mineable resource present on a body's surface.
// ---------------------------------------------------------------------------

struct ResourceDeposit {
    NF::Game::ResourceType type{NF::Game::ResourceType::None};
    float                  abundance{0.f};   ///< 0..1 relative richness.
    float                  worldX{0.f};      ///< Flat-map X position (local to body).
    float                  worldZ{0.f};      ///< Flat-map Z position (local to body).
};

// ---------------------------------------------------------------------------
// CelestialBody — a single object in the solar system.
// ---------------------------------------------------------------------------

struct CelestialBody {
    uint32_t            id{0};               ///< Unique ID within the system.
    std::string         name;                ///< Display name (e.g. "Kepler-4b").
    CelestialBodyType   type{CelestialBodyType::Planet};

    // Orbital parameters (relative to parent; star orbits origin at radius 0).
    float               orbitRadius{0.f};    ///< AU from parent.
    float               orbitalPeriod{1.f};  ///< Orbital period in earth-years.
    float               orbitAngle{0.f};     ///< Current angle in radians.

    // Physical parameters
    float               mass{1.f};           ///< Earth masses.
    float               radius{1.f};         ///< Earth radii.
    float               temperature{288.f};  ///< Surface temperature in Kelvin.

    // Visual
    uint32_t            color{0xCCCCCCFF};   ///< 0xRRGGBBAA display colour.

    // Hierarchy
    uint32_t            parentId{0};         ///< 0 = root (star).

    // Resources present on this body.
    std::vector<ResourceDeposit> deposits;

    /// @brief Compute the flat-map X position from orbital parameters.
    [[nodiscard]] float FlatX() const noexcept;
    /// @brief Compute the flat-map Y position from orbital parameters.
    [[nodiscard]] float FlatY() const noexcept;
};

// ---------------------------------------------------------------------------
// DevSolarSystem — owns the full body list and PCG generator.
// ---------------------------------------------------------------------------

class DevSolarSystem {
public:
    DevSolarSystem() = default;

    /// @brief Set the generation seed.  Call before Generate().
    void SetSeed(uint32_t seed) noexcept { m_Seed = seed; }
    [[nodiscard]] uint32_t GetSeed() const noexcept { return m_Seed; }

    /// @brief Procedurally generate the solar system from the current seed.
    ///        Clears any existing bodies.
    void Generate();

    /// @brief Save the solar system to a JSON file.
    /// @param path File path for the save file.
    /// @return True on success.
    bool SaveToFile(const std::string& path) const;

    /// @brief Load a previously saved solar system from a JSON file.
    ///        Replaces any existing bodies.
    /// @param path File path for the save file.
    /// @return True on success.
    bool LoadFromFile(const std::string& path);

    /// @brief Return all bodies (star + planets + moons).
    [[nodiscard]] const std::vector<CelestialBody>& GetBodies() const noexcept { return m_Bodies; }
    [[nodiscard]] std::vector<CelestialBody>& GetBodies() noexcept { return m_Bodies; }

    /// @brief Find a body by ID.  Returns nullptr if not found.
    [[nodiscard]] const CelestialBody* FindBody(uint32_t id) const noexcept;
    [[nodiscard]] CelestialBody* FindBody(uint32_t id) noexcept;

    /// @brief Number of bodies in the system.
    [[nodiscard]] size_t BodyCount() const noexcept { return m_Bodies.size(); }

    /// @brief Number of planets (not counting star or moons).
    [[nodiscard]] size_t PlanetCount() const noexcept;

    /// @brief Regenerate resource deposits on all bodies using the current seed.
    void RegenerateDeposits();

private:
    uint32_t                  m_Seed{42};
    std::vector<CelestialBody> m_Bodies;
    uint32_t                  m_NextId{1};

    /// @brief Simple deterministic hash for PCG.
    [[nodiscard]] static uint32_t Hash(uint32_t x) noexcept;

    /// @brief Generate deposits for a single body.
    void GenerateDepositsForBody(CelestialBody& body, uint32_t bodySeed) const;
};

} // namespace NF::Game::Gameplay

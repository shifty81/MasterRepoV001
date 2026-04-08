// DevSolarSystem.cpp — Seed-based procedural solar system generator.
//
// Generates a star, 3–7 planets, and 0–3 moons per planet from a single
// seed value.  Resource deposits are scattered per body using a secondary
// hash derived from the body's ID and the system seed.

#include "Game/Gameplay/SolarSystem/DevSolarSystem.h"
#include <cmath>
#include <algorithm>

namespace NF::Game::Gameplay {

// ---------------------------------------------------------------------------
// CelestialBody helpers
// ---------------------------------------------------------------------------

float CelestialBody::FlatX() const noexcept {
    return orbitRadius * std::cos(orbitAngle);
}

float CelestialBody::FlatY() const noexcept {
    return orbitRadius * std::sin(orbitAngle);
}

// ---------------------------------------------------------------------------
// DevSolarSystem — PCG
// ---------------------------------------------------------------------------

uint32_t DevSolarSystem::Hash(uint32_t x) noexcept {
    // Wang hash — fast, decent distribution.
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
}

void DevSolarSystem::Generate() {
    m_Bodies.clear();
    m_NextId = 1;

    // ---- Star (always ID 1) ------------------------------------------------
    CelestialBody star;
    star.id          = m_NextId++;
    star.name        = "Sol-" + std::to_string(m_Seed % 10000);
    star.type        = CelestialBodyType::Star;
    star.orbitRadius = 0.f;
    star.orbitAngle  = 0.f;
    star.mass        = 1.0f + static_cast<float>(Hash(m_Seed) % 100) * 0.01f;
    star.radius      = 109.f;          // ~109 earth radii
    star.temperature = 5500.f + static_cast<float>(Hash(m_Seed + 1) % 2000);
    star.color       = 0xFFDD44FF;     // yellow
    star.parentId    = 0;

    m_Bodies.push_back(std::move(star));

    // ---- Planets -----------------------------------------------------------
    const uint32_t numPlanets = 3 + (Hash(m_Seed + 2) % 5);  // 3–7 planets

    static const char* kPlanetNames[] = {
        "Hermes", "Aphrodite", "Gaia", "Ares",
        "Zeus", "Kronos", "Ouranos", "Poseidon",
        "Hades", "Demeter"
    };
    static const uint32_t kPlanetColors[] = {
        0xBBAA88FF, 0xEECC77FF, 0x4488CCFF, 0xCC6644FF,
        0xDDCC88FF, 0xCCBB99FF, 0x88BBEEFF, 0x4466CCFF,
        0x886666FF, 0x66AA66FF
    };

    for (uint32_t i = 0; i < numPlanets; ++i) {
        CelestialBody planet;
        planet.id          = m_NextId++;
        planet.name        = kPlanetNames[i % 10];
        planet.type        = CelestialBodyType::Planet;
        planet.orbitRadius = 0.4f + static_cast<float>(i) * 1.2f
                           + static_cast<float>(Hash(m_Seed + 10 + i) % 100) * 0.005f;
        planet.orbitalPeriod = std::pow(planet.orbitRadius, 1.5f);  // Kepler's 3rd law approx
        planet.orbitAngle  = static_cast<float>(Hash(m_Seed + 20 + i) % 628) * 0.01f;
        planet.mass        = 0.1f + static_cast<float>(Hash(m_Seed + 30 + i) % 500) * 0.02f;
        planet.radius      = 0.3f + static_cast<float>(Hash(m_Seed + 40 + i) % 300) * 0.01f;
        planet.temperature = 100.f + static_cast<float>(Hash(m_Seed + 50 + i) % 400);
        planet.color       = kPlanetColors[i % 10];
        planet.parentId    = 1; // star

        GenerateDepositsForBody(planet, Hash(m_Seed + 100 + i));

        const uint32_t planetId = planet.id;
        m_Bodies.push_back(std::move(planet));

        // ---- Moons for this planet ----------------------------------------
        const uint32_t numMoons = Hash(m_Seed + 60 + i) % 4; // 0–3
        for (uint32_t mi = 0; mi < numMoons; ++mi) {
            CelestialBody moon;
            moon.id          = m_NextId++;
            moon.name        = std::string(kPlanetNames[i % 10]) + "-" + std::to_string(mi + 1);
            moon.type        = CelestialBodyType::Moon;
            moon.orbitRadius = 0.05f + static_cast<float>(mi) * 0.04f
                             + static_cast<float>(Hash(m_Seed + 70 + i * 10 + mi) % 30) * 0.001f;
            moon.orbitalPeriod = 0.01f + static_cast<float>(mi) * 0.005f;
            moon.orbitAngle  = static_cast<float>(Hash(m_Seed + 80 + i * 10 + mi) % 628) * 0.01f;
            moon.mass        = 0.001f + static_cast<float>(Hash(m_Seed + 90 + i * 10 + mi) % 50) * 0.0002f;
            moon.radius      = 0.05f + static_cast<float>(Hash(m_Seed + 95 + i * 10 + mi) % 100) * 0.002f;
            moon.temperature = 50.f + static_cast<float>(Hash(m_Seed + 98 + i * 10 + mi) % 200);
            moon.color       = 0xAAAAAAFF;
            moon.parentId    = planetId;

            GenerateDepositsForBody(moon, Hash(m_Seed + 200 + i * 10 + mi));

            m_Bodies.push_back(std::move(moon));
        }
    }
}

void DevSolarSystem::GenerateDepositsForBody(CelestialBody& body, uint32_t bodySeed) const {
    body.deposits.clear();

    // Stars don't have mineable deposits.
    if (body.type == CelestialBodyType::Star) return;

    // Number of deposits: 1–5 for planets, 0–2 for moons.
    const uint32_t maxDeposits = (body.type == CelestialBodyType::Planet) ? 5 : 2;
    const uint32_t numDeposits = 1 + Hash(bodySeed) % maxDeposits;

    for (uint32_t d = 0; d < numDeposits; ++d) {
        ResourceDeposit dep;
        // Pick a resource type (1–7, skipping None).
        dep.type      = static_cast<NF::Game::ResourceType>(1 + Hash(bodySeed + d * 3 + 1) % 7);
        dep.abundance = 0.1f + static_cast<float>(Hash(bodySeed + d * 3 + 2) % 90) * 0.01f;
        // Scatter positions on a flat 100×100 tile for the body.
        dep.worldX    = static_cast<float>(Hash(bodySeed + d * 3 + 3) % 1000) * 0.1f - 50.f;
        dep.worldZ    = static_cast<float>(Hash(bodySeed + d * 3 + 4) % 1000) * 0.1f - 50.f;
        body.deposits.push_back(dep);
    }
}

const CelestialBody* DevSolarSystem::FindBody(uint32_t id) const noexcept {
    for (const auto& b : m_Bodies)
        if (b.id == id) return &b;
    return nullptr;
}

CelestialBody* DevSolarSystem::FindBody(uint32_t id) noexcept {
    for (auto& b : m_Bodies)
        if (b.id == id) return &b;
    return nullptr;
}

size_t DevSolarSystem::PlanetCount() const noexcept {
    size_t count = 0;
    for (const auto& b : m_Bodies)
        if (b.type == CelestialBodyType::Planet) ++count;
    return count;
}

void DevSolarSystem::RegenerateDeposits() {
    for (size_t i = 0; i < m_Bodies.size(); ++i) {
        GenerateDepositsForBody(m_Bodies[i], Hash(m_Seed + 100 + static_cast<uint32_t>(i)));
    }
}

} // namespace NF::Game::Gameplay

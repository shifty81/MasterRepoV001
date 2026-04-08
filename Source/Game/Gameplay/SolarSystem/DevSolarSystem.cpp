// DevSolarSystem.cpp — Seed-based procedural solar system generator.
//
// Generates a star, 3–7 planets, and 0–3 moons per planet from a single
// seed value.  Resource deposits are scattered per body using a secondary
// hash derived from the body's ID and the system seed.

#include "Game/Gameplay/SolarSystem/DevSolarSystem.h"
#include <cmath>
#include <algorithm>
#include <fstream>
#include <sstream>

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

// ---------------------------------------------------------------------------
// SaveToFile — write solar system state to a simple JSON file
// ---------------------------------------------------------------------------

bool DevSolarSystem::SaveToFile(const std::string& path) const {
    std::ofstream file(path);
    if (!file.is_open()) return false;

    file << "{\n  \"seed\": " << m_Seed << ",\n  \"bodies\": [\n";
    for (size_t i = 0; i < m_Bodies.size(); ++i) {
        const auto& b = m_Bodies[i];
        file << "    {\n";
        file << "      \"id\": " << b.id << ",\n";
        file << "      \"name\": \"" << b.name << "\",\n";
        file << "      \"type\": " << static_cast<int>(b.type) << ",\n";
        file << "      \"orbitRadius\": " << b.orbitRadius << ",\n";
        file << "      \"orbitalPeriod\": " << b.orbitalPeriod << ",\n";
        file << "      \"orbitAngle\": " << b.orbitAngle << ",\n";
        file << "      \"mass\": " << b.mass << ",\n";
        file << "      \"radius\": " << b.radius << ",\n";
        file << "      \"temperature\": " << b.temperature << ",\n";
        file << "      \"color\": " << b.color << ",\n";
        file << "      \"parentId\": " << b.parentId << ",\n";
        file << "      \"deposits\": [\n";
        for (size_t d = 0; d < b.deposits.size(); ++d) {
            const auto& dep = b.deposits[d];
            file << "        { \"type\": " << static_cast<int>(dep.type)
                 << ", \"abundance\": " << dep.abundance
                 << ", \"worldX\": " << dep.worldX
                 << ", \"worldZ\": " << dep.worldZ << " }";
            if (d + 1 < b.deposits.size()) file << ",";
            file << "\n";
        }
        file << "      ]\n";
        file << "    }";
        if (i + 1 < m_Bodies.size()) file << ",";
        file << "\n";
    }
    file << "  ]\n}\n";
    return true;
}

// ---------------------------------------------------------------------------
// LoadFromFile — read solar system state from JSON
// ---------------------------------------------------------------------------

namespace {

// Minimal JSON helpers for loading solar system data.

std::string SSJsonString(const std::string& json, const std::string& key) {
    const std::string needle = "\"" + key + "\"";
    auto pos = json.find(needle);
    if (pos == std::string::npos) return {};
    pos = json.find(':', pos + needle.size());
    if (pos == std::string::npos) return {};
    pos = json.find('"', pos + 1);
    if (pos == std::string::npos) return {};
    auto end = json.find('"', pos + 1);
    if (end == std::string::npos) return {};
    return json.substr(pos + 1, end - pos - 1);
}

float SSJsonFloat(const std::string& json, const std::string& key, float fallback) {
    const std::string needle = "\"" + key + "\"";
    auto pos = json.find(needle);
    if (pos == std::string::npos) return fallback;
    pos = json.find(':', pos + needle.size());
    if (pos == std::string::npos) return fallback;
    ++pos;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) ++pos;
    try { return std::stof(json.substr(pos)); }
    catch (...) { return fallback; }
}

uint32_t SSJsonUInt(const std::string& json, const std::string& key, uint32_t fallback) {
    return static_cast<uint32_t>(SSJsonFloat(json, key, static_cast<float>(fallback)));
}

} // anonymous namespace

bool DevSolarSystem::LoadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return false;

    std::ostringstream ss;
    ss << file.rdbuf();
    const std::string json = ss.str();

    m_Seed = SSJsonUInt(json, "seed", m_Seed);

    // Parse bodies array — find each body block delimited by { }
    m_Bodies.clear();
    m_NextId = 1;

    const std::string bodiesKey = "\"bodies\"";
    auto arrStart = json.find(bodiesKey);
    if (arrStart == std::string::npos) return false;
    arrStart = json.find('[', arrStart);
    if (arrStart == std::string::npos) return false;

    size_t searchPos = arrStart + 1;
    while (searchPos < json.size()) {
        auto objStart = json.find('{', searchPos);
        if (objStart == std::string::npos) break;

        // Find matching close brace (handle nested deposits array)
        int depth = 0;
        size_t objEnd = objStart;
        for (size_t p = objStart; p < json.size(); ++p) {
            if (json[p] == '{') ++depth;
            else if (json[p] == '}') { --depth; if (depth == 0) { objEnd = p; break; } }
        }
        if (objEnd <= objStart) break;

        const std::string block = json.substr(objStart, objEnd - objStart + 1);

        CelestialBody body;
        body.id            = SSJsonUInt(block, "id", 0);
        body.name          = SSJsonString(block, "name");
        body.type          = static_cast<CelestialBodyType>(SSJsonUInt(block, "type", 1));
        body.orbitRadius   = SSJsonFloat(block, "orbitRadius", 0.f);
        body.orbitalPeriod = SSJsonFloat(block, "orbitalPeriod", 1.f);
        body.orbitAngle    = SSJsonFloat(block, "orbitAngle", 0.f);
        body.mass          = SSJsonFloat(block, "mass", 1.f);
        body.radius        = SSJsonFloat(block, "radius", 1.f);
        body.temperature   = SSJsonFloat(block, "temperature", 288.f);
        body.color         = SSJsonUInt(block, "color", 0xCCCCCCFF);
        body.parentId      = SSJsonUInt(block, "parentId", 0);

        // Parse deposits sub-array
        auto depStart = block.find("\"deposits\"");
        if (depStart != std::string::npos) {
            depStart = block.find('[', depStart);
            if (depStart != std::string::npos) {
                size_t depSearch = depStart + 1;
                while (depSearch < block.size()) {
                    auto ds = block.find('{', depSearch);
                    if (ds == std::string::npos) break;
                    auto de = block.find('}', ds);
                    if (de == std::string::npos) break;
                    const std::string depBlock = block.substr(ds, de - ds + 1);
                    ResourceDeposit dep;
                    dep.type      = static_cast<NF::Game::ResourceType>(SSJsonUInt(depBlock, "type", 0));
                    dep.abundance = SSJsonFloat(depBlock, "abundance", 0.f);
                    dep.worldX    = SSJsonFloat(depBlock, "worldX", 0.f);
                    dep.worldZ    = SSJsonFloat(depBlock, "worldZ", 0.f);
                    body.deposits.push_back(dep);
                    depSearch = de + 1;
                }
            }
        }

        if (body.id >= m_NextId) m_NextId = body.id + 1;
        m_Bodies.push_back(std::move(body));
        searchPos = objEnd + 1;
    }

    return !m_Bodies.empty();
}

} // namespace NF::Game::Gameplay

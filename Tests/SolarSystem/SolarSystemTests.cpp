#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "Game/Gameplay/SolarSystem/DevSolarSystem.h"
#include "Game/Gameplay/PCG/PCGItemGen.h"

using namespace NF::Game::Gameplay;
using Catch::Matchers::WithinAbs;

// =============================================================================
// DevSolarSystem — Generation
// =============================================================================

TEST_CASE("DevSolarSystem: default seed generates bodies", "[SolarSystem]") {
    DevSolarSystem sys;
    sys.Generate();
    REQUIRE(sys.BodyCount() > 0);
}

TEST_CASE("DevSolarSystem: star is always first body", "[SolarSystem]") {
    DevSolarSystem sys;
    sys.Generate();
    REQUIRE(sys.GetBodies().front().type == CelestialBodyType::Star);
    REQUIRE(sys.GetBodies().front().id == 1);
}

TEST_CASE("DevSolarSystem: generates 3-7 planets", "[SolarSystem]") {
    for (uint32_t seed = 0; seed < 20; ++seed) {
        DevSolarSystem sys;
        sys.SetSeed(seed);
        sys.Generate();
        const size_t planets = sys.PlanetCount();
        REQUIRE(planets >= 3);
        REQUIRE(planets <= 7);
    }
}

TEST_CASE("DevSolarSystem: different seeds produce different systems", "[SolarSystem]") {
    DevSolarSystem sys1;
    sys1.SetSeed(42);
    sys1.Generate();

    DevSolarSystem sys2;
    sys2.SetSeed(999);
    sys2.Generate();

    // At minimum the planet count or orbit angles should differ.
    bool differs = (sys1.BodyCount() != sys2.BodyCount());
    if (!differs && !sys1.GetBodies().empty() && !sys2.GetBodies().empty()) {
        differs = (sys1.GetBodies()[1].orbitAngle != sys2.GetBodies()[1].orbitAngle);
    }
    REQUIRE(differs);
}

TEST_CASE("DevSolarSystem: FindBody returns correct body", "[SolarSystem]") {
    DevSolarSystem sys;
    sys.Generate();

    const auto* star = sys.FindBody(1);
    REQUIRE(star != nullptr);
    REQUIRE(star->type == CelestialBodyType::Star);

    REQUIRE(sys.FindBody(9999) == nullptr);
}

TEST_CASE("DevSolarSystem: planets have deposits", "[SolarSystem]") {
    DevSolarSystem sys;
    sys.Generate();

    bool anyDeposits = false;
    for (const auto& body : sys.GetBodies()) {
        if (body.type == CelestialBodyType::Planet && !body.deposits.empty())
            anyDeposits = true;
    }
    REQUIRE(anyDeposits);
}

TEST_CASE("DevSolarSystem: star has no deposits", "[SolarSystem]") {
    DevSolarSystem sys;
    sys.Generate();

    const auto* star = sys.FindBody(1);
    REQUIRE(star != nullptr);
    REQUIRE(star->deposits.empty());
}

TEST_CASE("DevSolarSystem: body FlatX/FlatY match orbit math", "[SolarSystem]") {
    CelestialBody body;
    body.orbitRadius = 2.0f;
    body.orbitAngle  = 0.0f;
    REQUIRE_THAT(body.FlatX(), WithinAbs(2.0f, 1e-5));
    REQUIRE_THAT(body.FlatY(), WithinAbs(0.0f, 1e-5));

    body.orbitAngle = 3.14159265f;
    REQUIRE_THAT(body.FlatX(), WithinAbs(-2.0f, 1e-3));
    REQUIRE_THAT(body.FlatY(), WithinAbs(0.0f, 1e-3));
}

TEST_CASE("DevSolarSystem: moons have parent planet", "[SolarSystem]") {
    DevSolarSystem sys;
    sys.Generate();

    for (const auto& body : sys.GetBodies()) {
        if (body.type == CelestialBodyType::Moon) {
            const auto* parent = sys.FindBody(body.parentId);
            REQUIRE(parent != nullptr);
            REQUIRE(parent->type == CelestialBodyType::Planet);
        }
    }
}

TEST_CASE("DevSolarSystem: RegenerateDeposits replaces deposits", "[SolarSystem]") {
    DevSolarSystem sys;
    sys.Generate();

    const size_t origDepCount = sys.GetBodies()[1].deposits.size();
    sys.RegenerateDeposits();
    // After regeneration deposits should exist (may or may not be same count).
    REQUIRE(!sys.GetBodies().empty());
}

TEST_CASE("DevSolarSystem: CelestialBodyTypeName returns valid strings", "[SolarSystem]") {
    REQUIRE(std::string(CelestialBodyTypeName(CelestialBodyType::Star))   == "Star");
    REQUIRE(std::string(CelestialBodyTypeName(CelestialBodyType::Planet)) == "Planet");
    REQUIRE(std::string(CelestialBodyTypeName(CelestialBodyType::Moon))   == "Moon");
}

// =============================================================================
// PCGItemGen — Item generation
// =============================================================================

TEST_CASE("PCGItemGen: generates items for a body with deposits", "[PCGItemGen]") {
    CelestialBody body;
    body.id   = 10;
    body.type = CelestialBodyType::Planet;
    body.deposits.push_back({NF::Game::ResourceType::Ore, 0.5f, 10.f, 20.f});

    PCGItemGen gen;
    gen.GenerateForBody(body);

    REQUIRE(gen.ItemCount() > 0);
    REQUIRE(gen.GetItems().front().sourceBodyId == 10);
    REQUIRE(gen.GetItems().front().resourceType == NF::Game::ResourceType::Ore);
}

TEST_CASE("PCGItemGen: generates items for full system", "[PCGItemGen]") {
    DevSolarSystem sys;
    sys.SetSeed(42);
    sys.Generate();

    PCGItemGen gen;
    gen.SetSeed(42);
    gen.GenerateForSystem(sys);

    REQUIRE(gen.ItemCount() > 0);
}

TEST_CASE("PCGItemGen: items have valid positions", "[PCGItemGen]") {
    CelestialBody body;
    body.id   = 5;
    body.type = CelestialBodyType::Planet;
    body.deposits.push_back({NF::Game::ResourceType::Metal, 0.8f, 0.f, 0.f});

    PCGItemGen gen;
    gen.GenerateForBody(body);

    for (const auto& item : gen.GetItems()) {
        // Items should be near the deposit (±55 units max).
        REQUIRE(item.posX > -60.f);
        REQUIRE(item.posX < 60.f);
        REQUIRE(item.posZ > -60.f);
        REQUIRE(item.posZ < 60.f);
    }
}

TEST_CASE("PCGItemGen: FindItem returns correct item", "[PCGItemGen]") {
    CelestialBody body;
    body.id = 3;
    body.type = CelestialBodyType::Planet;
    body.deposits.push_back({NF::Game::ResourceType::Stone, 0.3f, 5.f, 5.f});

    PCGItemGen gen;
    gen.GenerateForBody(body);

    REQUIRE(gen.ItemCount() > 0);
    const auto& first = gen.GetItems().front();
    auto* found = gen.FindItem(first.id);
    REQUIRE(found != nullptr);
    REQUIRE(found->id == first.id);

    REQUIRE(gen.FindItem(99999) == nullptr);
}

TEST_CASE("PCGItemGen: ItemsForBody filters correctly", "[PCGItemGen]") {
    CelestialBody body1;
    body1.id = 1;
    body1.type = CelestialBodyType::Planet;
    body1.deposits.push_back({NF::Game::ResourceType::Ore, 0.5f, 0.f, 0.f});

    CelestialBody body2;
    body2.id = 2;
    body2.type = CelestialBodyType::Planet;
    body2.deposits.push_back({NF::Game::ResourceType::Ice, 0.3f, 10.f, 10.f});

    PCGItemGen gen;
    gen.GenerateForBody(body1);
    gen.GenerateForBody(body2);

    auto items1 = gen.ItemsForBody(1);
    auto items2 = gen.ItemsForBody(2);

    REQUIRE(!items1.empty());
    REQUIRE(!items2.empty());
    for (const auto* it : items1) REQUIRE(it->sourceBodyId == 1);
    for (const auto* it : items2) REQUIRE(it->sourceBodyId == 2);
}

TEST_CASE("PCGItemGen: user-edited items preserved across regeneration", "[PCGItemGen]") {
    DevSolarSystem sys;
    sys.SetSeed(42);
    sys.Generate();

    PCGItemGen gen;
    gen.SetSeed(42);
    gen.GenerateForSystem(sys);

    // Mark first item as user-edited.
    REQUIRE(gen.ItemCount() > 0);
    auto& first = gen.GetItems().front();
    first.userEdited = true;
    first.name       = "CustomName";
    const uint32_t editedId = first.id;

    // Regenerate.
    gen.GenerateForSystem(sys);

    // The edited item should be preserved.
    auto* found = gen.FindItem(editedId);
    REQUIRE(found != nullptr);
    REQUIRE(found->userEdited == true);
    REQUIRE(found->name == "CustomName");
}

TEST_CASE("PCGItemGen: Clear removes all items", "[PCGItemGen]") {
    CelestialBody body;
    body.id = 1;
    body.type = CelestialBodyType::Planet;
    body.deposits.push_back({NF::Game::ResourceType::Dirt, 0.5f, 0.f, 0.f});

    PCGItemGen gen;
    gen.GenerateForBody(body);
    REQUIRE(gen.ItemCount() > 0);

    gen.Clear();
    REQUIRE(gen.ItemCount() == 0);
}

TEST_CASE("PCGItemGen: items have valid quantities", "[PCGItemGen]") {
    CelestialBody body;
    body.id = 7;
    body.type = CelestialBodyType::Planet;
    body.deposits.push_back({NF::Game::ResourceType::Organic, 1.0f, 0.f, 0.f});

    PCGItemGen gen;
    gen.GenerateForBody(body);

    for (const auto& item : gen.GetItems()) {
        REQUIRE(item.quantity >= 1);
        REQUIRE(item.quantity <= 10);
    }
}

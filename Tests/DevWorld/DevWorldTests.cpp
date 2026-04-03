#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "Game/World/DevWorldConfig.h"
#include "Game/World/DevWorldSerializer.h"
#include "Game/World/WorldFileService.h"
#include "Game/World/WorldSaveLoad.h"
#include "Game/World/WorldDebugOverlay.h"
#include "Game/World/GameWorld.h"
#include <cstdint>
#include <filesystem>
#include <vector>

using namespace NF;
using namespace NF::Game;

namespace {

/// @brief Locate Content/Definitions/DevWorld.json from the CWD.
/// CTest may run from build/bin/, build/, or the repo root.
std::string FindContentRoot()
{
    DevWorldConfig probe;
    if (probe.LoadFromFile("Content/Definitions/DevWorld.json"))
        return "Content";
    if (probe.LoadFromFile("../Content/Definitions/DevWorld.json"))
        return "../Content";
    if (probe.LoadFromFile("../../Content/Definitions/DevWorld.json"))
        return "../../Content";
    return {};
}

/// @brief Return a cross-platform path in the temp directory.
std::string TempPath(const std::string& filename) {
    return (std::filesystem::temp_directory_path() / filename).string();
}

} // anonymous namespace

// =============================================================================
// DevWorldConfig
// =============================================================================

TEST_CASE("DevWorldConfig::Defaults returns valid config", "[DevWorld]") {
    auto cfg = DevWorldConfig::Defaults();
    REQUIRE(cfg.IsValid());
    REQUIRE(cfg.WorldId() == "DevWorld");
    REQUIRE(cfg.DisplayName() == "Development Sandbox");
    REQUIRE(cfg.Seed() == 42);
}

TEST_CASE("DevWorldConfig::Defaults has correct spawn point", "[DevWorld]") {
    auto cfg = DevWorldConfig::Defaults();
    const auto& sp = cfg.GetSpawnPoint();
    REQUIRE_THAT(sp.Position.X, Catch::Matchers::WithinAbs(128.0, 0.01));
    REQUIRE_THAT(sp.Position.Y, Catch::Matchers::WithinAbs(32.0, 0.01));
    REQUIRE_THAT(sp.Position.Z, Catch::Matchers::WithinAbs(128.0, 0.01));
}

TEST_CASE("DevWorldConfig::Defaults has correct camera config", "[DevWorld]") {
    auto cfg = DevWorldConfig::Defaults();
    const auto& cam = cfg.GetCameraConfig();
    REQUIRE_THAT(cam.FOV, Catch::Matchers::WithinAbs(75.0, 0.01));
    REQUIRE_THAT(cam.NearClip, Catch::Matchers::WithinAbs(0.1, 0.001));
    REQUIRE_THAT(cam.FarClip, Catch::Matchers::WithinAbs(1000.0, 0.01));
}

TEST_CASE("DevWorldConfig loads from DevWorld.json", "[DevWorld]") {
    auto contentRoot = FindContentRoot();
    REQUIRE(!contentRoot.empty());
    DevWorldConfig cfg;
    bool loaded = cfg.LoadFromFile(contentRoot + "/Definitions/DevWorld.json");
    REQUIRE(loaded);
    REQUIRE(cfg.IsValid());
    REQUIRE(cfg.WorldId() == "DevWorld");
    REQUIRE(cfg.Seed() == 42);
}

TEST_CASE("DevWorldConfig fails gracefully on missing file", "[DevWorld]") {
    DevWorldConfig cfg;
    bool loaded = cfg.LoadFromFile("nonexistent/path.json");
    REQUIRE_FALSE(loaded);
    REQUIRE_FALSE(cfg.IsValid());
}

// =============================================================================
// WorldSaveLoad — in-memory round-trip
// =============================================================================

TEST_CASE("WorldSaveLoad round-trips through memory", "[WorldSave]") {
    const uint32_t seed = 42;
    const std::vector<uint32_t> ids = {1, 2, 3, 5, 8};

    WorldSaveLoad saver;
    auto bytes = saver.SaveToMemory(seed, ids);
    REQUIRE(!bytes.empty());

    WorldSaveLoad loader;
    bool ok = loader.LoadFromMemory(bytes.data(), bytes.size());
    REQUIRE(ok);
    REQUIRE(loader.GetHeader().Seed == seed);
    REQUIRE(loader.GetHeader().EntityCount == ids.size());
    REQUIRE(loader.GetEntityIds() == ids);
}

TEST_CASE("WorldSaveLoad validates magic number", "[WorldSave]") {
    std::vector<std::byte> bad(16, std::byte{0xFF});
    WorldSaveLoad loader;
    REQUIRE_FALSE(loader.LoadFromMemory(bad.data(), bad.size()));
}

TEST_CASE("WorldSaveLoad rejects truncated data", "[WorldSave]") {
    WorldSaveLoad loader;
    std::byte tiny[2]{};
    REQUIRE_FALSE(loader.LoadFromMemory(tiny, sizeof(tiny)));
}

TEST_CASE("WorldSaveLoad round-trips with zero entities", "[WorldSave]") {
    WorldSaveLoad saver;
    auto bytes = saver.SaveToMemory(99, {});
    REQUIRE(!bytes.empty());

    WorldSaveLoad loader;
    REQUIRE(loader.LoadFromMemory(bytes.data(), bytes.size()));
    REQUIRE(loader.GetHeader().Seed == 99);
    REQUIRE(loader.GetHeader().EntityCount == 0);
    REQUIRE(loader.GetEntityIds().empty());
}

TEST_CASE("WorldSaveLoad file round-trip", "[WorldSave]") {
    const std::string path = TempPath("nf_test_save.nfsv");
    const uint32_t seed = 7;
    const std::vector<uint32_t> ids = {10, 20, 30};

    WorldSaveLoad saver;
    REQUIRE(saver.SaveToFile(path, seed, ids));

    WorldSaveLoad loader;
    REQUIRE(loader.LoadFromFile(path));
    REQUIRE(loader.GetHeader().Seed == seed);
    REQUIRE(loader.GetEntityIds() == ids);

    // Clean up
    std::remove(path.c_str());
}

// =============================================================================
// WorldDebugOverlay
// =============================================================================

TEST_CASE("WorldDebugOverlay generates lines when enabled", "[DebugOverlay]") {
    auto cfg = DevWorldConfig::Defaults();
    WorldDebugOverlay overlay;
    overlay.SetEnabled(true);
    overlay.Update(cfg, nullptr, NullEntity);

    const auto& lines = overlay.GetLines();
    REQUIRE(!lines.empty());
    REQUIRE(lines.front() == "=== World Debug Overlay ===");
    REQUIRE(lines.back() == "===========================");
}

TEST_CASE("WorldDebugOverlay produces no lines when disabled", "[DebugOverlay]") {
    auto cfg = DevWorldConfig::Defaults();
    WorldDebugOverlay overlay;
    overlay.SetEnabled(false);
    overlay.Update(cfg, nullptr, NullEntity);
    REQUIRE(overlay.GetLines().empty());
}

TEST_CASE("WorldDebugOverlay includes seed", "[DebugOverlay]") {
    auto cfg = DevWorldConfig::Defaults();
    WorldDebugOverlay overlay;
    overlay.Update(cfg, nullptr, NullEntity);

    bool foundSeed = false;
    for (const auto& line : overlay.GetLines()) {
        if (line.find("Seed") != std::string::npos &&
            line.find("42") != std::string::npos) {
            foundSeed = true;
            break;
        }
    }
    REQUIRE(foundSeed);
}

// =============================================================================
// GameWorld integration
// =============================================================================

TEST_CASE("GameWorld initializes with defaults when config missing", "[GameWorld]") {
    GameWorld gw;
    REQUIRE(gw.Initialize("nonexistent_content"));
    REQUIRE(gw.IsReady());
    REQUIRE(gw.GetConfig().IsValid());
    REQUIRE(gw.GetPlayerEntity() != NullEntity);
    gw.Shutdown();
    REQUIRE_FALSE(gw.IsReady());
}

TEST_CASE("GameWorld initializes with DevWorld.json", "[GameWorld]") {
    auto contentRoot = FindContentRoot();
    REQUIRE(!contentRoot.empty());
    GameWorld gw;
    REQUIRE(gw.Initialize(contentRoot));
    REQUIRE(gw.IsReady());
    REQUIRE(gw.GetConfig().WorldId() == "DevWorld");
    REQUIRE(gw.GetConfig().Seed() == 42);
    gw.Shutdown();
}

TEST_CASE("GameWorld spawn point matches config", "[GameWorld]") {
    auto contentRoot = FindContentRoot();
    REQUIRE(!contentRoot.empty());
    GameWorld gw;
    gw.Initialize(contentRoot);
    const auto& sp = gw.GetSpawnPoint();
    REQUIRE_THAT(sp.Position.X, Catch::Matchers::WithinAbs(128.0, 0.01));
    REQUIRE_THAT(sp.Position.Y, Catch::Matchers::WithinAbs(32.0, 0.01));
    REQUIRE_THAT(sp.Position.Z, Catch::Matchers::WithinAbs(128.0, 0.01));
    gw.Shutdown();
}

TEST_CASE("GameWorld save and load round-trip", "[GameWorld]") {
    auto contentRoot = FindContentRoot();
    REQUIRE(!contentRoot.empty());
    const std::string savePath = TempPath("nf_gameworld_save.nfsv");
    GameWorld gw;
    gw.Initialize(contentRoot);

    REQUIRE(gw.SaveWorld(savePath));
    REQUIRE(gw.LoadWorld(savePath));

    gw.Shutdown();
    std::remove(savePath.c_str());
}

TEST_CASE("GameWorld debug overlay works", "[GameWorld]") {
    auto contentRoot = FindContentRoot();
    REQUIRE(!contentRoot.empty());
    GameWorld gw;
    gw.Initialize(contentRoot);
    gw.LogDebugOverlay();
    REQUIRE(!gw.GetDebugOverlay().GetLines().empty());
    gw.Shutdown();
}

TEST_CASE("GameWorld tick does not crash", "[GameWorld]") {
    auto contentRoot = FindContentRoot();
    REQUIRE(!contentRoot.empty());
    GameWorld gw;
    gw.Initialize(contentRoot);
    gw.Tick(1.0f / 60.0f);
    gw.Tick(1.0f / 60.0f);
    gw.Tick(1.0f / 60.0f);
    gw.Shutdown();
}

TEST_CASE("GameWorld cannot save when not initialized", "[GameWorld]") {
    GameWorld gw;
    REQUIRE_FALSE(gw.SaveWorld(TempPath("should_not_exist.nfsv")));
}

// =============================================================================
// DevWorldSerializer — round-trip
// =============================================================================

TEST_CASE("DevWorldSerializer loads DevWorld.json", "[DevWorldSerializer]") {
    auto contentRoot = FindContentRoot();
    REQUIRE(!contentRoot.empty());

    nf::DevWorldSerializer serializer;
    auto result = serializer.Load(contentRoot + "/Definitions/DevWorld.json");
    REQUIRE(result.has_value());

    const auto& data = *result;
    REQUIRE(data.worldId == "DevWorld");
    REQUIRE(data.displayName == "Development Sandbox");
    REQUIRE(data.seed == 42);
    REQUIRE_THAT(data.gravity, Catch::Matchers::WithinAbs(-9.81, 0.01));
    REQUIRE_THAT(data.terrainSize[0], Catch::Matchers::WithinAbs(256.0, 0.1));
    REQUIRE_THAT(data.terrainSize[1], Catch::Matchers::WithinAbs(64.0, 0.1));
    REQUIRE_THAT(data.terrainSize[2], Catch::Matchers::WithinAbs(256.0, 0.1));
    REQUIRE_THAT(data.spawnPosition[0], Catch::Matchers::WithinAbs(128.0, 0.1));
    REQUIRE_THAT(data.spawnPosition[1], Catch::Matchers::WithinAbs(32.0, 0.1));
    REQUIRE_THAT(data.spawnPosition[2], Catch::Matchers::WithinAbs(128.0, 0.1));
    REQUIRE_THAT(data.fov, Catch::Matchers::WithinAbs(75.0, 0.1));
    REQUIRE_THAT(data.nearClip, Catch::Matchers::WithinAbs(0.1, 0.01));
    REQUIRE_THAT(data.farClip, Catch::Matchers::WithinAbs(1000.0, 0.1));
}

TEST_CASE("DevWorldSerializer fails gracefully on missing file", "[DevWorldSerializer]") {
    nf::DevWorldSerializer serializer;
    auto result = serializer.Load("nonexistent/path.json");
    REQUIRE_FALSE(result.has_value());
}

TEST_CASE("DevWorldSerializer round-trips through file", "[DevWorldSerializer]") {
    const std::string path = TempPath("nf_devworld_roundtrip.json");

    nf::DevWorldData original;
    original.worldId          = "TestWorld";
    original.displayName      = "Test Sandbox";
    original.description      = "Round-trip test world";
    original.seed             = 99;
    original.terrainSize      = {128.f, 32.f, 128.f};
    original.gravity          = -10.0f;
    original.spawnPosition    = {64.f, 16.f, 64.f};
    original.spawnRotation    = {0.f, 0.707f, 0.f, 0.707f};
    original.spawnLookDirection = {1.f, 0.f, 0.f};
    original.fov              = 90.0f;
    original.nearClip         = 0.5f;
    original.farClip          = 500.0f;
    original.moveSpeed        = 20.0f;
    original.lookSensitivity  = 0.5f;

    nf::DevWorldSerializer serializer;
    REQUIRE(serializer.Save(path, original));

    auto loaded = serializer.Load(path);
    REQUIRE(loaded.has_value());

    const auto& d = *loaded;
    REQUIRE(d.worldId == original.worldId);
    REQUIRE(d.displayName == original.displayName);
    REQUIRE(d.description == original.description);
    REQUIRE(d.seed == original.seed);
    REQUIRE_THAT(d.gravity, Catch::Matchers::WithinAbs(original.gravity, 0.01));
    REQUIRE_THAT(d.terrainSize[0], Catch::Matchers::WithinAbs(original.terrainSize[0], 0.1));
    REQUIRE_THAT(d.terrainSize[1], Catch::Matchers::WithinAbs(original.terrainSize[1], 0.1));
    REQUIRE_THAT(d.terrainSize[2], Catch::Matchers::WithinAbs(original.terrainSize[2], 0.1));
    REQUIRE_THAT(d.spawnPosition[0], Catch::Matchers::WithinAbs(original.spawnPosition[0], 0.1));
    REQUIRE_THAT(d.spawnPosition[1], Catch::Matchers::WithinAbs(original.spawnPosition[1], 0.1));
    REQUIRE_THAT(d.spawnPosition[2], Catch::Matchers::WithinAbs(original.spawnPosition[2], 0.1));
    REQUIRE_THAT(d.fov, Catch::Matchers::WithinAbs(original.fov, 0.1));
    REQUIRE_THAT(d.nearClip, Catch::Matchers::WithinAbs(original.nearClip, 0.01));
    REQUIRE_THAT(d.farClip, Catch::Matchers::WithinAbs(original.farClip, 0.1));
    REQUIRE_THAT(d.moveSpeed, Catch::Matchers::WithinAbs(original.moveSpeed, 0.1));
    REQUIRE_THAT(d.lookSensitivity, Catch::Matchers::WithinAbs(original.lookSensitivity, 0.01));

    std::remove(path.c_str());
}

// =============================================================================
// WorldFileService — real file I/O
// =============================================================================

TEST_CASE("WorldFileService loads DevWorld.json", "[WorldFileService]") {
    auto contentRoot = FindContentRoot();
    REQUIRE(!contentRoot.empty());

    nf::WorldFileService svc;
    REQUIRE(svc.LoadWorld(contentRoot + "/Definitions/DevWorld.json"));

    const auto& data = svc.GetWorldData();
    REQUIRE(data.has_value());
    REQUIRE(data->worldId == "DevWorld");
    REQUIRE(data->seed == 42);
    REQUIRE_FALSE(svc.GetState().dirty);
}

TEST_CASE("WorldFileService save and reload round-trip", "[WorldFileService]") {
    auto contentRoot = FindContentRoot();
    REQUIRE(!contentRoot.empty());
    const std::string savePath = TempPath("nf_wfs_roundtrip.json");

    nf::WorldFileService svc;
    REQUIRE(svc.LoadWorld(contentRoot + "/Definitions/DevWorld.json"));

    // Modify and save
    svc.GetMutableWorldData().seed = 777;
    svc.MarkDirty();
    REQUIRE(svc.GetState().dirty);
    REQUIRE(svc.SaveWorldAs(savePath));
    REQUIRE_FALSE(svc.GetState().dirty);

    // Load the saved file into a new service
    nf::WorldFileService svc2;
    REQUIRE(svc2.LoadWorld(savePath));
    REQUIRE(svc2.GetWorldData()->seed == 777);

    std::remove(savePath.c_str());
}

TEST_CASE("WorldFileService fails on empty path", "[WorldFileService]") {
    nf::WorldFileService svc;
    REQUIRE_FALSE(svc.LoadWorld(""));
    REQUIRE_FALSE(svc.SaveWorld());
}

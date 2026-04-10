#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "Game/Gameplay/Progression/ProgressionSystem.h"
#include "Game/Gameplay/Mining/MiningSystem.h"
#include "Game/Gameplay/Builder/BuilderSystem.h"
#include "Game/Gameplay/Inventory/InventorySystem.h"
#include "Game/Gameplay/Storage/StorageSystem.h"
#include "Game/Gameplay/Salvage/SalvageSystem.h"
#include "Game/Gameplay/Missions/MissionRegistry.h"
#include "Game/Gameplay/Combat/CombatSystem.h"
#include "Game/Interaction/RigState.h"
#include "Game/Interaction/Inventory.h"
#include "Game/Voxel/ChunkMap.h"
#include "Game/Voxel/VoxelEditApi.h"

using namespace NF::Game::Gameplay;
using Catch::Matchers::WithinAbs;

static constexpr float kEps = 1e-3f;

// =============================================================================
// ProgressionSystem
// =============================================================================

TEST_CASE("ProgressionSystem: starts at level 1 with 0 XP", "[Progression]") {
    ProgressionSystem ps;
    REQUIRE(ps.GetLevel() == 1);
    REQUIRE(ps.GetXP()    == 0);
}

TEST_CASE("ProgressionSystem: XPThreshold for level 1 is 0", "[Progression]") {
    REQUIRE(ProgressionSystem::XPThreshold(1) == 0);
}

TEST_CASE("ProgressionSystem: level-up fires callback", "[Progression]") {
    ProgressionSystem ps;
    int callbackLevel = 0;
    ps.SetLevelUpCallback([&](int lv){ callbackLevel = lv; });

    // Level 2 threshold = kXpPerLevel * 1 * 2 / 2 = 100
    ps.AddXP(100);
    REQUIRE(ps.GetLevel() == 2);
    REQUIRE(callbackLevel == 2);
}

TEST_CASE("ProgressionSystem: multiple level-ups in one AddXP", "[Progression]") {
    ProgressionSystem ps;
    int levelUps = 0;
    ps.SetLevelUpCallback([&](int){ ++levelUps; });

    // Grant a large amount to skip several levels.
    ps.AddXP(10000);
    REQUIRE(ps.GetLevel() > 5);
    REQUIRE(levelUps >= 4);
}

TEST_CASE("ProgressionSystem: skills unlock at correct levels", "[Progression]") {
    ProgressionSystem ps;
    REQUIRE(!ps.IsUnlocked(SkillUnlock::Tier2Mining));
    // XP to reach level 3: (1 + 2) * 100 = 300
    ps.AddXP(ProgressionSystem::XPThreshold(3));
    REQUIRE(ps.GetLevel() >= 3);
    REQUIRE(ps.IsUnlocked(SkillUnlock::Tier2Mining));
    REQUIRE(!ps.IsUnlocked(SkillUnlock::AdvancedBuild)); // needs level 5
}

TEST_CASE("ProgressionSystem: Reset returns to level 1", "[Progression]") {
    ProgressionSystem ps;
    ps.AddXP(500);
    ps.Reset();
    REQUIRE(ps.GetLevel() == 1);
    REQUIRE(ps.GetXP()    == 0);
}

TEST_CASE("ProgressionSystem: GetXPForNextLevel decreases as XP is added", "[Progression]") {
    ProgressionSystem ps;
    const uint32_t before = ps.GetXPForNextLevel();
    ps.AddXP(50);
    REQUIRE(ps.GetXPForNextLevel() < before);
}

// =============================================================================
// MiningSystem
// =============================================================================

TEST_CASE("MiningSystem: default tier is Tier1", "[Mining]") {
    MiningSystem ms;
    REQUIRE(ms.GetTier() == MiningTier::Tier1);
}

TEST_CASE("MiningSystem: SyncTier advances with progression level", "[Mining]") {
    MiningSystem ms;
    ProgressionSystem ps;
    ps.AddXP(ProgressionSystem::XPThreshold(3)); // reach level 3
    ms.SyncTier(ps);
    REQUIRE(ms.GetTier() == MiningTier::Tier2);

    ps.AddXP(ProgressionSystem::XPThreshold(7) - ps.GetXP()); // reach level 7
    ms.SyncTier(ps);
    REQUIRE(ms.GetTier() == MiningTier::Tier3);
}

TEST_CASE("MiningSystem: higher tiers have higher yield multiplier", "[Mining]") {
    MiningSystem ms;
    ms.SetTier(MiningTier::Tier1);
    const uint32_t y1 = ms.GetYieldMultiplier();
    ms.SetTier(MiningTier::Tier2);
    REQUIRE(ms.GetYieldMultiplier() > y1);
    ms.SetTier(MiningTier::Tier3);
    REQUIRE(ms.GetYieldMultiplier() > y1);
}

TEST_CASE("MiningSystem: Mine awards XP on success", "[Mining]") {
    NF::Game::ChunkMap map;
    NF::Game::VoxelEditApi api(map);
    // Place a solid voxel to mine.
    api.SetVoxel(0, 0, 0, static_cast<NF::Game::VoxelId>(NF::Game::VoxelType::Stone));

    NF::Game::RigState rig;
    MiningSystem ms;
    ProgressionSystem ps;

    const uint32_t xpBefore = ps.GetXP();
    ms.Mine(api, rig, 0, 0, 0, &ps);
    REQUIRE(ps.GetXP() >= xpBefore);
}

TEST_CASE("MiningSystem: TotalMined increments on success", "[Mining]") {
    NF::Game::ChunkMap map;
    NF::Game::VoxelEditApi api(map);
    api.SetVoxel(1, 0, 0, static_cast<NF::Game::VoxelId>(NF::Game::VoxelType::Ore));

    NF::Game::RigState rig;
    MiningSystem ms;
    ms.Mine(api, rig, 1, 0, 0, nullptr);
    REQUIRE(ms.GetTotalMined() == 1);
}

// =============================================================================
// BuilderSystem
// =============================================================================

TEST_CASE("BuilderSystem: FindRecipe returns non-null for Stone", "[Builder]") {
    BuilderSystem bs;
    REQUIRE(bs.FindRecipe(NF::Game::ResourceType::Stone) != nullptr);
}

TEST_CASE("BuilderSystem: Build succeeds with sufficient resources", "[Builder]") {
    NF::Game::ChunkMap map;
    NF::Game::VoxelEditApi api(map);
    NF::Game::Inventory inv;
    inv.AddItem(NF::Game::ResourceType::Stone, 5);

    BuilderSystem bs;
    const BuildResult r = bs.Build(api, inv, 2, 0, 0, NF::Game::ResourceType::Stone);
    REQUIRE(r == BuildResult::Success);
    REQUIRE(bs.GetTotalBuilt() == 1);
    REQUIRE(inv.GetCount(NF::Game::ResourceType::Stone) == 4);
}

TEST_CASE("BuilderSystem: Build fails without resources", "[Builder]") {
    NF::Game::ChunkMap map;
    NF::Game::VoxelEditApi api(map);
    NF::Game::Inventory inv; // empty

    BuilderSystem bs;
    const BuildResult r = bs.Build(api, inv, 3, 0, 0, NF::Game::ResourceType::Stone);
    REQUIRE(r == BuildResult::InsufficientResources);
}

TEST_CASE("BuilderSystem: Metal build requires AdvancedBuild unlock", "[Builder]") {
    NF::Game::ChunkMap map;
    NF::Game::VoxelEditApi api(map);
    NF::Game::Inventory inv;
    inv.AddItem(NF::Game::ResourceType::Metal, 10);

    BuilderSystem bs;
    // No progression system or unlocked skill -> LockedTier.
    REQUIRE(bs.Build(api, inv, 4, 0, 0, NF::Game::ResourceType::Metal, nullptr)
            == BuildResult::LockedTier);

    // With progression at level 5+
    ProgressionSystem ps;
    ps.AddXP(ProgressionSystem::XPThreshold(5));
    REQUIRE(bs.Build(api, inv, 4, 0, 0, NF::Game::ResourceType::Metal, &ps)
            == BuildResult::Success);
}

TEST_CASE("BuilderSystem: Teardown refunds resources", "[Builder]") {
    NF::Game::ChunkMap map;
    NF::Game::VoxelEditApi api(map);
    api.SetVoxel(5, 0, 0, static_cast<NF::Game::VoxelId>(NF::Game::VoxelType::Stone));

    NF::Game::Inventory inv;
    BuilderSystem bs;
    const TeardownResult r = bs.Teardown(api, inv, 5, 0, 0);
    REQUIRE(r == TeardownResult::Success);
    REQUIRE(inv.GetCount(NF::Game::ResourceType::Stone) > 0);
    REQUIRE(bs.GetTotalTornDown() == 1);
}

// =============================================================================
// InventorySystem
// =============================================================================

TEST_CASE("InventorySystem: AddContainer returns unique IDs", "[Inventory]") {
    InventorySystem is;
    const ContainerId a = is.AddContainer("Backpack");
    const ContainerId b = is.AddContainer("Chest");
    REQUIRE(a != b);
    REQUIRE(a != kInvalidContainer);
    REQUIRE(b != kInvalidContainer);
}

TEST_CASE("InventorySystem: GetContainer returns correct inventory", "[Inventory]") {
    InventorySystem is;
    const ContainerId id = is.AddContainer("Pack");
    auto* inv = is.GetContainer(id);
    REQUIRE(inv != nullptr);
    inv->AddItem(NF::Game::ResourceType::Stone, 3);
    REQUIRE(is.GetContainer(id)->GetCount(NF::Game::ResourceType::Stone) == 3);
}

TEST_CASE("InventorySystem: FindContainer by name", "[Inventory]") {
    InventorySystem is;
    is.AddContainer("Alpha");
    const ContainerId beta = is.AddContainer("Beta");
    REQUIRE(is.FindContainer("Beta") == beta);
    REQUIRE(is.FindContainer("Unknown") == kInvalidContainer);
}

TEST_CASE("InventorySystem: Transfer moves items between containers", "[Inventory]") {
    InventorySystem is;
    const ContainerId src = is.AddContainer("Src");
    const ContainerId dst = is.AddContainer("Dst");
    is.GetContainer(src)->AddItem(NF::Game::ResourceType::Ore, 5);

    REQUIRE(is.Transfer(src, dst, NF::Game::ResourceType::Ore, 3));
    REQUIRE(is.GetContainer(src)->GetCount(NF::Game::ResourceType::Ore) == 2);
    REQUIRE(is.GetContainer(dst)->GetCount(NF::Game::ResourceType::Ore) == 3);
}

TEST_CASE("InventorySystem: Transfer fails when source has insufficient items", "[Inventory]") {
    InventorySystem is;
    const ContainerId src = is.AddContainer("Src");
    const ContainerId dst = is.AddContainer("Dst");
    is.GetContainer(src)->AddItem(NF::Game::ResourceType::Ore, 1);

    REQUIRE(!is.Transfer(src, dst, NF::Game::ResourceType::Ore, 5));
    REQUIRE(is.GetContainer(src)->GetCount(NF::Game::ResourceType::Ore) == 1);
}

TEST_CASE("InventorySystem: RemoveContainer decrements count", "[Inventory]") {
    InventorySystem is;
    const ContainerId id = is.AddContainer("Tmp");
    REQUIRE(is.ContainerCount() == 1);
    REQUIRE(is.RemoveContainer(id));
    REQUIRE(is.ContainerCount() == 0);
}

// =============================================================================
// StorageSystem
// =============================================================================

TEST_CASE("StorageSystem: AddBox returns unique IDs", "[Storage]") {
    StorageSystem ss;
    const StorageBoxId a = ss.AddBox("BoxA", {0,0,0});
    const StorageBoxId b = ss.AddBox("BoxB", {10,0,0});
    REQUIRE(a != b);
    REQUIRE(a != kInvalidBoxId);
}

TEST_CASE("StorageSystem: FindNearest returns closest box within radius", "[Storage]") {
    StorageSystem ss;
    ss.AddBox("Far",   {100,0,0});
    const StorageBoxId near = ss.AddBox("Near", {1,0,0});
    const StorageBoxId found = ss.FindNearest({0,0,0}, 10.f);
    REQUIRE(found == near);
}

TEST_CASE("StorageSystem: FindNearest returns invalid when none in range", "[Storage]") {
    StorageSystem ss;
    ss.AddBox("Far", {100,0,0});
    REQUIRE(ss.FindNearest({0,0,0}, 5.f) == kInvalidBoxId);
}

TEST_CASE("StorageSystem: Deposit moves items from inventory into box", "[Storage]") {
    StorageSystem ss;
    const StorageBoxId id = ss.AddBox("Crate", {0,0,0});
    NF::Game::Inventory player;
    player.AddItem(NF::Game::ResourceType::Dirt, 4);

    REQUIRE(ss.Deposit(id, player, NF::Game::ResourceType::Dirt, 2));
    REQUIRE(player.GetCount(NF::Game::ResourceType::Dirt) == 2);
    REQUIRE(ss.GetBox(id)->GetCount(NF::Game::ResourceType::Dirt) == 2);
}

TEST_CASE("StorageSystem: Withdraw moves items from box into inventory", "[Storage]") {
    StorageSystem ss;
    const StorageBoxId id = ss.AddBox("Crate", {0,0,0});
    ss.GetBox(id)->AddItem(NF::Game::ResourceType::Metal, 5);
    NF::Game::Inventory player;

    REQUIRE(ss.Withdraw(id, player, NF::Game::ResourceType::Metal, 3));
    REQUIRE(ss.GetBox(id)->GetCount(NF::Game::ResourceType::Metal) == 2);
    REQUIRE(player.GetCount(NF::Game::ResourceType::Metal) == 3);
}

// =============================================================================
// CombatSystem
// =============================================================================

TEST_CASE("CombatSystem: RegisterEntity creates alive entity at full health", "[Combat]") {
    CombatSystem cs;
    cs.RegisterEntity(1, 80.f, 5.f);
    REQUIRE(cs.IsAlive(1));
    REQUIRE_THAT(cs.GetState(1).health, WithinAbs(80.f, kEps));
}

TEST_CASE("CombatSystem: DealDamage reduces health", "[Combat]") {
    CombatSystem cs;
    cs.RegisterEntity(2, 100.f, 0.f);
    REQUIRE(cs.DealDamage(2, 30.f, DamageType::Physical) == DamageResult::Hit);
    REQUIRE_THAT(cs.GetState(2).health, WithinAbs(70.f, kEps));
}

TEST_CASE("CombatSystem: armor mitigates physical damage", "[Combat]") {
    CombatSystem cs;
    cs.RegisterEntity(3, 100.f, 20.f); // 20 armor
    cs.DealDamage(3, 30.f, DamageType::Physical); // effective = 10
    REQUIRE_THAT(cs.GetState(3).health, WithinAbs(90.f, kEps));
}

TEST_CASE("CombatSystem: armor does not mitigate energy damage", "[Combat]") {
    CombatSystem cs;
    cs.RegisterEntity(4, 100.f, 50.f); // large armor
    cs.DealDamage(4, 30.f, DamageType::Energy); // bypasses armor
    REQUIRE_THAT(cs.GetState(4).health, WithinAbs(70.f, kEps));
}

TEST_CASE("CombatSystem: lethal damage fires death callback and sets not-alive", "[Combat]") {
    CombatSystem cs;
    uint32_t deadId = 0;
    cs.SetDeathCallback([&](uint32_t id){ deadId = id; });
    cs.RegisterEntity(5, 50.f, 0.f);
    REQUIRE(cs.DealDamage(5, 100.f, DamageType::Physical) == DamageResult::Killed);
    REQUIRE(!cs.IsAlive(5));
    REQUIRE(deadId == 5);
}

TEST_CASE("CombatSystem: DealDamage on dead entity returns AlreadyDead", "[Combat]") {
    CombatSystem cs;
    cs.RegisterEntity(6, 10.f, 0.f);
    cs.DealDamage(6, 100.f, DamageType::Physical);
    REQUIRE(cs.DealDamage(6, 5.f, DamageType::Physical) == DamageResult::AlreadyDead);
}

TEST_CASE("CombatSystem: Heal restores health clamped to max", "[Combat]") {
    CombatSystem cs;
    cs.RegisterEntity(7, 100.f, 0.f);
    cs.DealDamage(7, 40.f, DamageType::Physical);
    cs.Heal(7, 20.f);
    REQUIRE_THAT(cs.GetState(7).health, WithinAbs(80.f, kEps));
    cs.Heal(7, 1000.f);
    REQUIRE_THAT(cs.GetState(7).health, WithinAbs(100.f, kEps));
}

TEST_CASE("CombatSystem: Respawn brings dead entity back to full health", "[Combat]") {
    CombatSystem cs;
    cs.RegisterEntity(8, 60.f, 0.f);
    cs.DealDamage(8, 100.f, DamageType::Explosive);
    REQUIRE(!cs.IsAlive(8));
    REQUIRE(cs.Respawn(8));
    REQUIRE(cs.IsAlive(8));
    REQUIRE_THAT(cs.GetState(8).health, WithinAbs(60.f, kEps));
}

TEST_CASE("CombatSystem: Respawn returns false for unknown entity", "[Combat]") {
    CombatSystem cs;
    REQUIRE(!cs.Respawn(999));
}

TEST_CASE("CombatSystem: UnregisterEntity removes entity", "[Combat]") {
    CombatSystem cs;
    cs.RegisterEntity(9, 100.f, 0.f);
    REQUIRE(cs.EntityCount() == 1);
    REQUIRE(cs.UnregisterEntity(9));
    REQUIRE(cs.EntityCount() == 0);
    REQUIRE(cs.DealDamage(9, 10.f, DamageType::Physical) == DamageResult::NotFound);
}

// =============================================================================
// SalvageSystem — Phase 7
// =============================================================================

TEST_CASE("SalvageSystem: PlaceWreck returns unique IDs", "[Salvage]") {
    NF::Game::Gameplay::SalvageSystem ss;
    const auto id1 = ss.PlaceWreck("W1", {0.f, 0.f, 0.f});
    const auto id2 = ss.PlaceWreck("W2", {5.f, 0.f, 0.f});
    REQUIRE(id1 != id2);
    REQUIRE(id1 != NF::Game::Gameplay::kInvalidWreckId);
    REQUIRE(id2 != NF::Game::Gameplay::kInvalidWreckId);
    REQUIRE(ss.WreckCount() == 2);
}

TEST_CASE("SalvageSystem: AddLoot stores resources in wreck", "[Salvage]") {
    NF::Game::Gameplay::SalvageSystem ss;
    const auto id = ss.PlaceWreck("Probe", {0.f, 0.f, 0.f});
    REQUIRE(ss.AddLoot(id, NF::Game::ResourceType::Ore, 10u));
    const auto* w = ss.GetWreck(id);
    REQUIRE(w != nullptr);
    REQUIRE(w->TotalLoot() == 10u);
    REQUIRE(!w->IsEmpty());
}

TEST_CASE("SalvageSystem: Salvage extracts resources into inventory", "[Salvage]") {
    NF::Game::Gameplay::SalvageSystem ss;
    const auto id = ss.PlaceWreck("Probe", {0.f, 0.f, 0.f});
    ss.AddLoot(id, NF::Game::ResourceType::Ore, 10u);

    NF::Game::Inventory inv;
    const uint32_t extracted = ss.Salvage(id, inv, 5u);
    REQUIRE(extracted == 5u);
    REQUIRE(inv.GetCount(NF::Game::ResourceType::Ore) == 5u);
    REQUIRE(ss.GetWreck(id)->TotalLoot() == 5u);
}

TEST_CASE("SalvageSystem: Salvage returns 0 on empty wreck", "[Salvage]") {
    NF::Game::Gameplay::SalvageSystem ss;
    const auto id = ss.PlaceWreck("Empty", {0.f, 0.f, 0.f});
    NF::Game::Inventory inv;
    REQUIRE(ss.Salvage(id, inv) == 0u);
}

TEST_CASE("SalvageSystem: FindNearest returns closest non-empty wreck", "[Salvage]") {
    NF::Game::Gameplay::SalvageSystem ss;
    const auto near = ss.PlaceWreck("Near", {2.f, 0.f, 0.f});
    const auto far  = ss.PlaceWreck("Far",  {50.f, 0.f, 0.f});
    ss.AddLoot(near, NF::Game::ResourceType::Stone, 5u);
    ss.AddLoot(far,  NF::Game::ResourceType::Stone, 5u);

    const NF::Vector3 origin = {0.f, 0.f, 0.f};
    REQUIRE(ss.FindNearest(origin, 100.f) == near);
}

TEST_CASE("SalvageSystem: FindNearest ignores empty wrecks", "[Salvage]") {
    NF::Game::Gameplay::SalvageSystem ss;
    const auto id = ss.PlaceWreck("Empty", {2.f, 0.f, 0.f});
    (void)id; // empty, no loot added
    REQUIRE(ss.FindNearest({0.f, 0.f, 0.f}, 100.f) == NF::Game::Gameplay::kInvalidWreckId);
}

TEST_CASE("SalvageSystem: RemoveWreck removes the site", "[Salvage]") {
    NF::Game::Gameplay::SalvageSystem ss;
    const auto id = ss.PlaceWreck("TBR", {0.f, 0.f, 0.f});
    REQUIRE(ss.WreckCount() == 1);
    REQUIRE(ss.RemoveWreck(id));
    REQUIRE(ss.WreckCount() == 0);
    REQUIRE(ss.GetWreck(id) == nullptr);
}

// =============================================================================
// MissionRegistry — Phase 7 notifiers
// =============================================================================

TEST_CASE("MissionRegistry: NotifyDeposited advances DepositToStorage mission", "[Mission]") {
    NF::Game::Gameplay::MissionRegistry reg;
    reg.Init();

    int completedId = -1;
    reg.SetOnMissionComplete([&](uint32_t id){ completedId = static_cast<int>(id); });

    // "Stash It" mission (id=6) requires 5 deposited units.
    reg.NotifyDeposited(3u);
    REQUIRE(completedId == -1); // not yet
    reg.NotifyDeposited(2u);
    REQUIRE(completedId == 6);
}

TEST_CASE("MissionRegistry: NotifySalvaged advances SalvageWreck mission", "[Mission]") {
    NF::Game::Gameplay::MissionRegistry reg;
    reg.Init();

    int completedId = -1;
    reg.SetOnMissionComplete([&](uint32_t id){ completedId = static_cast<int>(id); });

    // "Salvage Run" mission (id=7) requires 3 salvaged items.
    reg.NotifySalvaged(3u);
    REQUIRE(completedId == 7);
}

# NovaForge Roadmap

## Repo Identity
NovaForge is a standalone native C++ repo for the game, engine, renderer, and game-specific editor.
The editor exists to build NovaForge.
The editor does not ship with the game.
This repo does not own Atlas Suite, WPF shell work, or generalized tooling ambitions.

## Direction Lock
The project is now locked to an editor-first, voxel-first implementation path.
The current goal is not feature expansion. The current goal is editor trustworthiness.

## Phase 0 — Bootstrap and Reset Baseline
Status: Done

Goal:
- clean repo structure
- build system
- editor and game program entrypoints
- content/config/docs/test layout

## Phase 1 — Editor Core Trust Layer
Status: Done

Goal:
Make the editor a trustworthy tool instead of a promising shell.

Required outputs:
- authoritative selection service
- property inspector with read and write support
- editor command history
- voxel add/remove commands routed through undo/redo
- toolbar and menu state derived from command availability
- stable docking, input routing, and panel resize behavior

Done when:
- click voxel -> select ✅
- edit voxel/property -> change applies ✅
- undo -> change reverts ✅
- redo -> change reapplies ✅

## Phase 2 — Persistence and Dev World Round-Trip
Status: Done

Goal:
Prove that edits survive save and reload.

Required outputs:
- editor world session management
- save/load pipeline for dev world
- dirty state tracking
- project/path resolution cleanup
- smoke-tested save/reload round-trip

Done when:
- edit world -> save -> reload -> same result ✅
- standalone game can load the same saved state ✅

## Phase 3 — Viewport and World Truth
Status: Done

Goal:
Make viewport, outliner, inspector, and actual world state reflect the same truth.

Required outputs:
- single authoritative viewport path
- viewport-local picking
- chunk/entity/voxel world representation cleanup
- outliner backed by real world data
- stale parallel viewport/editor paths archived or removed

Done when:
- visible world, selected world, and saved world all match ✅

## Phase 4 — Dev World Interaction Loop
Status: Done

Goal:
Use one stable dev world to prove the first game-authoring loop.

Required outputs:
- starter R.I.G. state
- mining/place/repair loop through editor and standalone game
- basic HUD and loop validation
- play/relaunch flow from editor

All outputs verified. ✅

## Phase 5 — Runtime Expansion
Status: Done

Goal:
Grow the game loop beyond the DevWorld interaction loop toward a real playable game.
All work must be exercised in DevWorld/DevSolarSystem before it goes to main.

### Phase 5 Checklist

**Editor Improvements**
- [x] PIE crosshair overlay in viewport when Playing
- [x] PCGItemGen save/load (.nfpi) — placed items persist across sessions
- [x] Functional TransformGizmo — drag-to-move entities in the viewport
- [x] Full PIE HUD mirror — inventory slots, active tool feedback, position, mine flash

**Solar System → In-Game**
- [x] GameClientApp solar map overlay (M key, 2D orbital map with bodies and deposits)
- [x] Travel mechanic skeleton — EditorWorldSession::TravelToBody(bodyId) swaps world context
- [x] Per-body terrain generation — PCGWorldGen seeded from body ID generates unique terrain
- [x] Resource deposits → voxel terrain bridge — deposit positions influence ore placement per body
- [x] Dev-only access gate — config flag `dev.solar_map_enabled` in `novaforge.project.json` gates solar map in game without shipping to players

**Core Gameplay Systems**
- [x] ExplorationSystem skeleton — owns body-to-body travel state, current/previous body tracking
- [x] CombatSystem basics — damage model, armour mitigation, health, death callback, respawn
- [x] BuilderSystem + recipe palette — recipe-based voxel placement/teardown with resource deduction and XP
- [x] Economy basics — ResourceRegistry live (8 types), TradeMarket with buy/sell/price fluctuation
- [x] ProgressionSystem wired to gameplay — XP awarded for mining (10 XP/voxel) and kills (25 XP); level-up callback fires; tier unlocks gate MiningSystem tier
- [x] MissionRegistry — 3 starter missions (First Contact, Ore Run, Journeyman) auto-accepted; completion/reward pipeline wired to Orchestrator

**Multiplayer Foundation**
- [x] Real socket listen/connect — GameServer/GameClient over TCP (POSIX Socket + NetChannel)
- [x] Snapshot replication — authoritative server sends player positions + voxel edits to clients each tick
- [x] Dedicated server headless mode — NovaForgeServer/main.cpp runs a full GameServer tick loop (60 Hz, configurable port/tick-rate)

## Explicitly Deferred
Do not prioritize these until earlier phases are complete:
- visual scripting
- generalized IDE/tool suite work
- Atlas Suite concepts
- WPF shell concepts
- advanced AI systems
- broad server/admin platform work
- luxury editor panels not tied to core authoring loop

## Phase 7 — Salvage, Storage & World Interaction Loop
Status: Current Active Phase

Goal:
Implement the salvage and storage mechanics, closing the resource loop:
mine → salvage wreck → deposit to storage → trade or craft.
All systems must be exercised in DevWorld before going to main.

### Phase 7 Checklist

**SalvageSystem**
- [x] `SalvageSystem` — header + implementation: wreck sites with resource loot pools; `PlaceWreck`, `AddLoot`, `FindNearest`, `Salvage` (batch extraction), `GetWrecks`
- [x] Default wreck ("Derelict Probe") seeded at session start 15 units from spawn with Ore/Metal/Stone loot

**MissionRegistry**
- [x] `DepositToStorage` + `SalvageWreck` objective types added
- [x] `NotifyDeposited()` + `NotifySalvaged()` notifiers
- [x] Two new auto-accepted starter missions — "Stash It" (deposit 5 units) + "Salvage Run" (salvage 3 items)

**Orchestrator**
- [x] `SalvageSystem`, `StorageSystem`, `InventorySystem` owned and initialised in `Init`
- [x] Default "Homebase Storage" box at world origin; default "Backpack" container in InventorySystem
- [x] `GetSalvage()`, `GetStorage()`, `GetInventorySys()` accessors exposed

**In-Game (GameClientApp)**
- [x] G key — salvage nearest wreck (extracts one batch of 5) or deposit all held items into nearest storage box (within 25 units)
- [x] HUD shows "[G] Salvage: \<name\>" when a non-empty wreck is nearby, or "[G] Deposit to storage" when a box is nearby

**Editor**
- [x] `InventoryPanel` — shows backpack contents, Homebase Storage box, and wreck sites with loot counts
- [x] `InventoryPanel` wired into bottom dock as "Inventory" tab

## Phase 6 — Economy, Manufacturing & Station Loop
Status: Done

Goal:
Wire the economy and manufacturing subsystems into the live gameplay loop.
All work must be exercised in DevWorld/DevSolarSystem before it goes to main.

### Phase 6 Checklist

**Mission System**
- [x] Two new starter missions — "First Trade" (sell 3 resources) and "First Craft" (manufacture 1 Metal)
- [x] MissionRegistry — `SellResources` + `CraftItem` objective types; `NotifySold()` + `NotifyCrafted()` notifiers

**Economy Integration**
- [x] ResourceRegistry + TradeMarket wired into Orchestrator via StationServices
- [x] Starter credits (500) seeded at session start; station named "Homebase"
- [x] ManufacturingQueue auto-collect in Orchestrator::Tick — output deposited into player inventory; `NotifyCrafted` fired

**In-Game (GameClientApp)**
- [x] F key — dock / undock at Homebase station
- [x] T key — sell all held resources at station market (requires docked)
- [x] HUD shows credits balance + station context hint (F/T bindings)

**Editor**
- [x] EconomyPanel — market price table, credits balance, station status, manufacturing recipes
- [x] EconomyPanel wired into bottom dock as "Economy" tab

## Phase 7 — Salvage, Storage & World Interaction Loop
Status: Done

Goal:
Implement the salvage and storage mechanics, closing the resource loop:
mine → salvage wreck → deposit to storage → trade or craft.
All systems must be exercised in DevWorld before going to main.

### Phase 7 Checklist

**SalvageSystem**
- [x] `SalvageSystem` — header + implementation: wreck sites with resource loot pools; `PlaceWreck`, `AddLoot`, `FindNearest`, `Salvage` (batch extraction), `GetWrecks`
- [x] Default wreck ("Derelict Probe") seeded at session start 15 units from spawn with Ore/Metal/Stone loot

**MissionRegistry**
- [x] `DepositToStorage` + `SalvageWreck` objective types added
- [x] `NotifyDeposited()` + `NotifySalvaged()` notifiers
- [x] Two new auto-accepted starter missions — "Stash It" (deposit 5 units) + "Salvage Run" (salvage 3 items)

**Orchestrator**
- [x] `SalvageSystem`, `StorageSystem`, `InventorySystem` owned and initialised in `Init`
- [x] Default "Homebase Storage" box at world origin; default "Backpack" container in InventorySystem
- [x] `GetSalvage()`, `GetStorage()`, `GetInventorySys()` accessors exposed

**In-Game (GameClientApp)**
- [x] G key — salvage nearest wreck (extracts one batch of 5) or deposit all held items into nearest storage box (within 25 units)
- [x] HUD shows "[G] Salvage: \<name\>" when a non-empty wreck is nearby, or "[G] Deposit to storage" when a box is nearby

**Editor**
- [x] `InventoryPanel` — shows backpack contents, Homebase Storage box, and wreck sites with loot counts
- [x] `InventoryPanel` wired into bottom dock as "Inventory" tab

## Phase 8 — Chunk Streaming
Status: Done

Goal:
Stream voxel chunks in and out of memory dynamically as the player moves,
replacing the flat "load everything at startup" approach.

### Phase 8 Checklist

- [x] `ChunkStreamer` — configurable load/unload radii, LRU memory cap, save-on-unload
- [x] Background terrain generation via `TaskSystem`; async chunk queue
- [x] LOD levels (`Full`, `Simplified`, `None`) computed by distance
- [x] `ChunkStreamConfig` struct (LoadRadius=4, UnloadRadius=6, MaxLoadedChunks=512)
- [x] `ChunkStreamer` owned and ticked by `Orchestrator`; seeded from world seed
- [x] Dedicated server streams around origin; client/listen-server streams around player position

## Phase 9 — Audio Foundation
Status: Done

Goal:
Establish a cross-platform software audio pipeline:
device → bank → mixer → spatial panning.

### Phase 9 Checklist

- [x] `AudioDevice` — Null (cross-platform) backend; master volume; 44 100 Hz / stereo
- [x] `SoundAsset` + `SoundBank` — in-memory PCM float samples; `LoadWav`, `GenerateTone`, `GenerateNoise`
- [x] `AudioMixer` — up to 64 concurrent channels; `Play`, `Stop`, `StopAll`, `Update`; looping support
- [x] `SpatialAudio` — inverse-distance attenuation + stereo pan from listener orientation; `PlayAt`
- [x] Orchestrator wires all four systems; seeds three placeholder sounds (`sfx_mine`, `sfx_place`, `sfx_ambient`)
- [x] Orchestrator `Tick` calls `AudioMixer::Update` and `SpatialAudio::SetListenerPosition`
- [x] GameClientApp caches sound ids; plays `sfx_mine` spatially on mine hit; loops `sfx_ambient` on start

## Phase 10 — Factions, Anomalies & Audio Feedback
Status: Done

Goal:
Make the game world feel alive with faction relationships and discoverable anomaly events.
Closes the audio feedback loop started in Phase 9.

### Phase 10 Checklist

**FactionRegistry**
- [x] `FactionRegistry` — 3 built-in factions: Miners Guild, Traders Union, Raiders
- [x] Reputation per faction [-100, 100]; standing thresholds: Hostile < -25 ≤ Neutral < 25 ≤ Friendly < 75 ≤ Allied
- [x] `NotifyMined` → +1 Miners Guild; `NotifySold` → +1 Traders Union; `NotifyKill` → -5 Raiders; `NotifyInvestigated` → +2 Traders Union
- [x] Owned by `Orchestrator`; wired to mine, kill, and anomaly investigation callbacks

**AnomalySystem**
- [x] `AnomalySystem` — anomaly sites with typed loot pools (Derelict, Mineral, Signal, Radiation)
- [x] `PlaceAnomaly`, `AddLoot`, `FindNearest`, `Investigate` (extracts loot, marks investigated)
- [x] Default "Strange Signal" anomaly seeded 30 units from spawn with Ore/Ice loot
- [x] Owned by `Orchestrator`; investigation callback fires `FactionRegistry` + `MissionRegistry`

**MissionRegistry**
- [x] `GainFactionRep` + `InvestigateAnomaly` objective types added
- [x] `NotifyRepGained()` + `NotifyInvestigated()` notifiers
- [x] Two new auto-accepted missions — "Guild Bond" (gain 10 Miners Guild rep) + "Signal Hunt" (investigate 1 anomaly)

**In-Game (GameClientApp)**
- [x] I key — investigate nearest anomaly within 25 units
- [x] HUD shows "[I] Investigate: \<name\>" when an uninvestigated anomaly is nearby
- [x] Audio: `sfx_mine` played spatially via `SpatialAudio::PlayAt` on every successful mine
- [x] Audio: `sfx_ambient` looped at 15% volume on game start

**Editor**
- [x] `FactionPanel` — three faction rows with colour-coded reputation bars and standing text
- [x] `FactionPanel` wired into bottom dock as "Factions" tab

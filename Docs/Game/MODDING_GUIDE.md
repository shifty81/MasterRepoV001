# NovaForge Modding Guide

**Audience:** Modders and advanced players who want to add custom ships, missions, skills, and modules.  
**Requires:** A text editor and basic knowledge of JSON.  

---

## 1. Overview

NovaForge is built around data-driven JSON files for all game content. Ships, missions, skills, and modules are all defined as JSON files that conform to published JSON Schema contracts. Adding custom content requires:

1. Creating a new JSON data file in the correct `Content/Data/` subdirectory.
2. Ensuring it validates against the relevant schema.
3. (Optional) Writing a `.nfgraph` GameFlow graph script for complex mission logic.

No recompilation is required for data-only mods.

---

## 2. Adding a Custom Ship

Ship definitions live in `Content/Data/Ships/`. Each ship is a single JSON file.

**Step 1: Create the JSON file**

```json
// Content/Data/Ships/my_custom_freighter.json
{
  "$schema": "../../../Schemas/ship.schema.json",
  "id": "my_custom_freighter",
  "class": "transport",
  "name": "Custom Freighter",
  "description": "A custom heavy transport built for bulk hauling.",
  "stats": {
    "hull": 900,
    "shield": 200,
    "speed": 35.0,
    "maneuverability": 0.25,
    "cargo_capacity": 200,
    "crew": 6,
    "power_output": 150.0
  },
  "modules": ["cargo_expander", "basic_engine"],
  "module_slots": { "weapon": 1, "utility": 2, "engine": 1, "cargo": 6 },
  "cost": { "Metal": 1200, "Stone": 300 },
  "unlock_level": 4
}
```

**Step 2: Validate the schema**

```bash
# Using ajv-cli (npm install -g ajv-cli):
ajv validate -s Schemas/ship.schema.json -d Content/Data/Ships/my_custom_freighter.json
```

**Step 3: (Optional) Create a voxel asset**

Open the editor, create a new Block Asset at `Content/Blocks/Ships/my_custom_freighter.nfblock`, and author the ship voxel model. Reference this path in the `voxel_asset` field of your JSON.

---

## 3. Adding a Custom Mission

Mission definitions live in `Content/Data/Missions/`.

```json
// Content/Data/Missions/my_escort_mission.json
{
  "$schema": "../../../Schemas/mission.schema.json",
  "id": "my_escort_mission",
  "type": "escort",
  "name": "Safe Passage",
  "description": "Escort a merchant convoy through pirate territory.",
  "sector": "sector_alpha",
  "giver": "faction_merchant_guild",
  "prerequisites": [],
  "objectives": [
    {
      "id": "obj_escort_convoy",
      "type": "travel",
      "description": "Escort convoy to the destination",
      "target_id": "poi_merchant_waypoint"
    }
  ],
  "rewards": {
    "credits": 1200,
    "xp": 200,
    "reputation": { "faction_merchant_guild": 5 },
    "items": []
  },
  "repeatable": true
}
```

### 3.1 Scripted Missions with GraphVM

For missions requiring branching dialogue, timed events, or complex objective chains, you can attach a `.nfgraph` GameFlow graph:

```json
"graph_script": "Content/Graphs/Missions/my_escort_mission.nfgraph"
```

Graphs are authored in the NovaForge Editor's Graph Editor panel. See Section 6 for an overview of the GraphVM scripting system.

---

## 4. Adding a Custom Skill

Skill definitions live in `Content/Data/Skills/`.

```json
// Content/Data/Skills/heavy_mining.json
{
  "$schema": "../../../Schemas/skill.schema.json",
  "id": "heavy_mining",
  "tree": "mining",
  "name": "Heavy Mining",
  "description": "Increases mining laser damage and ore yield from large veins.",
  "max_rank": 3,
  "prerequisites": [
    { "skill_id": "basic_mining", "min_rank": 2 }
  ],
  "effects_per_rank": [
    { "stat": "mining_damage", "operation": "multiply", "value": 1.15 },
    { "stat": "mining_damage", "operation": "multiply", "value": 1.30 },
    { "stat": "mining_damage", "operation": "multiply", "value": 1.50 }
  ],
  "xp_cost_per_rank": [500, 1200, 2500]
}
```

---

## 5. Adding a Custom Module

Module definitions live in `Content/Data/Modules/`.

```json
// Content/Data/Modules/overdrive_engine.json
{
  "$schema": "../../../Schemas/module.schema.json",
  "id": "overdrive_engine",
  "slot_type": "engine",
  "name": "Overdrive Engine",
  "description": "Massively boosts speed at the cost of hull integrity.",
  "tier": 3,
  "stats": {
    "speed": 50.0,
    "hull": -100
  },
  "power_draw": 35.0,
  "cost": { "Metal": 400, "Ore": 100 },
  "unlock_level": 8,
  "compatible_classes": ["fighter", "explorer"]
}
```

---

## 6. GraphVM Overview

The **GraphVM** is NovaForge's visual scripting runtime. It powers:

| Graph type | Used for |
|---|---|
| `GameFlow` | Mission scripting, cutscenes, world events |
| `Behavior` | NPC AI behavior trees |
| `Conversation` | Branching dialogue trees |
| `Animation` | Procedural animation blend logic |
| `UI` | Custom HUD widgets |

### 6.1 How It Works

- Graphs are authored visually in the **Graph Editor** panel (editor docked tab, future).
- At save time, the visual graph is compiled to deterministic bytecode stored in `.nfgraph`.
- At runtime, `GraphVM::Execute(graphId)` runs the bytecode on the server tick.
- Graphs are fully deterministic — given the same inputs and initial state, they always produce the same outputs. This is required for lockstep multiplayer.

### 6.2 Nodes Available in GameFlow Graphs

| Node | Purpose |
|---|---|
| `OnMissionStart` | Entry point; fires when the mission begins |
| `SetObjectiveActive` | Enable/disable an objective |
| `WaitForObjective` | Pause execution until an objective completes |
| `BranchOnFaction` | Branch on player reputation with a faction |
| `SpawnNPC` | Spawn an NPC at a location |
| `PlayDialogue` | Trigger a Conversation graph |
| `GiveReward` | Grant items, XP, or credits |
| `CompleteMission` | End the mission |
| `FailMission` | Fail the mission |

### 6.3 Creating a Graph

1. Open the NovaForge Editor.
2. In the Content Browser, right-click a folder and choose **New Graph → GameFlow**.
3. Name the graph to match your mission ID.
4. The Graph Editor panel opens. Drag nodes from the palette and connect pins.
5. Save (Ctrl+S). The `.nfgraph` bytecode file is written to `Content/Graphs/`.
6. Reference the path in your mission JSON `graph_script` field.

---

## 7. Packaging and Distributing Mods

A mod is simply a folder containing JSON data files and/or `.nfgraph` files. No packaging format is required for local play.

For distribution:

```
my_mod/
  Content/Data/Ships/my_ship.json
  Content/Data/Missions/my_mission.json
  Content/Graphs/Missions/my_mission.nfgraph
  README.md
```

Users place the mod folder at the repo/game root and the `AssetDatabase` will discover new files on next launch.

---

## 8. Schema Reference

All schemas are in `Schemas/`:

| Schema file | Validates |
|---|---|
| `ship.schema.json` | `Content/Data/Ships/*.json` |
| `mission.schema.json` | `Content/Data/Missions/*.json` |
| `module.schema.json` | `Content/Data/Modules/*.json` |
| `skill.schema.json` | `Content/Data/Skills/*.json` |

---

## 9. Troubleshooting

| Problem | Solution |
|---|---|
| Ship doesn't appear in shipyard | Check `id` is unique; validate JSON against schema |
| Mission doesn't trigger | Check `prerequisites` — all listed missions must be completed first |
| Graph script error | Check Graph Editor console for bytecode compile errors |
| Stats don't apply | Check `effects_per_rank` array length equals `max_rank` |

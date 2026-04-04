# NovaForge — Game Design Vision

**Version:** 0.2 (Reset Edition)  
**Status:** Active  

---

## 1. Elevator Pitch

NovaForge is a **voxel-based space game** where players build ships from first principles, explore procedurally generated sectors, fight in high-stakes fleet battles, and shape a living economy through industry and trade. Every ship is a voxel structure you design and fly. Every asteroid is mineable. Every station is built by players or NPCs.

---

## 2. Tri-Modal Gameplay

NovaForge has three distinct play modes that blend seamlessly:

### 2.1 FPS Mode (R.I.G. — Remote Infantry Gear)

The player inhabits a R.I.G. — a powered exosuit that operates in close-quarters environments:

- Explore ship interiors, asteroid caves, and space stations on foot
- Mine voxels directly with hand tools
- Build and repair structures block by block
- Engage in close-range combat with energy weapons and melee
- The R.I.G. is the game's primary "cursor" — the body through which the player interacts with the voxel world

### 2.2 Flight Mode (Ship Piloting)

The player pilots their custom voxel-built ship through space:

- Navigate between asteroid fields, stations, and sector jump points
- Dog-fight against pirates, rivals, and hostile factions
- Mine asteroids with ship-mounted drill arrays
- Trade cargo between stations
- The ship's performance stats are derived directly from the voxel blocks placed during construction

### 2.3 Fleet Mode (Strategic Command)

The player commands a fleet of AI-crewed ships as a strategic layer:

- Issue orders to fleet ships via a top-down tactical map
- Coordinate multi-ship attacks, mining operations, and supply runs
- Manage crew assignments and fleet logistics
- Fleet mode activates when the player has 3+ ships under command
- The player can drop back into Flight or FPS mode at any time during a fleet engagement

---

## 3. The Legend System

The **Legend System** is NovaForge's reputation and legacy mechanic:

- Every significant player action (first kill, first fleet victory, first jump to a new sector, first structure built) creates a **Legend Entry** — a permanent record in the game world's history.
- Legend Entries are visible to other players and NPCs; they shape how factions and individuals respond to you.
- Reaching milestone Legend levels unlocks unique ship blueprints, modules, and story missions.
- Legend is not transferable — it is tied to a single character and their choices.

**Legend Tiers:**

| Tier | Name | Unlock threshold |
|---|---|---|
| 0 | Unknown | Starting |
| 1 | Drifter | 100 Legend XP |
| 2 | Pioneer | 500 Legend XP |
| 3 | Forgemaster | 2,000 Legend XP |
| 4 | Sector Legend | 10,000 Legend XP |
| 5 | Void Sovereign | 50,000 Legend XP |

---

## 4. Economy Model

NovaForge uses a **player-driven resource economy**:

- Raw resources (Stone, Ore, Ice, Organic) are mined from the voxel world
- Refined materials (Metal, Crystal, Fuel) are produced by player-built refineries
- Ships, modules, and stations are constructed from materials — no "buy ships from NPC" shortcut
- Trade prices are set by supply and demand across sector markets
- Factions run competing trade networks; players can disrupt, support, or infiltrate them

**Resource flow:**
```
Mining → Raw Resources → Refinery → Materials → Shipyard → Ships/Modules
                                               → Constructor → Stations/Structures
```

---

## 5. Ship Class Catalogue

| Class | Role | Speed | Combat | Cargo | Example |
|---|---|---|---|---|---|
| Fighter | Dogfighting, escort | ★★★★★ | ★★★★ | ★ | Viper |
| Frigate | Fleet warfare, patrol | ★★★ | ★★★★★ | ★★ | Sentinel |
| Cruiser | Capital combat, anchor | ★ | ★★★★★ | ★★★ | Ironclad |
| Explorer | Survey, recon, long range | ★★★★ | ★★ | ★★★ | Pathfinder |
| Mining | Resource extraction | ★★ | ★ | ★★★★★ | Prospector |
| Transport | Cargo hauling, logistics | ★★ | ★ | ★★★★★ | (TBD) |
| Capital | Fleet command, fortress | ★ | ★★★★★ | ★★★★ | (TBD) |

All ship stats are derived from voxel block composition. A "Fighter" is a fighter because the designer placed lightweight hull blocks, high-thrust engine blocks, and weapon hardpoints — not because the game assigned it a class label.

---

## 6. Procedural Generation Principles

- **Sectors** are generated from a seed; the seed determines asteroid field density, resource distribution, NPC faction presence, and anomaly placement.
- **Chunks** use sine/cosine noise layered with octaves to produce natural terrain variation.
- **Ore deposits** are placed using a secondary noise pass weighted by depth and distance from sector centre.
- **Anomalies and story POIs** are placed deterministically from the mission manifest — they are always in the same location for a given world seed.

---

## 7. Multiplayer Model

NovaForge is **session-based co-op and PvP** with deterministic simulation:

- Session size: 2–20 players
- Network model: lockstep rollback (deterministic simulation)
- Players can form fleets together or compete in the same sector
- The server is authoritative; cheating is prevented by deterministic state verification
- Dedicated server (`NovaForgeServer.exe`) handles headless hosting

---

## 8. Core Design Rules

1. **Voxel is authoritative.** Every object in the world is either a voxel or anchored to one. There are no "free-floating" sprites or placeholder meshes in the shipped game.
2. **Player agency over everything.** No content is gated behind a paywall. The grind is the game.
3. **Systems not content.** Build flexible systems (economy, construction, AI) first. Content (ships, missions) emerges from those systems.
4. **No wasted code.** Every system built must be exercised in DevWorld before it goes to main.

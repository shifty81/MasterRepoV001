# DevWorld Authoring Plan

**Status:** Active  
**Applies to:** NovaForge Phase 1 (Dev World) and Phase 3 (First Interaction Loop)  
**Owner:** Editor / Gameplay team  

---

## 1. Purpose

This document defines the canonical workflow for authoring and iterating on **DevWorld** — the single reference world used to validate every game-loop system during development. DevWorld is never shipped to players; it is the fastest path from "code change" to "see it working in-game."

---

## 2. DevWorld Location

All DevWorld assets live under:

```
Content/Worlds/DevWorld/
  DevWorld.nfworld        ← primary world descriptor (JSON)
  Chunks/                 ← streamed chunk data (.nfchunk files)
  Objects/                ← placed world objects (.nfobj files)
  Lighting/               ← baked lighting data (future)
```

The project config references DevWorld as the default:

```json
// Config/novaforge.project.json
{
  "project": {
    "defaultWorld": "DevWorld"
  }
}
```

---

## 3. Session Lifecycle

### 3.1 Open

1. Launch `NovaForgeEditor.exe`.
2. Editor reads `Config/novaforge.project.json` → `defaultWorld = "DevWorld"`.
3. `EditorWorldSession::Open("Content/Worlds/DevWorld/DevWorld.nfworld")` is called during `EditorApp::Init`.
4. World chunks are loaded into the `ChunkManager`; placed objects are deserialized into the ECS `World`.
5. The main viewport renders the loaded world.

### 3.2 Edit

- Use the voxel editing tools (paint, fill, erase) to modify terrain chunks.
- Use the Object panel to place / remove world objects.
- Use the Inspector to modify properties of selected objects.
- Every modification is pushed onto the `CommandHistory` stack (supports Ctrl+Z / Ctrl+Y).
- The toolbar **Save** button becomes active (lit) as soon as `EditorWorldSession::IsDirty() == true`.

### 3.3 Save

1. User presses **Ctrl+S** or clicks the toolbar **Save** button.
2. `EditorWorldSession::Save()` serializes:
   - Dirty chunks → `Content/Worlds/DevWorld/Chunks/*.nfchunk`
   - Dirty objects → `Content/Worlds/DevWorld/Objects/*.nfobj`
   - World descriptor → `Content/Worlds/DevWorld/DevWorld.nfworld`
3. `CommandHistory` is cleared after a successful save.
4. The **Save** button returns to its inactive state.
5. A toast notification is displayed: "DevWorld saved."

### 3.4 Game Client Reload

After saving, the game client can be reloaded to see the same world state:

- **Hot reload (in-editor):** Press **Play** in the editor toolbar — spawns `NovaForgeGame.exe --world DevWorld` as a child process. The game client reads the freshly saved files.
- **Manual reload:** Close any running `NovaForgeGame.exe` instance, then re-run it. The game client always reads from `Content/Worlds/DevWorld/` at startup.

---

## 4. Dirty-State Propagation

The Save button state is driven every frame by the editor loop:

```
EditorApp::TickFrame():
    bool dirty = m_WorldSession.IsDirty();
    m_Toolbar.SetSaveEnabled(dirty);
```

`EditorWorldSession::IsDirty()` returns `true` if any of the following have changed since the last save:

- One or more `Chunk::IsDirty()` flags are set
- One or more world objects have `ObjectDirty` flag set
- The world descriptor metadata has changed

---

## 5. Play From Editor Launch Path

**Editor side:**
```cpp
// EditorApp — toolbar Play button handler
void EditorApp::OnPlayClicked()
{
    if (m_WorldSession.IsDirty())
        m_WorldSession.Save();   // auto-save before launch

    std::filesystem::path gameBin = GetExecutableDir() / "NovaForgeGame.exe";
    std::string cmdLine = gameBin.string() + " --world DevWorld";
    LaunchProcess(cmdLine);      // platform-specific process spawn
}
```

**Game side:**
```cpp
// NovaForgeGame/main.cpp — already parses --world flag
// GameClientApp::Init() calls WorldLoader::Load(worldName)
// WorldLoader resolves: Content/Worlds/<worldName>/<worldName>.nfworld
```

The `--world` argument is already wired into `GameClientApp::Init`; no new code is required on the game side.

---

## 6. DevWorld Required Contents (Phase 3 Interaction Loop)

For the Phase 3 first interaction loop to be fully testable, DevWorld must contain:

| Element | Description | Asset file |
|---|---|---|
| Starter R.I.G. spawn point | A `SpawnPoint` object with `entity_type = "RIG"` at position (0, 32, 0) | `Objects/starter_rig_spawn.nfobj` |
| Minable voxel cluster | A 4 × 4 × 4 region of `VoxelType::Ore` voxels at chunk (0,0,0), local (8,8,8)–(12,12,12) | Chunk data |
| Buildable surface | A flat 8 × 8 region of `VoxelType::Stone` at Y=30 with `BuildZone` component | `Objects/build_zone_01.nfobj` |
| Storage block | A `StorageBlock` world object at position (4, 32, 4) with 32-slot inventory | `Objects/storage_01.nfobj` |

The initial DevWorld JSON descriptor should match:

```json
// Content/Worlds/DevWorld/DevWorld.nfworld
{
  "version": 1,
  "name": "DevWorld",
  "spawn": { "x": 0, "y": 32, "z": 0 },
  "chunks": "Chunks/",
  "objects": "Objects/",
  "lighting": null,
  "ambient_light": 0.6
}
```

---

## 7. Smoke-Test Checklist

After any significant world or gameplay-system change, run this checklist before committing:

| # | Test | Expected result |
|---|---|---|
| 1 | Open editor, load DevWorld | No crash; world renders in viewport |
| 2 | Edit one voxel | Toolbar Save button becomes active |
| 3 | Press Ctrl+Z | Voxel reverts; Save button may remain active if other edits exist |
| 4 | Press Ctrl+S | Save button becomes inactive; toast "DevWorld saved" appears |
| 5 | Press Play | `NovaForgeGame.exe --world DevWorld` launches |
| 6 | In game: mine the Ore cluster | Ore voxels are removed; inventory fills with ore items |
| 7 | In game: place a block on build surface | Block appears at correct position |
| 8 | In game: deposit items into storage | Storage block inventory updates |
| 9 | Close game, reopen editor | Last saved state is visible; no data loss |

---

## 8. Iteration Guidelines

- **Never** commit a broken DevWorld to the main branch. If the world file becomes corrupt, restore from git.
- Keep chunk count low (≤ 9 chunks / 3 × 3 × 1 grid) during Phase 1–3 to keep load times under 1 second.
- World objects should use stable IDs (UUIDs) so references survive save/load round-trips.
- For multi-developer teams, treat `Content/Worlds/DevWorld/` as a shared mutable asset; communicate before making structural changes.

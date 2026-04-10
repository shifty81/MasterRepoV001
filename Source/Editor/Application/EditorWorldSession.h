#pragma once

#include "Game/World/GameWorld.h"
#include "Game/Gameplay/SolarSystem/DevSolarSystem.h"
#include "Game/Gameplay/PCG/PCGItemGen.h"
#include "Engine/World/Level.h"
#include <functional>
#include <string>

namespace NF::Editor {

/// @brief Encapsulates the editor's world lifecycle: create, save, load, reload.
///
/// EditorWorldSession centralises the paths and sequencing that previously
/// lived inline in EditorApp command lambdas.  It owns no subsystems itself
/// — it operates on references to the GameWorld and Level owned by EditorApp.
class EditorWorldSession {
public:
    /// @brief Initialise the session with references to the world and level.
    /// @param world       The GameWorld instance owned by EditorApp.
    /// @param level       The Level instance owned by EditorApp.
    /// @param contentRoot Root path for content (e.g. "Content").
    /// @param worldName   Name of the active world (e.g. "DevWorld").
    void Init(NF::Game::GameWorld& world, Level& level,
              const std::string& contentRoot, const std::string& worldName);

    /// @brief Set the solar system to save/load alongside the world.
    void SetSolarSystem(NF::Game::Gameplay::DevSolarSystem* sys) noexcept { m_SolarSystem = sys; }

    /// @brief Set the PCG item generator to save/load alongside the world.
    void SetItemGen(NF::Game::Gameplay::PCGItemGen* gen) noexcept { m_ItemGen = gen; }

    /// @brief Register a callback invoked after any world change (load/reload/new).
    ///
    /// EditorApp uses this to re-seed and regenerate the solar system whenever
    /// the active world changes without the session needing to know about PCG.
    void SetOnWorldChanged(std::function<void()> cb) noexcept { m_OnWorldChanged = std::move(cb); }

    /// @brief Create a fresh world, discarding any unsaved state.
    void NewWorld();

    /// @brief Load a world by name (e.g. "DevWorld", "TestWorld").
    /// @param worldName Name of the world definition file (without extension).
    void LoadWorld(const std::string& worldName);

    /// @brief Save the current world (entities + chunks + config + solar system) to disk.
    /// @return True if all saves succeed.
    bool Save();

    /// @brief Reload the world from the last saved state on disk.
    void Reload();

    /// @brief Load any saved chunk data over the generated terrain.
    /// Call after GameWorld::Initialize to overlay persisted edits.
    /// @return True if saved chunks were found and loaded.
    bool LoadSavedChunks();

    // ---- Queries ------------------------------------------------------------

    /// @brief True if the world has unsaved modifications.
    [[nodiscard]] bool IsDirty() const noexcept { return m_Dirty; }

    /// @brief Mark the world as dirty (modified since last save).
    void MarkDirty() noexcept { m_Dirty = true; }

    /// @brief Clear the dirty flag (e.g. after a successful save).
    void ClearDirty() noexcept { m_Dirty = false; }

    /// @brief Returns the entity save path (e.g. "Content/Worlds/DevWorld.nfsv").
    [[nodiscard]] const std::string& GetEntityPath() const noexcept { return m_EntityPath; }

    /// @brief Returns the chunk save path (e.g. "Content/Worlds/DevWorld.nfck").
    [[nodiscard]] const std::string& GetChunkPath() const noexcept { return m_ChunkPath; }

    /// @brief Returns the content root path.
    [[nodiscard]] const std::string& GetContentRoot() const noexcept { return m_ContentRoot; }

    /// @brief Returns the world name.
    [[nodiscard]] const std::string& GetWorldName() const noexcept { return m_WorldName; }

private:
    NF::Game::GameWorld* m_World{nullptr};
    Level*               m_Level{nullptr};
    NF::Game::Gameplay::DevSolarSystem* m_SolarSystem{nullptr};
    NF::Game::Gameplay::PCGItemGen*     m_ItemGen{nullptr};
    std::function<void()> m_OnWorldChanged;   ///< Fired after load/reload/new.
    std::string          m_ContentRoot;
    std::string          m_WorldName;
    std::string          m_EntityPath;
    std::string          m_ChunkPath;
    std::string          m_SolarSystemPath;
    std::string          m_ItemsPath;
    std::string          m_ConfigPath;
    bool                 m_Dirty{false};
};

} // namespace NF::Editor

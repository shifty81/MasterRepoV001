#include "Editor/Application/EditorWorldSession.h"
#include "Core/Logging/Log.h"

namespace NF::Editor {

void EditorWorldSession::Init(NF::Game::GameWorld& world, Level& level,
                               const std::string& contentRoot,
                               const std::string& worldName)
{
    m_World           = &world;
    m_Level           = &level;
    m_ContentRoot     = contentRoot;
    m_WorldName       = worldName;
    m_EntityPath      = contentRoot + "/Worlds/" + worldName + ".nfsv";
    m_ChunkPath       = contentRoot + "/Worlds/" + worldName + ".nfck";
    m_SolarSystemPath = contentRoot + "/Worlds/" + worldName + ".nfss";
    m_ItemsPath       = contentRoot + "/Worlds/" + worldName + ".nfpi";
    m_ConfigPath      = contentRoot + "/Definitions/" + worldName + ".json";
    m_Dirty           = false;

    NF::Logger::Log(NF::LogLevel::Info, "EditorWorldSession",
                    "Initialised — world: " + worldName
                    + ", content: " + contentRoot);
}

void EditorWorldSession::NewWorld()
{
    if (!m_World || !m_Level) return;

    NF::Logger::Log(NF::LogLevel::Info, "EditorWorldSession",
                    "Creating new world");
    m_World->Shutdown();
    m_World->Initialize(m_ContentRoot, m_WorldName);
    m_Level->Unload();
    m_Level->Load(m_WorldName);
    m_Dirty = false;
    NF::Logger::Log(NF::LogLevel::Info, "EditorWorldSession",
                    "New world ready");

    if (m_OnWorldChanged)
        m_OnWorldChanged();
}

void EditorWorldSession::LoadWorld(const std::string& worldName)
{
    if (!m_World || !m_Level) return;

    NF::Logger::Log(NF::LogLevel::Info, "EditorWorldSession",
                    "Loading world: " + worldName);

    m_World->Shutdown();
    m_WorldName       = worldName;
    m_EntityPath      = m_ContentRoot + "/Worlds/" + worldName + ".nfsv";
    m_ChunkPath       = m_ContentRoot + "/Worlds/" + worldName + ".nfck";
    m_SolarSystemPath = m_ContentRoot + "/Worlds/" + worldName + ".nfss";
    m_ItemsPath       = m_ContentRoot + "/Worlds/" + worldName + ".nfpi";
    m_ConfigPath      = m_ContentRoot + "/Definitions/" + worldName + ".json";

    m_World->Initialize(m_ContentRoot, worldName);
    LoadSavedChunks();
    m_Level->Unload();
    m_Level->Load(worldName);
    m_Dirty = false;

    NF::Logger::Log(NF::LogLevel::Info, "EditorWorldSession",
                    "World loaded: " + worldName);

    if (m_OnWorldChanged)
        m_OnWorldChanged();
}

bool EditorWorldSession::Save()
{
    if (!m_World) return false;

    NF::Logger::Log(NF::LogLevel::Info, "EditorWorldSession",
                    "Saving world...");
    const bool entityOk = m_World->SaveWorld(m_EntityPath);
    const bool chunkOk  = m_World->SaveChunks(m_ChunkPath);

    // Save world definition (spawn position, gravity, etc.) so changes
    // are picked up by the game on next launch.
    const bool configOk = m_World->GetConfig().SaveToFile(m_ConfigPath);

    // Save solar system data if one is wired up.
    bool solarOk = true;
    if (m_SolarSystem) {
        solarOk = m_SolarSystem->SaveToFile(m_SolarSystemPath);
        if (solarOk)
            NF::Logger::Log(NF::LogLevel::Info, "EditorWorldSession",
                            "Solar system saved");
    }

    // Save PCG item placements if an item generator is wired up.
    bool itemsOk = true;
    if (m_ItemGen) {
        itemsOk = m_ItemGen->SaveToFile(m_ItemsPath);
        if (itemsOk)
            NF::Logger::Log(NF::LogLevel::Info, "EditorWorldSession",
                            "PCG items saved ("
                            + std::to_string(m_ItemGen->ItemCount()) + " items)");
    }

    if (entityOk && chunkOk && configOk) {
        m_Dirty = false;
        NF::Logger::Log(NF::LogLevel::Info, "EditorWorldSession",
                        "World saved (entities + chunks + config"
                        + std::string(m_SolarSystem ? " + solar system" : "")
                        + std::string(m_ItemGen ? " + PCG items" : "")
                        + ")");
    } else {
        NF::Logger::Log(NF::LogLevel::Warning, "EditorWorldSession",
                        "World save incomplete — entities: "
                        + std::string(entityOk ? "ok" : "FAILED")
                        + ", chunks: "
                        + std::string(chunkOk ? "ok" : "FAILED")
                        + ", config: "
                        + std::string(configOk ? "ok" : "FAILED"));
    }

    return entityOk && chunkOk && configOk && solarOk && itemsOk;
}

void EditorWorldSession::Reload()
{
    if (!m_World || !m_Level) return;

    NF::Logger::Log(NF::LogLevel::Info, "EditorWorldSession",
                    "Reloading world...");
    m_World->Shutdown();
    m_World->Initialize(m_ContentRoot, m_WorldName);
    LoadSavedChunks();
    m_Level->Unload();
    m_Level->Load(m_WorldName);
    m_Dirty = false;
    NF::Logger::Log(NF::LogLevel::Info, "EditorWorldSession",
                    "World reloaded");

    if (m_OnWorldChanged)
        m_OnWorldChanged();
}

bool EditorWorldSession::LoadSavedChunks()
{
    if (!m_World) return false;

    bool chunksOk = false;
    if (m_World->LoadChunks(m_ChunkPath)) {
        chunksOk = true;
        NF::Logger::Log(NF::LogLevel::Info, "EditorWorldSession",
                        "Loaded saved chunk data ("
                        + std::to_string(m_World->GetLoadedChunkCount())
                        + " chunks)");
    }

    // Load PCG items if a file exists; it's non-fatal if it doesn't.
    if (m_ItemGen && m_ItemGen->LoadFromFile(m_ItemsPath)) {
        NF::Logger::Log(NF::LogLevel::Info, "EditorWorldSession",
                        "Loaded PCG items ("
                        + std::to_string(m_ItemGen->ItemCount()) + " items)");
    }

    return chunksOk;
}

} // namespace NF::Editor

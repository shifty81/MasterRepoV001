#include "Editor/Application/EditorWorldSession.h"
#include "Core/Logging/Log.h"

namespace NF::Editor {

void EditorWorldSession::Init(NF::Game::GameWorld& world, Level& level,
                               const std::string& contentRoot,
                               const std::string& worldName)
{
    m_World       = &world;
    m_Level       = &level;
    m_ContentRoot = contentRoot;
    m_WorldName   = worldName;
    m_EntityPath  = contentRoot + "/Worlds/" + worldName + ".nfsv";
    m_ChunkPath   = contentRoot + "/Worlds/" + worldName + ".nfck";
    m_Dirty       = false;

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
}

void EditorWorldSession::LoadWorld(const std::string& worldName)
{
    if (!m_World || !m_Level) return;

    NF::Logger::Log(NF::LogLevel::Info, "EditorWorldSession",
                    "Loading world: " + worldName);

    m_World->Shutdown();
    m_WorldName  = worldName;
    m_EntityPath = m_ContentRoot + "/Worlds/" + worldName + ".nfsv";
    m_ChunkPath  = m_ContentRoot + "/Worlds/" + worldName + ".nfck";

    m_World->Initialize(m_ContentRoot, worldName);
    LoadSavedChunks();
    m_Level->Unload();
    m_Level->Load(worldName);
    m_Dirty = false;

    NF::Logger::Log(NF::LogLevel::Info, "EditorWorldSession",
                    "World loaded: " + worldName);
}

bool EditorWorldSession::Save()
{
    if (!m_World) return false;

    NF::Logger::Log(NF::LogLevel::Info, "EditorWorldSession",
                    "Saving world...");
    const bool entityOk = m_World->SaveWorld(m_EntityPath);
    const bool chunkOk  = m_World->SaveChunks(m_ChunkPath);

    if (entityOk && chunkOk) {
        m_Dirty = false;
        NF::Logger::Log(NF::LogLevel::Info, "EditorWorldSession",
                        "World saved (entities + chunks)");
    } else {
        NF::Logger::Log(NF::LogLevel::Warning, "EditorWorldSession",
                        "World save incomplete — entities: "
                        + std::string(entityOk ? "ok" : "FAILED")
                        + ", chunks: "
                        + std::string(chunkOk ? "ok" : "FAILED"));
    }

    return entityOk && chunkOk;
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
}

bool EditorWorldSession::LoadSavedChunks()
{
    if (!m_World) return false;

    if (m_World->LoadChunks(m_ChunkPath)) {
        NF::Logger::Log(NF::LogLevel::Info, "EditorWorldSession",
                        "Loaded saved chunk data ("
                        + std::to_string(m_World->GetLoadedChunkCount())
                        + " chunks)");
        return true;
    }
    return false;
}

} // namespace NF::Editor

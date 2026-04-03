#include "Game/World/WorldFileService.h"
#include "Core/Logging/Log.h"

using NF::Logger;
using NF::LogLevel;

namespace nf
{
    bool WorldFileService::LoadWorld(const std::filesystem::path& path)
    {
        if (path.empty())
        {
            return false;
        }

        auto result = m_serializer.Load(path);
        if (!result.has_value()) {
            Logger::Log(LogLevel::Warning, "WorldFileService",
                        "Failed to load world from: " + path.string());
            return false;
        }

        m_worldData = std::move(result);
        m_state.activeWorldPath = path;
        m_state.dirty = false;
        Logger::Log(LogLevel::Info, "WorldFileService",
                    "Loaded world: " + m_worldData->displayName);
        return true;
    }

    bool WorldFileService::SaveWorld()
    {
        if (m_state.activeWorldPath.empty() || !m_worldData.has_value())
        {
            Logger::Log(LogLevel::Warning, "WorldFileService",
                        "Cannot save — no world loaded or no active path");
            return false;
        }

        if (!m_serializer.Save(m_state.activeWorldPath, *m_worldData)) {
            Logger::Log(LogLevel::Error, "WorldFileService",
                        "Failed to save world to: " + m_state.activeWorldPath.string());
            return false;
        }

        m_state.dirty = false;
        return true;
    }

    bool WorldFileService::SaveWorldAs(const std::filesystem::path& path)
    {
        if (path.empty() || !m_worldData.has_value())
        {
            return false;
        }

        if (!m_serializer.Save(path, *m_worldData)) {
            Logger::Log(LogLevel::Error, "WorldFileService",
                        "Failed to save world to: " + path.string());
            return false;
        }

        m_state.activeWorldPath = path;
        m_state.dirty = false;
        return true;
    }

    bool WorldFileService::ReloadWorld()
    {
        if (m_state.activeWorldPath.empty()) {
            Logger::Log(LogLevel::Warning, "WorldFileService",
                        "Cannot reload — no active world path");
            return false;
        }

        auto result = m_serializer.Load(m_state.activeWorldPath);
        if (!result.has_value()) {
            Logger::Log(LogLevel::Warning, "WorldFileService",
                        "Failed to reload world from: " + m_state.activeWorldPath.string());
            return false;
        }

        m_worldData = std::move(result);
        m_state.dirty = false;
        Logger::Log(LogLevel::Info, "WorldFileService",
                    "Reloaded world: " + m_worldData->displayName);
        return true;
    }

    const WorldFileState& WorldFileService::GetState() const noexcept
    {
        return m_state;
    }

    void WorldFileService::MarkDirty(bool dirty) noexcept
    {
        m_state.dirty = dirty;
    }

    const std::optional<DevWorldData>& WorldFileService::GetWorldData() const noexcept
    {
        return m_worldData;
    }

    DevWorldData& WorldFileService::GetMutableWorldData() noexcept
    {
        // Caller must ensure GetWorldData().has_value() before calling.
        return *m_worldData;
    }
}

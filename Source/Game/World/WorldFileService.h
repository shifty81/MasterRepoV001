#pragma once

#include "Game/World/DevWorldSerializer.h"
#include <filesystem>
#include <optional>
#include <string>

namespace nf
{
    struct WorldFileState
    {
        std::filesystem::path activeWorldPath;
        bool dirty = false;
    };

    /// @brief Service that manages loading, saving, and reloading a world
    ///        definition file through the DevWorldSerializer.
    class WorldFileService
    {
    public:
        bool LoadWorld(const std::filesystem::path& path);
        bool SaveWorld();
        bool SaveWorldAs(const std::filesystem::path& path);
        bool ReloadWorld();

        const WorldFileState& GetState() const noexcept;
        void MarkDirty(bool dirty = true) noexcept;

        /// @brief Returns the loaded world data, or nullopt if no world is loaded.
        [[nodiscard]] const std::optional<DevWorldData>& GetWorldData() const noexcept;

        /// @brief Returns a mutable reference to the loaded world data.
        /// @pre GetWorldData().has_value() must be true.
        [[nodiscard]] DevWorldData& GetMutableWorldData() noexcept;

    private:
        WorldFileState                m_state;
        DevWorldSerializer            m_serializer;
        std::optional<DevWorldData>   m_worldData;
    };
}

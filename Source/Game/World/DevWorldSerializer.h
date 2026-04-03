#pragma once

#include <array>
#include <filesystem>
#include <optional>
#include <string>

namespace nf
{
    /// @brief Complete DevWorld definition matching Content/Definitions/DevWorld.json.
    struct DevWorldData
    {
        // World identity
        std::string worldId          = "DevWorld";
        std::string displayName      = "Development Sandbox";
        std::string description;
        int         seed             = 42;
        std::array<float, 3> terrainSize = {256.f, 64.f, 256.f};
        float       gravity          = -9.81f;

        // Spawn
        std::array<float, 3> spawnPosition      = {128.f, 32.f, 128.f};
        std::array<float, 4> spawnRotation       = {0.f, 0.f, 0.f, 1.f};
        std::array<float, 3> spawnLookDirection  = {0.f, 0.f, 1.f};

        // Camera
        float fov             = 75.0f;
        float nearClip        = 0.1f;
        float farClip         = 1000.0f;
        float moveSpeed       = 10.0f;
        float lookSensitivity = 0.3f;
    };

    /// @brief Loads and saves DevWorld definition JSON files.
    ///
    /// Uses the same minimal inline JSON parsing approach as DevWorldConfig
    /// to avoid external dependencies.  Supports the full nested format of
    /// Content/Definitions/DevWorld.json for faithful round-trip serialization.
    class DevWorldSerializer
    {
    public:
        std::optional<DevWorldData> Load(const std::filesystem::path& path) const;
        bool Save(const std::filesystem::path& path, const DevWorldData& data) const;
    };
}

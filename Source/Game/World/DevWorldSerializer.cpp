#include "Game/World/DevWorldSerializer.h"
#include "Core/Logging/Log.h"

#include <fstream>
#include <sstream>

using NF::Logger;
using NF::LogLevel;

namespace nf
{

// ---------------------------------------------------------------------------
// Minimal JSON helpers — same pattern as DevWorldConfig.cpp
// ---------------------------------------------------------------------------

namespace {

std::string JsonString(const std::string& json, const std::string& key) {
    const std::string needle = "\"" + key + "\"";
    auto pos = json.find(needle);
    if (pos == std::string::npos) return {};
    pos = json.find(':', pos + needle.size());
    if (pos == std::string::npos) return {};
    pos = json.find('"', pos + 1);
    if (pos == std::string::npos) return {};
    auto end = json.find('"', pos + 1);
    if (end == std::string::npos) return {};
    return json.substr(pos + 1, end - pos - 1);
}

float JsonFloat(const std::string& json, const std::string& key, float fallback) {
    const std::string needle = "\"" + key + "\"";
    auto pos = json.find(needle);
    if (pos == std::string::npos) return fallback;
    pos = json.find(':', pos + needle.size());
    if (pos == std::string::npos) return fallback;
    ++pos;
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) ++pos;
    try { return std::stof(json.substr(pos)); }
    catch (...) { return fallback; }
}

bool JsonFloatArray(const std::string& json, const std::string& key,
                    float* out, size_t count) {
    const std::string needle = "\"" + key + "\"";
    auto pos = json.find(needle);
    if (pos == std::string::npos) return false;
    pos = json.find('[', pos + needle.size());
    if (pos == std::string::npos) return false;
    ++pos;
    for (size_t i = 0; i < count; ++i) {
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t'
               || json[pos] == '\r' || json[pos] == '\n')) ++pos;
        try {
            size_t consumed = 0;
            out[i] = std::stof(json.substr(pos), &consumed);
            pos += consumed;
        } catch (...) {
            return false;
        }
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == ','
               || json[pos] == '\t')) ++pos;
    }
    return true;
}

/// @brief Write a JSON float array value with proper formatting.
void WriteFloatArray(std::ostream& out, const float* values, size_t count) {
    out << "[";
    for (size_t i = 0; i < count; ++i) {
        if (i > 0) out << ", ";
        out << values[i];
    }
    out << "]";
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Load — real JSON parsing
// ---------------------------------------------------------------------------

std::optional<DevWorldData> DevWorldSerializer::Load(const std::filesystem::path& path) const
{
    if (path.empty())
        return std::nullopt;

    std::ifstream in(path);
    if (!in.is_open()) {
        Logger::Log(LogLevel::Warning, "DevWorldSerializer",
                    "Cannot open: " + path.string());
        return std::nullopt;
    }

    std::ostringstream ss;
    ss << in.rdbuf();
    const std::string json = ss.str();

    DevWorldData data{};

    // World identity
    {
        auto val = JsonString(json, "id");
        if (!val.empty()) data.worldId = val;
    }
    {
        auto val = JsonString(json, "displayName");
        if (!val.empty()) data.displayName = val;
    }
    {
        auto val = JsonString(json, "description");
        if (!val.empty()) data.description = val;
    }

    // World parameters
    data.seed    = static_cast<int>(JsonFloat(json, "seed", static_cast<float>(data.seed)));
    data.gravity = JsonFloat(json, "gravity", data.gravity);
    JsonFloatArray(json, "terrainSize", data.terrainSize.data(), 3);

    // Spawn
    JsonFloatArray(json, "position", data.spawnPosition.data(), 3);
    JsonFloatArray(json, "rotation", data.spawnRotation.data(), 4);
    JsonFloatArray(json, "lookDirection", data.spawnLookDirection.data(), 3);

    // Camera
    data.fov             = JsonFloat(json, "fov", data.fov);
    data.nearClip        = JsonFloat(json, "nearClip", data.nearClip);
    data.farClip         = JsonFloat(json, "farClip", data.farClip);
    data.moveSpeed       = JsonFloat(json, "moveSpeed", data.moveSpeed);
    data.lookSensitivity = JsonFloat(json, "lookSensitivity", data.lookSensitivity);

    Logger::Log(LogLevel::Info, "DevWorldSerializer",
                "Loaded: " + data.displayName + " (seed=" + std::to_string(data.seed) + ")");
    return data;
}

// ---------------------------------------------------------------------------
// Save — write full DevWorld.json format
// ---------------------------------------------------------------------------

bool DevWorldSerializer::Save(const std::filesystem::path& path, const DevWorldData& data) const
{
    if (path.empty())
        return false;

    std::ofstream out(path);
    if (!out.is_open()) {
        Logger::Log(LogLevel::Warning, "DevWorldSerializer",
                    "Cannot open for writing: " + path.string());
        return false;
    }

    out << "{\n";
    out << "  \"world\": {\n";
    out << "    \"id\": \"" << data.worldId << "\",\n";
    out << "    \"displayName\": \"" << data.displayName << "\",\n";
    out << "    \"description\": \"" << data.description << "\",\n";
    out << "    \"seed\": " << data.seed << ",\n";
    out << "    \"terrainSize\": ";
    WriteFloatArray(out, data.terrainSize.data(), 3);
    out << ",\n";
    out << "    \"gravity\": " << data.gravity << "\n";
    out << "  },\n";
    out << "  \"spawn\": {\n";
    out << "    \"position\": ";
    WriteFloatArray(out, data.spawnPosition.data(), 3);
    out << ",\n";
    out << "    \"rotation\": ";
    WriteFloatArray(out, data.spawnRotation.data(), 4);
    out << ",\n";
    out << "    \"lookDirection\": ";
    WriteFloatArray(out, data.spawnLookDirection.data(), 3);
    out << "\n";
    out << "  },\n";
    out << "  \"camera\": {\n";
    out << "    \"fov\": " << data.fov << ",\n";
    out << "    \"nearClip\": " << data.nearClip << ",\n";
    out << "    \"farClip\": " << data.farClip << ",\n";
    out << "    \"moveSpeed\": " << data.moveSpeed << ",\n";
    out << "    \"lookSensitivity\": " << data.lookSensitivity << "\n";
    out << "  }\n";
    out << "}\n";

    Logger::Log(LogLevel::Info, "DevWorldSerializer",
                "Saved: " + data.displayName + " to " + path.string());
    return true;
}

} // namespace nf

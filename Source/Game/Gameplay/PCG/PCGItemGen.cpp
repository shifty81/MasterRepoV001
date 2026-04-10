// PCGItemGen.cpp — Procedural item placement on flat maps.
//
// For each ResourceDeposit on a body, generates 1–4 placed items scattered
// near the deposit's world position.  User-edited items are preserved across
// regeneration by matching on ID.

#include "Game/Gameplay/PCG/PCGItemGen.h"
#include <cmath>
#include <algorithm>
#include <fstream>
#include <sstream>

namespace NF::Game::Gameplay {

// ---------------------------------------------------------------------------
// Hash
// ---------------------------------------------------------------------------

uint32_t PCGItemGen::Hash(uint32_t x) noexcept {
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
}

// ---------------------------------------------------------------------------
// Color per resource type
// ---------------------------------------------------------------------------

static uint32_t ResourceColor(NF::Game::ResourceType rt) noexcept {
    switch (rt) {
        case NF::Game::ResourceType::Stone:   return 0x888888FF;
        case NF::Game::ResourceType::Ore:     return 0xCC8844FF;
        case NF::Game::ResourceType::Dirt:    return 0x886644FF;
        case NF::Game::ResourceType::Rock:    return 0x666666FF;
        case NF::Game::ResourceType::Metal:   return 0xAABBCCFF;
        case NF::Game::ResourceType::Ice:     return 0x88DDFFFF;
        case NF::Game::ResourceType::Organic: return 0x44AA44FF;
        default:                              return 0xCCCCCCFF;
    }
}

// ---------------------------------------------------------------------------
// GenerateForBody
// ---------------------------------------------------------------------------

void PCGItemGen::GenerateForBody(const CelestialBody& body) {
    for (size_t di = 0; di < body.deposits.size(); ++di) {
        const auto& dep = body.deposits[di];

        // Each deposit spawns 1–4 items.
        const uint32_t depSeed = Hash(m_Seed ^ (body.id * 997) ^ static_cast<uint32_t>(di * 31));
        const uint32_t numItems = 1 + depSeed % 4;

        for (uint32_t ii = 0; ii < numItems; ++ii) {
            const uint32_t itemSeed = Hash(depSeed + ii * 7 + 1);

            PlacedItem item;
            item.id           = m_NextId++;
            item.name         = std::string(NF::Game::ResourceTypeName(dep.type))
                              + " #" + std::to_string(item.id);
            item.resourceType = dep.type;
            item.quantity     = 1 + Hash(itemSeed + 1) % 10;

            // Scatter near the deposit position (±5 units).
            const float offsetX = static_cast<float>(Hash(itemSeed + 2) % 100) * 0.1f - 5.f;
            const float offsetZ = static_cast<float>(Hash(itemSeed + 3) % 100) * 0.1f - 5.f;
            item.posX         = dep.worldX + offsetX;
            item.posZ         = dep.worldZ + offsetZ;

            item.color        = ResourceColor(dep.type);
            item.iconRadius   = 1.f + dep.abundance * 3.f;
            item.sourceBodyId = body.id;
            item.userEdited   = false;

            m_Items.push_back(std::move(item));
        }
    }
}

// ---------------------------------------------------------------------------
// GenerateForSystem
// ---------------------------------------------------------------------------

void PCGItemGen::GenerateForSystem(const DevSolarSystem& system) {
    // Preserve user-edited items.
    std::vector<PlacedItem> edited;
    for (auto& item : m_Items) {
        if (item.userEdited) edited.push_back(std::move(item));
    }

    m_Items.clear();
    m_NextId = 1;

    for (const auto& body : system.GetBodies()) {
        GenerateForBody(body);
    }

    // Restore user-edited items (matched by ID; if IDs shifted, append at end).
    for (auto& e : edited) {
        auto* existing = FindItem(e.id);
        if (existing) {
            *existing = std::move(e);
        } else {
            e.id = m_NextId++;
            m_Items.push_back(std::move(e));
        }
    }
}

// ---------------------------------------------------------------------------
// Lookup helpers
// ---------------------------------------------------------------------------

const PlacedItem* PCGItemGen::FindItem(uint32_t id) const noexcept {
    for (const auto& item : m_Items)
        if (item.id == id) return &item;
    return nullptr;
}

PlacedItem* PCGItemGen::FindItem(uint32_t id) noexcept {
    for (auto& item : m_Items)
        if (item.id == id) return &item;
    return nullptr;
}

std::vector<const PlacedItem*> PCGItemGen::ItemsForBody(uint32_t bodyId) const {
    std::vector<const PlacedItem*> result;
    for (const auto& item : m_Items)
        if (item.sourceBodyId == bodyId) result.push_back(&item);
    return result;
}

// ---------------------------------------------------------------------------
// SaveToFile — write placed items to a simple JSON file
// ---------------------------------------------------------------------------

bool PCGItemGen::SaveToFile(const std::string& path) const {
    std::ofstream file(path);
    if (!file.is_open()) return false;

    file << "{\n  \"seed\": " << m_Seed << ",\n  \"items\": [\n";
    for (size_t i = 0; i < m_Items.size(); ++i) {
        const auto& item = m_Items[i];
        file << "    {\n";
        file << "      \"id\": "           << item.id                          << ",\n";
        file << "      \"name\": \""       << item.name                        << "\",\n";
        file << "      \"resourceType\": " << static_cast<int>(item.resourceType) << ",\n";
        file << "      \"quantity\": "     << item.quantity                    << ",\n";
        file << "      \"posX\": "         << item.posX                        << ",\n";
        file << "      \"posZ\": "         << item.posZ                        << ",\n";
        file << "      \"color\": "        << item.color                       << ",\n";
        file << "      \"iconRadius\": "   << item.iconRadius                  << ",\n";
        file << "      \"sourceBodyId\": " << item.sourceBodyId                << ",\n";
        file << "      \"userEdited\": "   << (item.userEdited ? 1 : 0)        << "\n";
        file << "    }";
        if (i + 1 < m_Items.size()) file << ",";
        file << "\n";
    }
    file << "  ]\n}\n";
    return true;
}

// ---------------------------------------------------------------------------
// LoadFromFile — read placed items from a JSON file
// ---------------------------------------------------------------------------

namespace {

float PIJsonFloat(const std::string& json, const std::string& key, float fallback) {
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

uint32_t PIJsonUInt(const std::string& json, const std::string& key, uint32_t fallback) {
    return static_cast<uint32_t>(PIJsonFloat(json, key, static_cast<float>(fallback)));
}

std::string PIJsonString(const std::string& json, const std::string& key) {
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

} // anonymous namespace

bool PCGItemGen::LoadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return false;

    std::ostringstream ss;
    ss << file.rdbuf();
    const std::string json = ss.str();

    const uint32_t seed = PIJsonUInt(json, "seed", m_Seed);
    m_Seed = seed;

    m_Items.clear();
    m_NextId = 1;

    const std::string itemsKey = "\"items\"";
    auto arrStart = json.find(itemsKey);
    if (arrStart == std::string::npos) return false;
    arrStart = json.find('[', arrStart);
    if (arrStart == std::string::npos) return false;

    size_t searchPos = arrStart + 1;
    while (searchPos < json.size()) {
        auto objStart = json.find('{', searchPos);
        if (objStart == std::string::npos) break;

        // Find matching close brace.
        int depth = 0;
        size_t objEnd = objStart;
        for (size_t p = objStart; p < json.size(); ++p) {
            if (json[p] == '{') ++depth;
            else if (json[p] == '}') { --depth; if (depth == 0) { objEnd = p; break; } }
        }
        if (objEnd <= objStart) break;

        const std::string block = json.substr(objStart, objEnd - objStart + 1);

        PlacedItem item;
        item.id           = PIJsonUInt(block, "id", 0);
        item.name         = PIJsonString(block, "name");
        item.resourceType = static_cast<NF::Game::ResourceType>(PIJsonUInt(block, "resourceType", 0));
        item.quantity     = PIJsonUInt(block, "quantity", 1);
        item.posX         = PIJsonFloat(block, "posX", 0.f);
        item.posZ         = PIJsonFloat(block, "posZ", 0.f);
        item.color        = PIJsonUInt(block, "color", 0xCCCCCCFF);
        item.iconRadius   = PIJsonFloat(block, "iconRadius", 2.f);
        item.sourceBodyId = PIJsonUInt(block, "sourceBodyId", 0);
        item.userEdited   = PIJsonUInt(block, "userEdited", 0) != 0;

        if (item.id >= m_NextId) m_NextId = item.id + 1;
        m_Items.push_back(std::move(item));
        searchPos = objEnd + 1;
    }

    // Return true if the file was opened and parsed — even an empty items array
    // is a valid state (e.g. a brand-new world with no deposits yet).
    return true;
}

} // namespace NF::Game::Gameplay

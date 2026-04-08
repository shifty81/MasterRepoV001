// PCGItemGen.cpp — Procedural item placement on flat maps.
//
// For each ResourceDeposit on a body, generates 1–4 placed items scattered
// near the deposit's world position.  User-edited items are preserved across
// regeneration by matching on ID.

#include "Game/Gameplay/PCG/PCGItemGen.h"
#include <cmath>
#include <algorithm>

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

} // namespace NF::Game::Gameplay

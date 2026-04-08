#pragma once
// PCGItemGen.h — Procedural content generation for items on a flat map.
//
// Given a CelestialBody (with its deposits), generates a flat list of
// placed items with world positions.  Each placed item is editable —
// the editor can override any property and the changes persist.

#include "Game/Gameplay/SolarSystem/DevSolarSystem.h"
#include "Game/Interaction/ResourceItem.h"
#include <cstdint>
#include <string>
#include <vector>

namespace NF::Game::Gameplay {

// ---------------------------------------------------------------------------
// PlacedItem — a single item placed on the flat map.
// ---------------------------------------------------------------------------

struct PlacedItem {
    uint32_t                id{0};           ///< Unique ID within the generation batch.
    std::string             name;            ///< Display name.
    NF::Game::ResourceType  resourceType{NF::Game::ResourceType::None};
    uint32_t                quantity{1};     ///< Stack count.

    // Flat-map position (world space).
    float                   posX{0.f};
    float                   posZ{0.f};

    // Visual
    uint32_t                color{0xCCCCCCFF};   ///< 0xRRGGBBAA display colour.
    float                   iconRadius{2.f};     ///< Radius for flat-map drawing.

    // Edit state
    bool                    userEdited{false};   ///< True if any property was manually changed.

    /// @brief Source body ID that generated this item (for traceability).
    uint32_t                sourceBodyId{0};
};

// ---------------------------------------------------------------------------
// PCGItemGen — procedural item placement generator.
// ---------------------------------------------------------------------------

class PCGItemGen {
public:
    PCGItemGen() = default;

    /// @brief Set the generation seed (usually matches the solar system seed).
    void SetSeed(uint32_t seed) noexcept { m_Seed = seed; }
    [[nodiscard]] uint32_t GetSeed() const noexcept { return m_Seed; }

    /// @brief Generate placed items for a single celestial body.
    ///
    /// Each ResourceDeposit on the body produces 1–4 items scattered near
    /// the deposit's position.  Items that were previously user-edited are
    /// preserved (matched by ID) so manual overrides survive regeneration.
    void GenerateForBody(const CelestialBody& body);

    /// @brief Generate placed items for all bodies in a solar system.
    void GenerateForSystem(const DevSolarSystem& system);

    /// @brief Return all placed items.
    [[nodiscard]] const std::vector<PlacedItem>& GetItems() const noexcept { return m_Items; }
    [[nodiscard]] std::vector<PlacedItem>& GetItems() noexcept { return m_Items; }

    /// @brief Find a placed item by ID.
    [[nodiscard]] const PlacedItem* FindItem(uint32_t id) const noexcept;
    [[nodiscard]] PlacedItem* FindItem(uint32_t id) noexcept;

    /// @brief Total item count.
    [[nodiscard]] size_t ItemCount() const noexcept { return m_Items.size(); }

    /// @brief Clear all generated items.
    void Clear() noexcept { m_Items.clear(); m_NextId = 1; }

    /// @brief Return items for a specific body.
    [[nodiscard]] std::vector<const PlacedItem*> ItemsForBody(uint32_t bodyId) const;

private:
    uint32_t                m_Seed{42};
    std::vector<PlacedItem> m_Items;
    uint32_t                m_NextId{1};

    /// @brief Simple deterministic hash.
    [[nodiscard]] static uint32_t Hash(uint32_t x) noexcept;
};

} // namespace NF::Game::Gameplay

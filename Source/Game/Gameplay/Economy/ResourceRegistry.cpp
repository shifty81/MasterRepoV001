#include "Game/Gameplay/Economy/ResourceRegistry.h"
#include <cstring>

namespace NF::Game::Gameplay {

ResourceRegistry::ResourceRegistry()
{
    using RT = NF::Game::ResourceType;

    m_Defs[0] = {RT::None,    "None",    0.f,   0.f,  false, false};
    m_Defs[1] = {RT::Stone,   "Stone",   1.f,   0.5f, true,  false};
    m_Defs[2] = {RT::Ore,     "Ore",     3.f,   1.0f, true,  false};
    m_Defs[3] = {RT::Dirt,    "Dirt",    0.5f,  0.3f, true,  false};
    m_Defs[4] = {RT::Rock,    "Rock",    1.5f,  0.8f, true,  false};
    m_Defs[5] = {RT::Metal,   "Metal",   8.f,   2.0f, true,  true };
    m_Defs[6] = {RT::Ice,     "Ice",     2.f,   0.4f, true,  false};
    m_Defs[7] = {RT::Organic, "Organic", 4.f,   0.2f, true,  false};
}

const ResourceDef& ResourceRegistry::Get(NF::Game::ResourceType type) const noexcept
{
    const auto idx = static_cast<int>(type);
    if (idx >= 0 && idx < kCount)
        return m_Defs[static_cast<size_t>(idx)];
    return m_Defs[0]; // None
}

const ResourceDef& ResourceRegistry::FindByName(const char* name) const noexcept
{
    for (const auto& def : m_Defs)
        if (std::strcmp(def.name, name) == 0)
            return def;
    return m_Defs[0]; // None
}

} // namespace NF::Game::Gameplay

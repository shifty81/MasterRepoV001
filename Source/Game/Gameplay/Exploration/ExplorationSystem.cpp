// ExplorationSystem.cpp — Body-to-body travel skeleton.
#include "Game/Gameplay/Exploration/ExplorationSystem.h"

namespace NF::Game::Gameplay {

bool ExplorationSystem::TravelToBody(uint32_t bodyId)
{
    if (bodyId == m_CurrentBodyId)
        return false; // Already on this body — no travel needed.

    m_PreviousBodyId = m_CurrentBodyId;
    m_CurrentBodyId  = bodyId;
    m_Traveling      = false; // Phase 5: instant travel; async reserved for later.

    if (m_OnTravelComplete)
        m_OnTravelComplete(bodyId);

    return true;
}

} // namespace NF::Game::Gameplay


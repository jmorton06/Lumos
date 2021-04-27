#include "Precompiled.h"
#include "Entity.h"
#include "EntityManager.h"

namespace Lumos
{
    Entity EntityManager::Create()
    {
        return Entity(m_Registry.create(), m_Scene);
    }

    Entity EntityManager::Create(const std::string& name)
    {
        auto e = m_Registry.create();
        m_Registry.emplace<NameComponent>(e, name);
        return Entity(e, m_Scene);
    }

    void EntityManager::Clear()
    {
        m_Registry.each([&](auto entity)
            { m_Registry.destroy(entity); });

        m_Registry.clear();
    }
}

#include "Precompiled.h"
#include "Entity.h"
#include "EntityManager.h"
#include "Maths/Random.h"

namespace Lumos
{
    Entity EntityManager::Create()
    {
        LUMOS_PROFILE_FUNCTION();
        auto e = m_Registry.create();
        m_Registry.emplace<IDComponent>(e);
        return Entity(e, m_Scene);
    }

    Entity EntityManager::Create(const std::string& name)
    {
        LUMOS_PROFILE_FUNCTION();
        auto e = m_Registry.create();
        m_Registry.emplace<NameComponent>(e, name);
        m_Registry.emplace<IDComponent>(e);
        return Entity(e, m_Scene);
    }

    void EntityManager::Clear()
    {
        LUMOS_PROFILE_FUNCTION();
        for(auto [entity] : m_Registry.storage<entt::entity>().each())
        {
            m_Registry.destroy(entity);
        }

        m_Registry.clear();
    }

    Entity EntityManager::GetEntityByUUID(uint64_t id)
    {
        LUMOS_PROFILE_FUNCTION();

        auto view = m_Registry.view<IDComponent>();
        for(auto entity : view)
        {
            auto& idComponent = m_Registry.get<IDComponent>(entity);
            if(idComponent.ID == id)
                return Entity(entity, m_Scene);
        }

        LUMOS_LOG_WARN("Entity not found by ID");
        return Entity {};
    }
}

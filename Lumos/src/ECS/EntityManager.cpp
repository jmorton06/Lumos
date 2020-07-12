#include "lmpch.h"
#include "Entity.h"
#include "EntityManager.h"

namespace Lumos {
    
    Entity EntityManager::Create()
    {
        return Entity(m_Registry.create(), m_Scene);
    }
    
    void EntityManager::Clear()
    {
        m_Registry.each([&](auto entity) {
            m_Registry.destroy(entity);
        });

        m_Registry.clear();
    }
}

#pragma once

namespace Lumos
{
    class SystemManager;
    class EntityManager;
    class ComponentManager;
    
    class EntityComponentSystem
    {
        EntityComponentSystem() = default;
        ~EntityComponentSystem() = default;
        
    private:
        std::unique_ptr<SystemManager> m_SystemManager;
        std::unique_ptr<EntityManager> m_EntityManager;
        std::unique_ptr<ComponentManager> m_ComponentManager;
    };
}

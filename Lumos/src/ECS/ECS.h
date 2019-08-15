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
        Scope<SystemManager> m_SystemManager;
        Scope<EntityManager> m_EntityManager;
        Scope<ComponentManager> m_ComponentManager;
    };
}

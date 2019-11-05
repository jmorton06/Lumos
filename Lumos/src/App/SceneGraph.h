#pragma once

#include <entt/entt.hpp>

namespace Lumos
{
    struct ParentComponent
    {
        entt::entity parent;
    };

    class SceneGraph
    {
    public:
        SceneGraph() {}
        ~SceneGraph() {}
        
        void Update();
        
        void AddParent(const entt::entity& child, const entt::entity& parent);
        
        const entt::registry& GetResistry() const { return m_Registry; }
        entt::registry& GetResistry() { return m_Registry; }

    private:
        entt::registry m_Registry;
    };
}

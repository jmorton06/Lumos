#pragma once

#include <entt/entt.hpp>

namespace Lumos
{
    struct ParentComponent
    {
        entt::entity parent;
    };

	class Hierarchy {
	public:
		Hierarchy(entt::entity p = entt::null) : _parent{ p } {}

		inline entt::entity parent() const { return _parent; }
		inline entt::entity next() const { return _next; }
		inline entt::entity prev() const { return _prev; }
		inline entt::entity first() const { return _first; }

		// Return true if rhs is an ancestor of rhs
		bool compare(const entt::registry& registry, const entt::entity rhs) const;

		// update hierarchy components when hierarchy component is added
		static void on_construct(entt::entity entity, entt::registry& registry, Hierarchy& hierarchy);

		// update hierarchy components when hierarchy component is removed
		static void on_destroy(entt::entity entity, entt::registry& registry);

		static void on_replace(entt::entity entity, entt::registry& registry);

		static void Reparent(entt::entity entity, entt::entity parent, entt::registry& registry, Hierarchy& hierarchy);

		entt::entity _parent = entt::null;
		entt::entity _first = entt::null;
		entt::entity _next = entt::null;
		entt::entity _prev = entt::null;
	};

    class SceneGraph
    {
    public:
		SceneGraph(entt::registry& registry);
        ~SceneGraph() {}
        
        void Update();
        
        const entt::registry& GetResistry() const { return m_Registry; }
        entt::registry& GetResistry() { return m_Registry; }

    private:
        entt::registry m_Registry;
    };
}

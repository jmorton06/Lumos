#pragma once

#include "Entity.h"
#include <entt/entt.hpp>

namespace Lumos
{

	class Scene;
	class Entity;

	template<class T>
	class EntityView
	{
		entt::view<T> m_View;
	};

	template<typename... Components>
	class EntityGroup
	{
		entt::sparse_set<entt::entity>::iterator begin()
		{
			return m_Group.begin();
		}
		entt::sparse_set<entt::entity>::iterator end()
		{
			return m_Group.end();
		}

		entt::group<Components...> m_Group;
	};

	template<typename...>
	struct TypeList
	{
	};

	class EntityManager
	{
	public:
		EntityManager(Scene* scene)
			: m_Scene(scene)
		{
		}

		Entity Create();
		Entity Create(const std::string& name);

		template<typename... Components>
		auto GetEntitiesWithTypes()
		{
			return m_Registry.group<Components...>();
		}

		template<typename Component>
		auto GetEntitiesWithType()
		{
			return m_Registry.view<Component>();
		}

		template<typename R, typename T>
		void AddDependency()
		{
			m_Registry.template on_construct<R>().template connect<&entt::registry::get_or_emplace<T>>();
		}

		entt::registry& GetRegistry()
		{
			return m_Registry;
		}

		void Clear();

	private:
		Scene* m_Scene = nullptr;
		entt::registry m_Registry;
	};
}

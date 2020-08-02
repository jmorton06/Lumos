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
    public:
		EntityView(Scene* scene)
			: m_Scene(scene), m_View(scene->GetRegistry().view<T>())
		{
		}
    
        Entity operator[](int i)
		{
			LUMOS_ASSERT(i < Size(), "Index out of range on Entity View");
			Entity(m_View[i], m_Scene);
		}

        bool Empty() const { return m_View.empty(); }
        size_t Size() const { return m_View.size(); }
		Entity Front() { return Entity(m_View[0], m_Scene); }
    private:
        Scene* m_Scene;
		entt::basic_view<entt::entity, entt::exclude_t<>, T> m_View;
	};

	template<typename... Components>
	class EntityGroup
	{
	public:
		EntityGroup(Scene* scene)
			: m_Scene(scene)
			, m_Group(scene->GetRegistry().group<Components...>())
		{
		}

		Entity operator[](int i)
		{
			LUMOS_ASSERT(i < Size(), "Index out of range on Entity View");
			Entity(m_Group[i], m_Scene);
		}

		size_t Size() const
		{
			return m_Group.size();
		}
		Entity Front()
		{
			return Entity(m_Group[0], m_Scene);
		}

	private:
		Scene* m_Scene;
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
		EntityView<Component> GetEntitiesWithType()
		{
			return EntityView<Component>(m_Scene);
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

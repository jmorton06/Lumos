#pragma once
#include "Maths/Transform.h"
#include "Scene/Scene.h"
#include "Scene/SceneGraph.h"
#include "Core/Profiler.h"
#include <entt/entt.hpp>

namespace Lumos
{

	class Entity
	{
	public:
    
        Entity() = default;

		Entity(entt::entity handle, Scene* scene)
			: m_EntityHandle(handle)
			, m_Scene(scene)
		{
		}

		~Entity()
		{
		}

		template<typename T, typename... Args>
		T& AddComponent(Args&&... args)
		{
#ifdef LUMOS_DEBUG
            if(HasComponent<T>())
                LUMOS_LOG_WARN("Attamptin to add Component twice");
#endif
			return m_Scene->GetRegistry().emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
		}

		template<typename T, typename... Args>
		T& GetOrAddComponent(Args&&... args)
		{
			return m_Scene->GetRegistry().get_or_emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
		}
    
        template<typename T, typename... Args>
        void AddOrReplaceComponent(Args&&... args)
        {
            m_Scene->GetRegistry().emplace_or_replace<T>(m_EntityHandle, std::forward<Args>(args)...);
        }

		template<typename T>
		T& GetComponent()
		{
			return m_Scene->GetRegistry().get<T>(m_EntityHandle);
		}

		template<typename T>
		T* TryGetComponent()
		{
			return m_Scene->GetRegistry().try_get<T>(m_EntityHandle);
		}

		template<typename T>
		bool HasComponent()
		{
			return m_Scene->GetRegistry().has<T>(m_EntityHandle);
		}

		template<typename T>
		void RemoveComponent()
		{
			return m_Scene->GetRegistry().remove<T>(m_EntityHandle);
		}

		bool Active()
		{
			LUMOS_PROFILE_FUNCTION();
			if(HasComponent<ActiveComponent>())
				return m_Scene->GetRegistry().get<ActiveComponent>(m_EntityHandle).active;

			return true;
		}

		void SetActive(bool isActive)
		{
			LUMOS_PROFILE_FUNCTION();
			GetOrAddComponent<ActiveComponent>().active = isActive;
		}

		Maths::Transform& GetTransform()
		{
			return m_Scene->GetRegistry().get<Maths::Transform>(m_EntityHandle);
		}

		const Maths::Transform& GetTransform() const
		{
			return m_Scene->GetRegistry().get<Maths::Transform>(m_EntityHandle);
		}

		void SetParent(Entity entity)
		{
			LUMOS_PROFILE_FUNCTION();
			bool acceptable = false;
			auto hierarchyComponent = TryGetComponent<Hierarchy>();
			if(hierarchyComponent != nullptr)
			{
				acceptable = entity.m_EntityHandle != m_EntityHandle && (!entity.IsParent(*this)) && (hierarchyComponent->parent() != m_EntityHandle);
			}
			else
				acceptable = entity.m_EntityHandle != m_EntityHandle;

			if(!acceptable)
				return;

			if(hierarchyComponent)
				Hierarchy::Reparent(m_EntityHandle, entity.m_EntityHandle, m_Scene->GetRegistry(), *hierarchyComponent);
			else
			{
				m_Scene->GetRegistry().emplace<Hierarchy>(m_EntityHandle, entity.m_EntityHandle);
			}
		}

		Entity GetParent()
		{
			LUMOS_PROFILE_FUNCTION();
			auto hierarchyComp = TryGetComponent<Hierarchy>();
			if(hierarchyComp)
				return Entity(hierarchyComp->parent(), m_Scene);
			else
				return Entity(entt::null, nullptr);
			
		}

		std::vector<Entity> GetChildren()
		{
			LUMOS_PROFILE_FUNCTION();
			std::vector<Entity> children;
			auto hierarchyComponent = TryGetComponent<Hierarchy>();
			if(hierarchyComponent)
			{
				entt::entity child = hierarchyComponent->first();
				while(child != entt::null && m_Scene->GetRegistry().valid(child))
				{
					children.emplace_back(child, m_Scene);
					hierarchyComponent = m_Scene->GetRegistry().try_get<Hierarchy>(child);
					if(hierarchyComponent)
						child = hierarchyComponent->next();
				}
			}
				
			return children;
		}

		bool IsParent(Entity potentialParent)
		{
			LUMOS_PROFILE_FUNCTION();
			auto nodeHierarchyComponent = m_Scene->GetRegistry().try_get<Hierarchy>(m_EntityHandle);
			if(nodeHierarchyComponent)
			{
				auto parent = nodeHierarchyComponent->parent();
				while(parent != entt::null)
				{
					if(parent == potentialParent.m_EntityHandle)
					{
						return true;
					}
					else
					{
						nodeHierarchyComponent = m_Scene->GetRegistry().try_get<Hierarchy>(parent);
						parent = nodeHierarchyComponent ? nodeHierarchyComponent->parent() : entt::null;
					}
				}
			}

			return false;
		}

		operator u32() const
		{
			return (u32)m_EntityHandle;
		}

		operator bool() const
		{
			return m_EntityHandle != entt::null && m_Scene;
		}

		bool operator==(const Entity& other) const
		{
			return m_EntityHandle == other.m_EntityHandle && m_Scene == other.m_Scene;
		}

		bool operator!=(const Entity& other) const
		{
			return !(*this == other);
		}

		entt::entity GetHandle() const 
		{
			return m_EntityHandle;
		}

		void Destroy()
		{
			m_Scene->GetRegistry().destroy(m_EntityHandle);
		}

		bool Valid()
		{
			return m_Scene->GetRegistry().valid(m_EntityHandle) && m_Scene;
		}

	private:
		entt::entity m_EntityHandle = entt::null;
		Scene* m_Scene = nullptr;

		friend class EntityManager;
	};
}

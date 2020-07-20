#pragma once
#include "Maths/Transform.h"
#include "Scene/Scene.h"
#include "Scene/SceneGraph.h"
#include <entt/entt.hpp>

namespace Lumos
{

	class Entity
	{
	public:
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
			return m_Scene->GetRegistry().emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
		}

		template<typename T, typename... Args>
		T& GetOrAddComponent(Args&&... args)
		{
			return m_Scene->GetRegistry().get_or_emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
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
			if(HasComponent<ActiveComponent>())
				return m_Scene->GetRegistry().get<ActiveComponent>(m_EntityHandle).active;

			return true;
		}

		void SetActive(bool isActive)
		{
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

		operator u32() const
		{
			return (u32)m_EntityHandle;
		}
		operator bool() const
		{
			return (u32)m_EntityHandle && m_Scene;
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

	private : 
		Entity() = default;
		Entity(const std::string& name);

	private:
		entt::entity m_EntityHandle;
		Scene* m_Scene = nullptr;

		friend class EntityManager;
	};
}

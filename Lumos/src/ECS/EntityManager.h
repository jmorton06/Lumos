#pragma once

#include "lmpch.h"
#include "ECS.h"
#include "ECSDefines.h"
#include "Utilities/TSingleton.h"
#include "Component/Components.h"
#include "ComponentManager.h"
#include "Maths/Transform.h"

#include "Core/Serialisable.h"

namespace Lumos
{
	class EntityManager;
	class LUMOS_EXPORT Entity : public Serialisable
	{
		friend class EntityManager;
	public:
		template<typename T, typename... Args>
		void AddComponent(Args&&... args);

		template <typename T, typename... Args>
		T* GetOrAddComponent(Args&&... args);

		template <typename T>
		T* GetComponent();

		template <typename T>
		bool HasComponent();

		template <typename T>
		void RemoveComponent();

		void RemoveComponent(size_t id);

		virtual void OnUpdateObject(float dt);
		virtual void OnImGui();
		virtual void Init();

		const std::vector<Entity*>& GetChildren() const { return m_Children; }

		void AddChild(Entity* child);
		void RemoveChild(Entity* child);

		u32& GetFrustumCullFlags() { return m_FrustumCullFlags; }

		Maths::Transform* GetTransformComponent();

		void SetParent(Entity* parent);

		const String& GetName() const { return m_Name; }
		const u32 GetUUID() const { return m_UUID; }

		const bool Active() const { return m_Active; }
		const bool ActiveInHierarchy() const;
		void SetActive(bool active) { m_Active = active; };
		void SetActiveRecursive(bool active);

		nlohmann::json Serialise() override;
		void Deserialise(nlohmann::json& data) override;

	protected:
		explicit Entity(EntityManager* manager, const String& name = "");
		virtual ~Entity();
	private:

		NONCOPYABLE(Entity)

			String                  m_Name;
		u32						m_UUID;
		EntityManager*			m_Manager;
		String                  m_PrefabFileLocation;
		bool                    m_Active;
		u32                     m_FrustumCullFlags;

		Entity* m_Parent;
		std::vector<Entity*> m_Children;
	};

	template<typename T, typename ... Args>
	_FORCE_INLINE_ void Entity::AddComponent(Args && ...args)
	{
		ComponentManager::Instance()->AddComponent<T>(this, std::forward<Args>(args) ...);
	}

	template<typename T, typename ... Args>
	_FORCE_INLINE_ T* Entity::GetOrAddComponent(Args && ...args)
	{
		T* component = GetComponent<T>();

		if (component != nullptr)
			return component;
		else
		{
			AddComponent<T>(std::forward<Args>(args) ...);

			return ComponentManager::Instance()->GetComponent<T>(this);
		}
	}

	template <typename T>
	_FORCE_INLINE_ T* Entity::GetComponent()
	{
		return ComponentManager::Instance()->GetComponent<T>(this);
	}

	template <typename T>
	_FORCE_INLINE_ void Entity::RemoveComponent()
	{
		ComponentManager::Instance()->RemoveComponent<T>(this);
	}

	_FORCE_INLINE_ void Entity::RemoveComponent(size_t id)
	{
		ComponentManager::Instance()->RemoveComponent(this, id);
	}

	template <typename T>
	_FORCE_INLINE_ bool Entity::HasComponent()
	{
		return ComponentManager::Instance()->HasComponent<T>(this);
	}

	class LUMOS_EXPORT EntityManager : public TSingleton<EntityManager>
	{
		friend class TSingleton<EntityManager>;
		friend class Entity;
	public:
		EntityManager() = default;
        ~EntityManager();

		void Clear();
		Entity* CreateEntity(const String& name = "Entity");
		void DeleteEntity(Entity* entity);

		Entity* GetEntity(u32 uuid);

		const std::vector<Entity*>& GetEntities() const { return m_Entities; }

		template<typename T>
		std::vector<Entity*> GetEntitiesWithType()
		{
			std::vector<Entity*> entities;

			for (auto entity : m_Entities)
			{
				if (entity->HasComponent<T>())
					entities.emplace_back(entity);
			}

			return entities;
		}

		template<typename T, typename S>
		std::vector<Entity*> GetEntitiesWithType()
		{
			std::vector<Entity*> entities;

			for (auto entity : m_Entities)
			{
				if (entity->HasComponent<T>() && entity->HasComponent<S>())
					entities.emplace_back(entity);
			}

			return entities;
		}

	private:
		std::vector<Entity*> m_Entities;
	};
}

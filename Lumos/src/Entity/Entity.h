#pragma once

#include "LM.h"
#include "Component/Components.h"
#include "ComponentManager.h"

struct EnumClassHash
{
    template <typename T>
    std::size_t operator()(T t) const
    {
        return static_cast<std::size_t>(t);
    }
};

namespace Lumos
{
	class LUMOS_EXPORT Entity
	{
		friend class EntityManager;
	public:
		template<typename T, typename... Args>
		void AddComponent(Args&&... args);
        
        template <typename T, typename... Args>
        T* GetOrAddComponent(Args&&... args);

		template <typename T>
		T* GetComponent()
		{
			return ComponentManager::Instance()->GetComponent<T>(this);
		}
        
        template <typename T>
        void RemoveComponent()
        {
			ComponentManager::Instance()->RemoveComponent<T>(this);
        }

		virtual void OnUpdateObject(float dt);
		virtual void OnIMGUI();
		virtual void OnGuizmo(u32 mode = 0);
		virtual void Init();

		std::vector<Entity*>& GetChildren() { return m_Children; }
		void AddChild(Entity* child);
		void RemoveChild(Entity* child);

		void  SetBoundingRadius(float radius) { m_BoundingRadius = radius; }
		float GetBoundingRadius() const { return m_BoundingRadius; }

		u32& GetFrustumCullFlags() { return m_FrustumCullFlags; }

		void DebugDraw(uint64 debugFlags);

		TransformComponent* GetTransformComponent();

		void SetParent(Entity* parent);

		const String& GetName() const { return m_Name; }
		const String& GetUUID() const { return m_UUID; }

		const bool Active() const { return m_Active; }
		const bool ActiveInHierarchy() const;
		void SetActive(bool active) { m_Active = active; };
		void SetActiveRecursive(bool active);

		std::vector<LumosComponent*> GetAllComponents();

	protected:
		explicit Entity(const String& name = "");
		virtual ~Entity();
	private:

		Entity(Entity const&) = delete;
		Entity& operator=(Entity const&) = delete;

		String					m_Name;
		float					m_BoundingRadius;
		u32						m_FrustumCullFlags;
		String                  m_UUID;
		bool					m_Active;
		TransformComponent*		m_DefaultTransformComponent = nullptr;
		
		Entity* m_Parent;
		std::vector<Entity*> m_Children;
	};

	template<typename T, typename ... Args>
	inline void Entity::AddComponent(Args && ...args)
	{
		auto component = new T(std::forward<Args>(args) ...);
		component->SetEntity(this);
		component->Init();

		if (ComponentManager::Instance()->GetComponentType<T>() == ComponentManager::Instance()->GetComponentType<TransformComponent>())
			m_DefaultTransformComponent = reinterpret_cast<TransformComponent*>(component);
		
		ComponentManager::Instance()->AddComponent<T>(this, component);
	}
    
    template<typename T, typename ... Args>
    inline T* Entity::GetOrAddComponent(Args && ...args)
    {
        T* component = GetComponent<T>();
        
        if(component != nullptr)
            return component;
		else
		{
			component = new T(std::forward<Args>(args) ...);
			component->SetEntity(this);
			component->Init();

			if (ComponentManager::Instance()->GetComponentType<T>() == ComponentManager::Instance()->GetComponentType<TransformComponent>())
				m_DefaultTransformComponent = reinterpret_cast<TransformComponent*>(component);

			ComponentManager::Instance()->AddComponent<T>(this, component);

			return component;
		}
        
		
    }
}

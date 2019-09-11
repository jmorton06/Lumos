#pragma once

#include "lmpch.h"
#include "ECS.h"
#include "ECSDefines.h"
#include "Utilities/TSingleton.h"
#include "Component/Components.h"
#include "ComponentManager.h"

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
        void RemoveComponent();
        
        virtual void OnUpdateObject(float dt);
        virtual void OnImGui();
        virtual void OnGuizmo(u32 mode = 0);
        virtual void Init();
        
        const std::vector<Entity*>& GetChildren() const { return m_Children; }

        void AddChild(Entity* child);
        void RemoveChild(Entity* child);
        
        void  SetBoundingRadius(float radius) { m_BoundingRadius = radius; }
        float GetBoundingRadius() const { return m_BoundingRadius; }
        u32& GetFrustumCullFlags() { return m_FrustumCullFlags; }
        
        TransformComponent* GetTransformComponent();
        
        void SetParent(Entity* parent);
        
        const String& GetName() const { return m_Name; }
        const String& GetUUID() const { return m_UUID; }
        
        const bool Active() const { return m_Active; }
        const bool ActiveInHierarchy() const;
        void SetActive(bool active) { m_Active = active; };
        void SetActiveRecursive(bool active);
        
        const std::vector<LumosComponent*> GetAllComponents();
        
        nlohmann::json Serialise() override;
        void Deserialise(nlohmann::json& data) override;
        
    protected:
        explicit Entity(EntityManager* manager, const String& name = "");
        virtual ~Entity();
    private:
        
        NONCOPYABLE(Entity)
        
        String                  m_Name;
        float                   m_BoundingRadius;
        String                  m_UUID;
        String                  m_PrefabFileLocation;
        bool                    m_Active;
        u32                     m_FrustumCullFlags;
        TransformComponent*     m_DefaultTransformComponent = nullptr;
        
        Entity* m_Parent;
        EntityManager* m_Manager;
        std::vector<Entity*> m_Children;
    };

    template<typename T, typename ... Args>
    inline void Entity::AddComponent(Args && ...args)
    {
        auto component = lmnew T(std::forward<Args>(args) ...);
        
        if (ComponentManager::Instance()->GetComponentType<T>() == ComponentManager::Instance()->GetComponentType<TransformComponent>())
            m_DefaultTransformComponent = reinterpret_cast<TransformComponent*>(component);
        
        ComponentManager::Instance()->AddComponent<T>(this, component);
    }

    template<typename T, typename ... Args>
    inline T* Entity::GetOrAddComponent(Args && ...args)
    {
        T* component = GetComponent<T>();
        
        if (component != nullptr)
            return component;
        else
        {
            component = lmnew T(std::forward<Args>(args) ...);
  
            if (ComponentManager::Instance()->GetComponentType<T>() == ComponentManager::Instance()->GetComponentType<TransformComponent>())
                m_DefaultTransformComponent = reinterpret_cast<TransformComponent*>(component);
            
            ComponentManager::Instance()->AddComponent<T>(this, component);
            
            return component;
        }
    }

    template <typename T>
    inline T* Entity::GetComponent()
    {
        return ComponentManager::Instance()->GetComponent<T>(this);
    }

    template <typename T>
    inline void Entity::RemoveComponent()
    {
        ComponentManager::Instance()->RemoveComponent<T>(this);
    }


	class LUMOS_EXPORT EntityManager : public TSingleton<EntityManager>
	{
		friend class TSingleton<EntityManager>;
		friend class Entity;
	public:
		EntityManager() = default;
        ~EntityManager();

		void Clear();
		Entity* CreateEntity(const String& name = "");
		void DeleteEntity(Entity* entity);

		const std::vector<Entity*>& GetEntities() const { return m_Entities; }

	private:
		std::vector<Entity*> m_Entities;
	};
}

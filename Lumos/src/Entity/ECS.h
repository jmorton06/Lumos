#pragma once
#include "LM.h"
#include "ComponentManager.h"
#include "EntityManager.h"
#include "SystemManager.h"
#include "Utilities/TSingleton.h"

namespace Lumos
{
    class LUMOS_EXPORT ECS : public TSingleton<ECS>
    {
        friend class TSingleton<ECS>;
    public:
        ECS() = default;
        ~ECS() { }
        
        void Init()
        {
            // Create pointers to each manager
            m_ComponentManager = std::make_unique<ComponentManager>();
            m_EntityManager = std::make_unique<EntityManager>();
            m_SystemManager = std::make_unique<SystemManager>();
        }
        
        template<typename T>
        void RegisterComponent()
        {
            m_ComponentManager->RegisterComponent<T>();
        }

        Entity* CreateEntity(const String& name = "") { return m_EntityManager->CreateEntity(name); };
        
        void DestroyEntity(Entity* entity)
        {
            m_EntityManager->DestroyEntity(entity);
            m_ComponentManager->EntityDestroyed(entity);
            m_SystemManager->EntityDestroyed(entity);
        }
        
        template<typename T>
        void AddComponent(Entity* entity, T component)
        {
            m_ComponentManager->AddComponent<T>(entity, component);
            
            auto signature = m_EntityManager->GetSignature(entity);
            signature.set(m_ComponentManager->GetComponentType<T>(), true);
            m_EntityManager->SetSignature(entity, signature);
            
            m_SystemManager->EntitySignatureChanged(entity, signature);
        }
        
        template<typename T>
        void RemoveComponent(Entity* entity)
        {
            m_ComponentManager->RemoveComponent<T>(entity);
            
            auto signature = m_EntityManager->GetSignature(entity);
            signature.set(m_ComponentManager->GetComponentType<T>(), false);
            m_EntityManager->SetSignature(entity, signature);
            
            m_SystemManager->EntitySignatureChanged(entity, signature);
        }
        
        // System methods
        template<typename T>
        std::shared_ptr<T> RegisterSystem()
        {
            return m_SystemManager->RegisterSystem<T>();
        }
        
        template<typename T>
        void SetSystemSignature(Signature signature)
        {
            m_SystemManager->SetSignature<T>(signature);
        }
        
        void Clear() { m_EntityManager->Clear(); }
        
    private:
        std::unique_ptr<ComponentManager> m_ComponentManager;
        std::unique_ptr<EntityManager> m_EntityManager;
        std::unique_ptr<SystemManager> m_SystemManager;
    };
}

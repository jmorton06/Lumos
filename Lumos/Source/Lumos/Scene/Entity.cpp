#include "Precompiled.h"
#include "Entity.h"
#include "Maths/Transform.h"
#include "Scene/SceneGraph.h"

namespace Lumos
{
        bool Entity::Active()
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            bool active = true;
            if(HasComponent<ActiveComponent>())
                active = m_Scene->GetRegistry().get<ActiveComponent>(m_EntityHandle).active;

            auto parent = GetParent();
            if(parent)
                active &= parent.Active();
            return active;
        }

        void Entity::SetActive(bool isActive)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            GetOrAddComponent<ActiveComponent>().active = isActive;
        }

        Maths::Transform& Entity::GetTransform()
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            return m_Scene->GetRegistry().get<Maths::Transform>(m_EntityHandle);
        }

        const Maths::Transform& Entity::GetTransform() const
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            return m_Scene->GetRegistry().get<Maths::Transform>(m_EntityHandle);
        }

        uint64_t Entity::GetID()
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            return m_Scene->GetRegistry().get<IDComponent>(m_EntityHandle).ID;
        }

        const std::string& Entity::GetName()
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            auto nameComponent = TryGetComponent<NameComponent>();

            if(nameComponent)
                return nameComponent->name;
            else
            {
                static std::string tempName = "Entity";
                return tempName;
            }
        }

        void Entity::SetParent(Entity entity)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            bool acceptable         = false;
            auto hierarchyComponent = TryGetComponent<Hierarchy>();
            if(hierarchyComponent != nullptr)
            {
                acceptable = entity.m_EntityHandle != m_EntityHandle && (!entity.IsParent(*this)) && (hierarchyComponent->Parent() != m_EntityHandle);
            }
            else
                acceptable = entity.m_EntityHandle != m_EntityHandle;

            if(!acceptable)
            {
                LWARN("Failed to parent entity!");
                return;
            }

            if(hierarchyComponent)
                Hierarchy::Reparent(m_EntityHandle, entity.m_EntityHandle, m_Scene->GetRegistry(), *hierarchyComponent);
            else
            {
                m_Scene->GetRegistry().emplace<Hierarchy>(m_EntityHandle, entity.m_EntityHandle);
            }
        }

        Entity Entity::GetParent()
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            auto hierarchyComp = TryGetComponent<Hierarchy>();
            if(hierarchyComp)
                return Entity(hierarchyComp->Parent(), m_Scene);
            else
                return Entity(entt::null, nullptr);
        }

        TDArray<Entity> Entity::GetChildren()
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            TDArray<Entity> children;
            auto hierarchyComponent = TryGetComponent<Hierarchy>();
            if(hierarchyComponent)
            {
                entt::entity child = hierarchyComponent->First();
                while(child != entt::null && m_Scene->GetRegistry().valid(child))
                {
                    children.EmplaceBack(child, m_Scene);
                    hierarchyComponent = m_Scene->GetRegistry().try_get<Hierarchy>(child);
                    if(hierarchyComponent)
                        child = hierarchyComponent->Next();
                }
            }

            return children;
        }

        void Entity::ClearChildren()
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            auto hierarchyComponent = TryGetComponent<Hierarchy>();
            if(hierarchyComponent)
            {
                hierarchyComponent->m_First = entt::null;
            }
        }

        bool Entity::IsParent(Entity potentialParent)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            auto nodeHierarchyComponent = m_Scene->GetRegistry().try_get<Hierarchy>(m_EntityHandle);
            if(nodeHierarchyComponent)
            {
                auto parent = nodeHierarchyComponent->Parent();
                while(parent != entt::null)
                {
                    if(parent == potentialParent.m_EntityHandle)
                    {
                        return true;
                    }
                    else
                    {
                        nodeHierarchyComponent = m_Scene->GetRegistry().try_get<Hierarchy>(parent);
                        parent                 = nodeHierarchyComponent ? nodeHierarchyComponent->Parent() : entt::null;
                    }
                }
            }

            return false;
        }
        
         void Entity::Destroy()
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            m_Scene->GetRegistry().destroy(m_EntityHandle);
        }

        bool Entity::Valid()
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            return m_Scene && m_Scene->GetRegistry().valid(m_EntityHandle);
        }


}
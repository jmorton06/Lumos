#include "Precompiled.h"
#include "Entity.h"
#include "Maths/Transform.h"
#include "Scene/SceneGraph.h"
#include "Core/Application.h"

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
        return m_Scene->GetRegistry().get_or_emplace<IDComponent>(m_EntityHandle).ID;
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

    Entity* Entity::GetChildren(Arena* arena)
    {
        LUMOS_PROFILE_FUNCTION_LOW();

        Entity* children = nullptr;
        u32 childIndex   = 0;

        auto hierarchyComponent = TryGetComponent<Hierarchy>();
        if(hierarchyComponent)
        {
            u32 childCount = 0;
            // TODO: remove
            {
                entt::entity child = hierarchyComponent->First();
                while(child != entt::null && m_Scene->GetRegistry().valid(child))
                {
                    childCount++;

                    hierarchyComponent = m_Scene->GetRegistry().try_get<Hierarchy>(child);
                    if(hierarchyComponent)
                        child = hierarchyComponent->Next();
                }
            }

            hierarchyComponent               = TryGetComponent<Hierarchy>();
            hierarchyComponent->m_ChildCount = childCount;

            children           = PushArrayNoZero(arena, Entity, childCount);
            entt::entity child = hierarchyComponent->First();
            while(child != entt::null && m_Scene->GetRegistry().valid(child))
            {
                children[childIndex] = { child, m_Scene };
                childIndex++;

                hierarchyComponent = m_Scene->GetRegistry().try_get<Hierarchy>(child);
                if(hierarchyComponent)
                    child = hierarchyComponent->Next();
            }
        }

        return children;
    }

    Entity* Entity::GetChildrenTemp()
    {
        return GetChildren(Application::Get().GetFrameArena());
    }

    u32 Entity::GetChildCount()
    {
        auto hierarchyComponent = TryGetComponent<Hierarchy>();
        if(hierarchyComponent)
        {
            return hierarchyComponent->m_ChildCount;
        }

        return 0;
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

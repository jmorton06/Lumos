#include "Precompiled.h"
#include "SceneGraph.h"
#include "Maths/Transform.h"

DISABLE_WARNING_PUSH
DISABLE_WARNING_CONVERSION_TO_SMALLER_TYPE
#include <entt/entity/registry.hpp>
DISABLE_WARNING_POP

namespace Lumos
{
    Hierarchy::Hierarchy(entt::entity p)
        : m_Parent(p)
    {
        m_First = entt::null;
        m_Next  = entt::null;
        m_Prev  = entt::null;
    }

    Hierarchy::Hierarchy()
    {
        m_Parent = entt::null;
        m_First  = entt::null;
        m_Next   = entt::null;
        m_Prev   = entt::null;
    }

    SceneGraph::SceneGraph()
    {
    }

    void SceneGraph::Init(entt::registry& registry)
    {
        registry.on_construct<Hierarchy>().connect<&Hierarchy::OnConstruct>();
        registry.on_update<Hierarchy>().connect<&Hierarchy::OnUpdate>();
        registry.on_destroy<Hierarchy>().connect<&Hierarchy::OnDestroy>();
    }

    void SceneGraph::Update(entt::registry& registry)
    {
        LUMOS_PROFILE_FUNCTION();
        auto nonHierarchyView = registry.view<Maths::Transform>(entt::exclude<Hierarchy>);

        for(auto entity : nonHierarchyView)
        {
            registry.get<Maths::Transform>(entity).SetWorldMatrix(glm::mat4(1.0f));
        }

        auto view = registry.view<Hierarchy>();
        for(auto entity : view)
        {
            const auto hierarchy = registry.try_get<Hierarchy>(entity);
            if(hierarchy && hierarchy->Parent() == entt::null)
            {
                // Recursively update children
                UpdateTransform(entity, registry);
            }
        }
    }

    void SceneGraph::UpdateTransform(entt::entity entity, entt::registry& registry)
    {
        LUMOS_PROFILE_FUNCTION();
        auto hierarchyComponent = registry.try_get<Hierarchy>(entity);
        if(hierarchyComponent)
        {
            auto transform = registry.try_get<Maths::Transform>(entity);
            if(transform)
            {
                if(hierarchyComponent->Parent() != entt::null)
                {
                    auto parentTransform = registry.try_get<Maths::Transform>(hierarchyComponent->Parent());
                    if(parentTransform)
                    {
                        transform->SetWorldMatrix(parentTransform->GetWorldMatrix());
                    }
                    else
                    {
                        transform->SetWorldMatrix(glm::mat4(1.0f));
                    }
                }
                else
                {
                    transform->SetWorldMatrix(glm::mat4(1.0f));
                }
            }

            entt::entity child = hierarchyComponent->First();
            while(child != entt::null)
            {
                auto hierarchyComponent = registry.try_get<Hierarchy>(child);
                auto next               = hierarchyComponent ? hierarchyComponent->Next() : entt::null;
                UpdateTransform(child, registry);
                child = next;
            }
        }
    }

    void Hierarchy::Reparent(entt::entity entity, entt::entity parent, entt::registry& registry, Hierarchy& hierarchy)
    {
        LUMOS_PROFILE_FUNCTION();
        Hierarchy::OnDestroy(registry, entity);

        hierarchy.m_Parent = entt::null;
        hierarchy.m_Next   = entt::null;
        hierarchy.m_Prev   = entt::null;

        if(parent != entt::null)
        {
            hierarchy.m_Parent = parent;
            Hierarchy::OnConstruct(registry, entity);
        }
    }

    bool Hierarchy::Compare(const entt::registry& registry, const entt::entity rhs) const
    {
        LUMOS_PROFILE_FUNCTION();
        if(rhs == entt::null || rhs == m_Parent || rhs == m_Prev)
        {
            return true;
        }
        else
        {
            if(m_Parent == entt::null)
            {
                return false;
            }
            else
            {
                auto& this_parent_h = registry.get<Hierarchy>(m_Parent);
                auto& rhs_h         = registry.get<Hierarchy>(rhs);
                if(this_parent_h.Compare(registry, rhs_h.m_Parent))
                {
                    return true;
                }
            }
        }
        return false;
    }

    void Hierarchy::Reset()
    {
        m_Parent = entt::null;
        m_First  = entt::null;
        m_Next   = entt::null;
        m_Prev   = entt::null;
    }

    void Hierarchy::OnConstruct(entt::registry& registry, entt::entity entity)
    {
        LUMOS_PROFILE_FUNCTION();
        auto& hierarchy = registry.get<Hierarchy>(entity);
        if(hierarchy.m_Parent != entt::null)
        {
            auto& parent_hierarchy = registry.get_or_emplace<Hierarchy>(hierarchy.m_Parent);

            if(parent_hierarchy.m_First == entt::null)
            {
                parent_hierarchy.m_First = entity;
            }
            else
            {
                // get last children
                auto prev_ent          = parent_hierarchy.m_First;
                auto current_hierarchy = registry.try_get<Hierarchy>(prev_ent);
                while(current_hierarchy != nullptr && current_hierarchy->m_Next != entt::null)
                {
                    prev_ent          = current_hierarchy->m_Next;
                    current_hierarchy = registry.try_get<Hierarchy>(prev_ent);
                }
                // add new
                current_hierarchy->m_Next = entity;
                hierarchy.m_Prev          = prev_ent;
            }
            // sort
            //			registry.sort<Hierarchy>([&registry](const entt::entity lhs, const entt::entity rhs) {
            //				auto& right_h = registry.get<Hierarchy>(rhs);
            //				auto result = right_h.Compare(registry, lhs);
            //				return result;
            //			});
        }
    }

    void DeleteChildren(entt::entity parent, entt::registry& registry)
    {
        LUMOS_PROFILE_FUNCTION();
        auto hierarchy = registry.try_get<Hierarchy>(parent);

        if(hierarchy)
        {
            entt::entity child = hierarchy->First();
            while(child != entt::null)
            {
                DeleteChildren(child, registry);
                hierarchy = registry.try_get<Hierarchy>(child);
                registry.destroy(child);

                if(hierarchy)
                {
                    child = hierarchy->Next();
                }
            }
        }
    }

    void Hierarchy::OnUpdate(entt::registry& registry, entt::entity entity)
    {
        LUMOS_PROFILE_FUNCTION();
        auto& hierarchy = registry.get<Hierarchy>(entity);
        // if is the first child
        if(hierarchy.m_Prev == entt::null)
        {
            if(hierarchy.m_Parent != entt::null)
            {
                auto parent_hierarchy = registry.try_get<Hierarchy>(hierarchy.m_Parent);
                if(parent_hierarchy != nullptr)
                {
                    parent_hierarchy->m_First = hierarchy.m_Next;
                    if(hierarchy.m_Next != entt::null)
                    {
                        auto next_hierarchy = registry.try_get<Hierarchy>(hierarchy.m_Next);
                        if(next_hierarchy != nullptr)
                        {
                            next_hierarchy->m_Prev = entt::null;
                        }
                    }
                }
            }
        }
        else
        {
            auto prev_hierarchy = registry.try_get<Hierarchy>(hierarchy.m_Prev);
            if(prev_hierarchy != nullptr)
            {
                prev_hierarchy->m_Next = hierarchy.m_Next;
            }
            if(hierarchy.m_Next != entt::null)
            {
                auto next_hierarchy = registry.try_get<Hierarchy>(hierarchy.m_Next);
                if(next_hierarchy != nullptr)
                {
                    next_hierarchy->m_Prev = hierarchy.m_Prev;
                }
            }
        }

        // sort
        //		registry.sort<Hierarchy>([&registry](const entt::entity lhs, const entt::entity rhs)
        //		{
        //			auto& right_h = registry.get<Hierarchy>(rhs);
        //			return right_h.Compare(registry, lhs);
        //		});
    }

    void Hierarchy::OnDestroy(entt::registry& registry, entt::entity entity)
    {
        LUMOS_PROFILE_FUNCTION();
        auto& hierarchy = registry.get<Hierarchy>(entity);
        // if is the first child
        if(hierarchy.m_Prev == entt::null || !registry.valid(hierarchy.m_Prev))
        {
            if(hierarchy.m_Parent != entt::null && registry.valid(hierarchy.m_Parent))
            {
                auto parent_hierarchy = registry.try_get<Hierarchy>(hierarchy.m_Parent);
                if(parent_hierarchy != nullptr)
                {
                    parent_hierarchy->m_First = hierarchy.m_Next;
                    if(hierarchy.m_Next != entt::null)
                    {
                        auto next_hierarchy = registry.try_get<Hierarchy>(hierarchy.m_Next);
                        if(next_hierarchy != nullptr)
                        {
                            next_hierarchy->m_Prev = entt::null;
                        }
                    }
                }
            }
        }
        else
        {
            auto prev_hierarchy = registry.try_get<Hierarchy>(hierarchy.m_Prev);
            if(prev_hierarchy != nullptr)
            {
                prev_hierarchy->m_Next = hierarchy.m_Next;
            }
            if(hierarchy.m_Next != entt::null)
            {
                auto next_hierarchy = registry.try_get<Hierarchy>(hierarchy.m_Next);
                if(next_hierarchy != nullptr)
                {
                    next_hierarchy->m_Prev = hierarchy.m_Prev;
                }
            }
        }

        // sort
        //		registry.sort<Hierarchy>([&registry](const entt::entity lhs, const entt::entity rhs)
        //		{
        //			auto& right_h = registry.get<Hierarchy>(rhs);
        //			return right_h.Compare(registry, lhs);
        //		});
    }

    void SceneGraph::DisableOnConstruct(bool disable, entt::registry& registry)
    {
        LUMOS_PROFILE_FUNCTION();
        if(disable)
            registry.on_construct<Hierarchy>().disconnect<&Hierarchy::OnConstruct>();
        else
            registry.on_construct<Hierarchy>().connect<&Hierarchy::OnConstruct>();
    }

}

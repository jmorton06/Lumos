#pragma once
#include "Scene/Scene.h"
#include "Core/Profiler.h"
#include "Core/UUID.h"

DISABLE_WARNING_PUSH
DISABLE_WARNING_CONVERSION_TO_SMALLER_TYPE
#include <entt/entity/registry.hpp>
DISABLE_WARNING_POP

namespace Lumos
{
    namespace Maths
    {
        class Transform;
    }

    struct IDComponent
    {
        UUID ID;

        template <typename Archive>
        void save(Archive& archive) const
        {
            uint64_t uuid = (uint64_t)ID;
            archive(uuid);
        }

        template <typename Archive>
        void load(Archive& archive)
        {
            uint64_t uuid;
            archive(uuid);

            ID = UUID(uuid);
        }
    };

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

        template <typename T, typename... Args>
        T& AddComponent(Args&&... args)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
#ifdef LUMOS_DEBUG
            if(HasComponent<T>())
                LWARN("Attempting to add Component twice");
#endif
            return m_Scene->GetRegistry().emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
        }

        template <typename T, typename... Args>
        T& GetOrAddComponent(Args&&... args)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            return m_Scene->GetRegistry().get_or_emplace<T>(m_EntityHandle, std::forward<Args>(args)...);
        }

        template <typename T, typename... Args>
        void AddOrReplaceComponent(Args&&... args)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            m_Scene->GetRegistry().emplace_or_replace<T>(m_EntityHandle, std::forward<Args>(args)...);
        }

        template <typename T>
        T& GetComponent()
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            return m_Scene->GetRegistry().get<T>(m_EntityHandle);
        }

        template <typename T>
        T* TryGetComponent()
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            return m_Scene->GetRegistry().try_get<T>(m_EntityHandle);
        }

        template <typename T>
        bool HasComponent()
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            return m_Scene->GetRegistry().all_of<T>(m_EntityHandle);
        }

        template <typename T>
        void RemoveComponent()
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            m_Scene->GetRegistry().remove<T>(m_EntityHandle);
        }

        template <typename T>
        void TryRemoveComponent()
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            if(HasComponent<T>())
                RemoveComponent<T>();
        }

        bool Active();
        void SetActive(bool isActive);
        Maths::Transform& GetTransform();
        const Maths::Transform& GetTransform() const;
        uint64_t GetID();
        const std::string& GetName();
        void SetParent(Entity entity);
        Entity GetParent();

        Entity* GetChildren(Arena* arena);
        Entity* GetChildrenTemp(); // Using Frame Arena
        u32 GetChildCount();

        void ClearChildren();
        bool IsParent(Entity potentialParent);
        void Destroy();
        bool Valid();

        operator entt::entity() const
        {
            return m_EntityHandle;
        }

        operator uint32_t() const
        {
            return (uint32_t)m_EntityHandle;
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

        Scene* GetScene() const { return m_Scene; }

    private:
        entt::entity m_EntityHandle = entt::null;
        Scene* m_Scene              = nullptr;

        friend class EntityManager;
    };
}

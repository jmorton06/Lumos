#pragma once

#include "Utilities/TSingleton.h"
#include "Scene/Entity.h"

#include <sol/sol.hpp>

DISABLE_WARNING_PUSH
DISABLE_WARNING_CONVERSION_TO_SMALLER_TYPE
#include <entt/entt.hpp>
DISABLE_WARNING_POP

namespace Lumos
{

    template <typename, typename>
    struct _ECS_export_view;

    template <typename... Component, typename... Exclude>
    struct _ECS_export_view<entt::type_list<Component...>, entt::type_list<Exclude...>>
    {
        static entt::view<entt::exclude_t<Exclude...>, Component...> view(entt::registry& registry)
        {
            return registry.view<Component...>(entt::exclude<Exclude...>);
        }
    };

#define REGISTER_COMPONENT_WITH_ECS(curLuaState, Comp, assignPtr)                                              \
    {                                                                                                          \
        using namespace entt;                                                                                  \
        auto entity_type = curLuaState["Entity"].get_or_create<sol::usertype<registry>>();                     \
        entity_type.set_function("Add" #Comp, assignPtr);                                                      \
        entity_type.set_function("Remove" #Comp, &Entity::RemoveComponent<Comp>);                              \
        entity_type.set_function("Get" #Comp, &Entity::GetComponent<Comp>);                                    \
        entity_type.set_function("GetOrAdd" #Comp, &Entity::GetOrAddComponent<Comp>);                          \
        entity_type.set_function("TryGet" #Comp, &Entity::TryGetComponent<Comp>);                              \
        entity_type.set_function("AddOrReplace" #Comp, &Entity::AddOrReplaceComponent<Comp>);                  \
        entity_type.set_function("Has" #Comp, &Entity::HasComponent<Comp>);                                    \
        auto entityManager_type = curLuaState["enttRegistry"].get_or_create<sol::usertype<registry>>();        \
        entityManager_type.set_function("view_" #Comp, &_ECS_export_view<type_list<Comp>, type_list<>>::view); \
        auto V = curLuaState.new_usertype<basic_view<entity, exclude_t<>, Comp>>(#Comp "_view");               \
        V.set_function("each", &basic_view<entity, exclude_t<>, Comp>::each<std::function<void(Comp&)>>);      \
        V.set_function("front", &basic_view<entity, exclude_t<>, Comp>::front);                                \
    }

    class Scene;

    class LUMOS_EXPORT LuaManager : public ThreadSafeSingleton<LuaManager>
    {
        friend class TSingleton<LuaManager>;

    public:
        LuaManager();
        ~LuaManager();

        void OnInit();
        void OnInit(Scene* scene);
        void OnUpdate(Scene* scene);

        void BindECSLua(sol::state& state);
        void BindLogLua(sol::state& state);
        void BindInputLua(sol::state& state);
        void BindSceneLua(sol::state& state);
        void BindAppLua(sol::state& state);

        sol::state& GetState()
        {
            return m_State;
        }

    private:
        sol::state m_State;
    };
}

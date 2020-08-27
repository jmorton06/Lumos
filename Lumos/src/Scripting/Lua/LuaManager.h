#pragma once

#include "Utilities/TSingleton.h"
#include "Scene/Entity.h"

#include <sol/sol.hpp>
#include <entt/entt.hpp>

namespace Lumos
{

	template<typename, typename>
	struct _ECS_export_view;

	template<typename... Component, typename... Exclude>
	struct _ECS_export_view<entt::type_list<Component...>, entt::type_list<Exclude...>>
	{
		static entt::view<entt::exclude_t<Exclude...>, Component...> view(entt::registry& registry)
		{
			return registry.view<Component...>(entt::exclude<Exclude...>);
		}
	};

#define REGISTER_COMPONENT_WITH_ECS(curLuaState, Comp, assignPtr) \
	{ \
		using namespace entt; \
		auto entity_type = curLuaState["Entity"].get_or_create<sol::usertype<registry>>(); \
		entity_type.set_function("Add" #Comp, assignPtr); \
		entity_type.set_function("Remove" #Comp, &Entity::RemoveComponent<Comp>); \
		entity_type.set_function("Get" #Comp, &Entity::GetComponent<Comp>); \
		entity_type.set_function("GetOrAdd" #Comp, &Entity::GetOrAddComponent<Comp>); \
		entity_type.set_function("TryGet" #Comp, &Entity::TryGetComponent<Comp>); \
        entity_type.set_function("AddOrReplace" #Comp, &Entity::AddOrReplaceComponent<Comp>); \
        entity_type.set_function("Has" #Comp, &Entity::HasComponent<Comp>); \
	}

	class Scene;
	struct WindowProperties;

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

		WindowProperties LoadConfigFile(const std::string& file);

	private:
		sol::state m_State;
	};
}

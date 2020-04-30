#pragma once
#include "lmpch.h"
#include "Utilities/TSingleton.h"

#include <sol/sol.hpp>
#include <entt/entt.hpp>

namespace Lumos
{

template<typename, typename>
struct _ECS_export_view;

template< typename... Component, typename... Exclude >
struct _ECS_export_view< entt::type_list< Component... >, entt::type_list < Exclude... > >
{
    static entt::view< entt::exclude_t < Exclude... >, Component... > view( entt::registry &registry )
    {
        return registry.view< Component... >( entt::exclude< Exclude... > );
    }
};

#define REGISTER_COMPONENT_WITH_ECS( curLuaState, Comp, assignPtr ) \
 { \
     using namespace entt; \
     auto reg_type = curLuaState["Registry"].get_or_create< sol::usertype< registry > >(); \
     reg_type.set_function( "assign_" #Comp, assignPtr ); \
     reg_type.set_function( "remove_" #Comp, &registry::remove< Comp > ); \
     reg_type.set_function( "get_" #Comp, static_cast< Comp&( registry::* )( entity )>( &registry::get< Comp > ) ); \
     reg_type.set_function( "get_or_add_" #Comp, static_cast< Comp&( registry::* )( entity )>( &registry::get_or_emplace< Comp > ) ); \
     reg_type.set_function( "try_get_" #Comp, static_cast< Comp*( registry::*)( entity )>( &registry::try_get< Comp > ) ); \
     reg_type.set_function( "view_" #Comp, &_ECS_export_view< type_list< Comp >, type_list<> >::view ); \
     auto V = curLuaState.new_usertype< basic_view< entity, exclude_t<>, Comp > >( #Comp "_view" ); \
     V.set_function( "each", &basic_view< entity, exclude_t<>, Comp >::each< std::function< void( Comp& ) > > ); \
 }

    class Scene;
	struct WindowProperties;

	class LUMOS_EXPORT LuaManager : public TSingleton<LuaManager>
	{
		friend class TSingleton<LuaManager>;
	public:
		LuaManager();
		~LuaManager();

		void OnInit();
        void OnUpdate(Scene* scene);

        void BindECSLua(sol::state& state);
        void BindLogLua(sol::state& state);
        void BindInputLua(sol::state& state);
		void BindSceneLua(sol::state& state);

		sol::state& GetState() { return m_State; }

		WindowProperties LoadConfigFile(const String& file);

	private:
		sol::state m_State;
	};
}

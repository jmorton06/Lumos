#include "LM.h"
#include "EntityBinding.h"
#include "Entity/Entity.h"

#include "App/SceneManager.h"
#include "App/Application.h"

namespace Lumos
{
const char EntityBinding::className[] = "Entity";

Luna<EntityBinding>::FunctionType EntityBinding::methods[] = {
	lunamethod(EntityBinding, PrintName),
	{ nullptr, nullptr }
};
Luna<EntityBinding>::PropertyType EntityBinding::properties[] = {
	{nullptr, nullptr }
};

EntityBinding::EntityBinding(lua_State* L)
{
	int argc = LuaScripting::SGetArgCount(L);
	String name;
	if (argc > 0)
		name = LuaScripting::SGetString(L, 1);
	else
		name = "Unknown";
	entity = std::make_shared<Entity>(name, Application::Instance()->GetSceneManager()->GetCurrentScene());
}

EntityBinding::~EntityBinding()
{
}

int EntityBinding::PrintName(::lua_State* L)
{
	LUMOS_CORE_INFO("Entity Name - {0}", entity->GetName());
	return 1;
}

void EntityBinding::Bind()
{
	static bool initialized = false;
	if (!initialized)
	{
		initialized = true;
		Luna<EntityBinding>::Register(LuaScripting::GetGlobal()->GetLuaState());
	}
}
}

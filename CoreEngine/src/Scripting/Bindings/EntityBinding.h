#pragma once
#include "JM.h"
#include "Scripting/LuaScript.h"
#include "Scripting/Luna.h"

namespace jm
{
	class Entity;

	class JM_EXPORT EntityBinding
	{
	public:
		static const char className[];
		static Luna<EntityBinding>::FunctionType methods[];
		static Luna<EntityBinding>::PropertyType properties[];

		EntityBinding(lua_State* L);
		~EntityBinding();

		int PrintName(lua_State* L);

		std::shared_ptr<Entity> entity;

		static void Bind();
	};
}

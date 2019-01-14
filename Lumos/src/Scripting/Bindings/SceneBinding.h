#pragma once
#include "JM.h"
#include "Scripting/LuaScript.h"
#include "Scripting/Luna.h"

namespace jm
{
    class Scene;

    class JM_EXPORT SceneBinding
    {
    public:
        static const char className[];
        static Luna<SceneBinding>::FunctionType methods[];
        static Luna<SceneBinding>::PropertyType properties[];

        SceneBinding(lua_State* L);
        SceneBinding(Scene* scene);
        ~SceneBinding();

        int SetCamera(lua_State* L);
		int AddEntity(lua_State* L);

        Scene* scene;

        static void Bind();
    };
}

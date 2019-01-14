#include "JM.h"
#include "SceneBinding.h"
#include "Renderer/Scene.h"
#include "CameraBindings/CameraBinding.h"
#include "CameraBindings/CameraFPSBinding.h"
#include "CameraBindings/CameraThirdPersonBinding.h"
#include "CameraBindings/CameraMayaBinding.h"
#include "CameraBindings/Camera2DBinding.h"
#include "EntityBinding.h"

namespace jm
{
    const char SceneBinding::className[] = "Scene";

    Luna<SceneBinding>::FunctionType SceneBinding::methods[] = {
            lunamethod(SceneBinding, SetCamera),
			lunamethod(SceneBinding, AddEntity),
            { nullptr, nullptr }
    };
    Luna<SceneBinding>::PropertyType SceneBinding::properties[] = {
            {nullptr, nullptr }
    };

    SceneBinding::SceneBinding(Scene* scene) : scene(scene)
    {
    }

    SceneBinding::SceneBinding(lua_State* L)
    {
        //int argc = LuaScripting::SGetArgCount(L);

        scene = nullptr;

    }

    SceneBinding::~SceneBinding()
    {
    }

    int SceneBinding::SetCamera(lua_State *L)
    {
        int argc = LuaScripting::SGetArgCount(L);
        if (argc > 0)
        {
            CameraBinding* v1 = Luna<CameraBinding>::lightcheck(L, 1);
            if (v1)
            {
                scene->SetCamera(v1->camera);
                return 1;
            }
            else
            {
                v1 = Luna<CameraFPSBinding>::lightcheck(L, 1);
                if (v1)
                {
                    scene->SetCamera(v1->camera);
                    return 1;
                }
                else
                {
                    v1 = Luna<CameraMayaBinding>::lightcheck(L, 1);
                    if (v1)
                    {
                        scene->SetCamera(v1->camera);
                        return 1;
                    }
                    else
                    {
                        v1 = Luna<CameraThirdPersonBinding>::lightcheck(L, 1);
                        if (v1)
                        {
                            scene->SetCamera(v1->camera);
                            return 1;
                        }
                        else
                        {
                            v1 = Luna<Camera2DBinding>::lightcheck(L, 1);
                            if (v1)
                            {
                                scene->SetCamera(v1->camera);
                                return 1;
                            }
                        }
                    }
                }
            }
            LuaScripting::SError(L, "SetProjectionMatrix not valid arguments!");

        }
        LuaScripting::SError(L, "SetProjectionMatrix not enough arguments!");
        return 0;
    }

	int SceneBinding::AddEntity(lua_State* L)
	{
		int argc = LuaScripting::SGetArgCount(L);
		if (argc > 0)
		{
			EntityBinding* v1 = Luna<EntityBinding>::lightcheck(L, 1);
			if (v1)
			{
				scene->AddEntity(v1->entity);
			}
			else
			{
				LuaScripting::SError(L, "AddEntity not valid arguments!");
			}
			return 1;
		}
		LuaScripting::SError(L, "AddEntity not valid arguments!");
		return 0;
	}

	void SceneBinding::Bind()
    {
        static bool initialized = false;
        if (!initialized)
        {
            initialized = true;
            Luna<SceneBinding>::Register(LuaScripting::GetGlobal()->GetLuaState());
            //LuaScripting::GetGlobal()->RunText("scene = Scene()");
        }
    }
}

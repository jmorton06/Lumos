#pragma once

#include "Utilities/TSingleton.h"
#include "Core/DataStructures/TDArray.h"

namespace sol
{
    class state;
}

namespace Lumos
{
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

        void CollectGarbage();

        void OnNewProject(const std::string& projectPath);

        void BindECSLua(sol::state& state);
        void BindLogLua(sol::state& state);
        void BindInputLua(sol::state& state);
        void BindSceneLua(sol::state& state);
        void BindAppLua(sol::state& state);
        void BindUILua(sol::state& lua);

        static TDArray<std::string>& GetIdentifiers() { return s_Identifiers; }

        sol::state& GetState()
        {
            return *m_State;
        }

    private:
        static TDArray<std::string> s_Identifiers;

        sol::state* m_State;
    };
}

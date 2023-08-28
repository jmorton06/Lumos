#pragma once

#include "Utilities/TSingleton.h"

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

        void OnNewProject(const std::string& projectPath);

        void BindECSLua(sol::state& state);
        void BindLogLua(sol::state& state);
        void BindInputLua(sol::state& state);
        void BindSceneLua(sol::state& state);
        void BindAppLua(sol::state& state);

        static std::vector<std::string>& GetIdentifiers() { return s_Identifiers; }

        sol::state& GetState()
        {
            return *m_State;
        }

        static std::vector<std::string> s_Identifiers;

    private:
        sol::state* m_State;
    };
}

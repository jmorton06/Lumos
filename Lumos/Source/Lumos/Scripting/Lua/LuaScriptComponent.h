#pragma once

#include "Core/Application.h"
#include "Core/UUID.h"
#include "Core/DataStructures/Map.h"
#include <sol/forward.hpp>
#include <unordered_map>

namespace Lumos
{
    class Scene;
    class Entity;

    class LUMOS_EXPORT LuaScriptComponent
    {
        template <typename Archive>
        friend void save(Archive& archive, const LuaScriptComponent& luaComponent);
        template <typename Archive>
        friend void load(Archive& archive, LuaScriptComponent& luaComponent);

    public:
        LuaScriptComponent();
        LuaScriptComponent(const std::string& fileName, Scene* scene);
        ~LuaScriptComponent();

        void Init();
        void OnInit();
        void OnUpdate(float dt);
        void Reload();
        void Load(const std::string& fileName);
        Entity GetCurrentEntity();

        // For accessing this component in lua
        void SetThisComponent();

        void LoadScript(const std::string& fileName);

        void OnCollision2DBegin();
        void OnCollision2DEnd();

        void OnCollision3DBegin();
        void OnCollision3DEnd();

        const sol::environment& GetSolEnvironment() const
        {
            return *m_Env;
        }
        const std::string& GetFilePath() const
        {
            return m_FileName;
        }

        void SetFilePath(const std::string& path)
        {
            m_FileName = path;
        }

        const std::unordered_map<int, std::string>& GetErrors() const
        {
            return m_Errors;
        }

        bool Loaded()
        {
            return m_Env.get() != nullptr;
        }

    private:
        Scene* m_Scene = nullptr;
        std::string m_FileName;
        std::unordered_map<int, std::string> m_Errors;

        SharedPtr<sol::environment> m_Env;
        SharedPtr<sol::protected_function> m_OnInitFunc;
        SharedPtr<sol::protected_function> m_UpdateFunc;
        SharedPtr<sol::protected_function> m_OnReleaseFunc;

        SharedPtr<sol::protected_function> m_Phys2DBeginFunc;
        SharedPtr<sol::protected_function> m_Phys3DBeginFunc;
        SharedPtr<sol::protected_function> m_Phys2DEndFunc;
        SharedPtr<sol::protected_function> m_Phys3DEndFunc;
    };
}

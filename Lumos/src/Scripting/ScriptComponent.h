#pragma once
#include "lmpch.h"
#include <sol/sol.hpp>

namespace Lumos
{    
	class Scene;

    class LUMOS_EXPORT ScriptComponent
    {
    public:
        ScriptComponent() = default;
        ScriptComponent(const String& fileName, Scene* scene);
        ~ScriptComponent();
        
		void Init();
		void Update(float dt);
        void Reload();
        void Load(const String& fileName);

		void LoadScript(const std::string& fileName);
        
        const sol::environment& GetSolEnvironment() const { return m_Env; }
        const String& GetFilePath() const { return m_FileName; }
        
        void SetFilePath(const String& path) { m_FileName = path; }
    
        const std::vector<std::string>& GetErrors() const { return m_Errors; }
        
	private:

		Scene* m_Scene = nullptr;
		std::string m_FileName;
        
        std::vector<std::string> m_Errors;
        
        sol::environment m_Env;
        sol::protected_function m_UpdateFunc;
    };
}

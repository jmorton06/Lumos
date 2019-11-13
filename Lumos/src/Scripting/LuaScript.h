#pragma once
#include "lmpch.h"
#include <sol/sol.hpp>

namespace Lumos
{    
	class Scene;

    class LUMOS_EXPORT LuaScript
    {
    public:
        LuaScript(const String& fileName);
        ~LuaScript();
        
		void Init();
        void Init(Scene* scene);
		void Update();

		void LoadScript(const std::string& fileName);

	private:

		Scene* m_Scene = nullptr;
		std::string m_FileName;
		std::vector<sol::table> m_LuaTables;
    };
}

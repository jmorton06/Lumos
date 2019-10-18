#pragma once
#include "lmpch.h"
#include "Utilities/TSingleton.h"

namespace sol
{
	class state;
}

namespace Lumos
{
	struct WindowProperties;

	class LUMOS_EXPORT LuaManager : public TSingleton<LuaManager>
	{
		friend class TSingleton<LuaManager>;
	public:
		LuaManager();
		~LuaManager();

		void OnInit();

		sol::state* GetState() const { return m_State; }

		WindowProperties LoadConfigFile(const String& file);

	private:
		sol::state* m_State;
	};
}

#pragma once
#include "LM.h"
#include "Utilities/TSingleton.h"
#include <sol/sol.hpp>

namespace lumos
{
    class LUMOS_EXPORT LuaScript : public TSingleton<LuaScript>
    {
        friend class TSingleton<LuaScript>;
    public:
        LuaScript();
        ~LuaScript();
        
        void OnInit();
        
        sol::state* GetState() const { return m_State; }
        
    private:
        sol::state* m_State;
    };
}

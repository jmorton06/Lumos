#pragma once
#include "LM.h"

namespace sol
{
    class state;
}

namespace lumos
{
    class LUMOS_EXPORT LuaScript
    {
    public:
        LuaScript();
        ~LuaScript();
        
    private:
        sol::state* m_State;
    };
}

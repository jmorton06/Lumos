#pragma once
#include "LM.h"
#include "Utilities/TSingleton.h"

namespace sol
{
    class state;
}

namespace lumos
{
    struct WindowProperties;
    
    class LUMOS_EXPORT LuaScript : public TSingleton<LuaScript>
    {
        friend class TSingleton<LuaScript>;
    public:
        LuaScript();
        ~LuaScript();
        
        void OnInit();
        
        sol::state* GetState() const { return m_State; }
        
        WindowProperties LoadConfigFile(const String& file);
        
    private:
        sol::state* m_State;
    };
}

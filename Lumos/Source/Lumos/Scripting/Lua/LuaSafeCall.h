#pragma once
#include "Sol2Config.h"
#include <sol/sol.hpp>
#include "Core/LMLog.h"

namespace Lumos
{
    // Safe wrapper for Lua function calls
    // With SOL_ALL_SAFETIES_ON 0, we need manual error handling
    template<typename Func, typename... Args>
    inline bool SafeLuaCall(const char* functionName, Func&& func, Args&&... args)
    {
        try
        {
            // Check if function is valid before calling
            if(!func.valid())
            {
                LERROR("Lua function '{0}' is not valid", functionName);
                return false;
            }

            // Call the function
            sol::protected_function_result result = func(std::forward<Args>(args)...);

            // Check if call succeeded
            if(!result.valid())
            {
                sol::error err = result;
                LERROR("Lua error in '{0}': {1}", functionName, err.what());
                return false;
            }

            return true;
        }
        catch(const sol::error& e)
        {
            LERROR("Lua exception in '{0}': {1}", functionName, e.what());
            return false;
        }
        catch(const std::exception& e)
        {
            LERROR("Exception in Lua call '{0}': {1}", functionName, e.what());
            return false;
        }
        catch(...)
        {
            LERROR("Unknown exception in Lua call '{0}'", functionName);
            return false;
        }
    }

    // Validate Lua table has required field
    inline bool ValidateLuaTable(sol::table& table, const char* fieldName, const char* tableName)
    {
        if(!table.valid())
        {
            LERROR("Lua table '{0}' is not valid", tableName);
            return false;
        }

        sol::object field = table[fieldName];
        if(!field.valid())
        {
            // Field doesn't exist - this is okay, just return false
            return false;
        }

        return true;
    }

    // Validate Lua function exists and is callable
    inline bool ValidateLuaFunction(sol::object& obj, const char* functionName)
    {
        if(!obj.valid())
        {
            return false;
        }

        if(obj.get_type() != sol::type::function)
        {
            LWARN("Lua object '{0}' is not a function (type: {1})", functionName, (int)obj.get_type());
            return false;
        }

        return true;
    }
}

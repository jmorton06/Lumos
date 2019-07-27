#pragma once
#include "LM.h"

    //Wrapper around typeid(T).name(), to enforce a consistent return style across all (supported) compilers.

#if defined(LUMOS_PLATFORM_WINDOWS)

namespace Lumos
{
    template<typename T>
    String GetTypename()
    {
        std::vector<std::string> vec = SplitString(typeid(T).name(), ' ');
        if (vec.size() < 2)
        {
            LUMOS_CORE_WARN("Failed to GetTypename. Returning empty string!");
            return "";
        }
        
        String result = "";
        for (size_t i = 1; i < vec.size(); i++)
        {
            result += vec[i];
        }
        return result;
    }
}
    
#define LUMOS_TYPENAME(T) GetTypename<T>()
    
#else
#include <cxxabi.h>

namespace Lumos
{
    template<typename T>
    std::string GetTypename()
    {
        int status;
        char* realName = abi::__cxa_demangle(typeid(T).name(), 0, 0, &status);
        String result = { realName };
        free(realName);
        return result;
    }
}
#define LUMOS_TYPENAME(T) GetTypename<T>()
#endif()

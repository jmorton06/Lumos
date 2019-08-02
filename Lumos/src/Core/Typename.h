#pragma once
#include "LM.h"

    //Wrapper around typeid(T).name(), to enforce a consistent return style across all (supported) compilers.

#if defined(LUMOS_PLATFORM_WINDOWS)

namespace Lumos
{
    template<typename T>
    String GetTypename()
    {
		String delimiters = String(1, ' ');
		size_t start = 0;
		String string = typeid(T).name();
		size_t end = string.find_first_of(delimiters);

		std::vector<String> result;

		while (end <= String::npos)
		{
			String token = string.substr(start, end - start);
			if (!token.empty())
				result.push_back(token);

			if (end == String::npos)
				break;

			start = end + 1;
			end = string.find_first_of(delimiters, start);
		}

        if (result.size() < 2)
        {
            LUMOS_CORE_WARN("Failed to GetTypename. Returning empty string!");
            return "";
        }
        
        String name = "";
        for (size_t i = 1; i < result.size(); i++)
        {
			name += result[i];
        }

        return name;
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
#endif

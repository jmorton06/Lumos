#pragma once
#include "lmpch.h"

#if defined(LUMOS_PLATFORM_WINDOWS)

namespace Lumos
{
	template<typename T>
	std::string GetTypename()
	{
		std::string delimiters = std::string(1, ' ');
		size_t start = 0;
		std::string string = typeid(T).name();
		size_t end = string.find_first_of(delimiters);

		std::vector<std::string> result;

		while(end <= std::string::npos)
		{
			std::string token = string.substr(start, end - start);
			if(!token.empty())
				result.push_back(token);

			if(end == std::string::npos)
				break;

			start = end + 1;
			end = string.find_first_of(delimiters, start);
		}

		if(result.size() < 2)
		{
			LUMOS_LOG_WARN("Failed to GetTypename. Returning empty string!");
			return "";
		}

		std::string name = "";
		for(size_t i = 1; i < result.size(); i++)
		{
			name += result[i];
		}

		return name;
	}
}

#	define LUMOS_TYPENAME_STRING(T) GetTypename<T>()

#else
#	include <cxxabi.h>

namespace Lumos
{
	template<typename T>
	std::string GetTypename()
	{
		int status;
		char* realName = abi::__cxa_demangle(typeid(T).name(), 0, 0, &status);
		std::string result = {realName};
		free(realName);
		return result;
	}
}
#	define LUMOS_TYPENAME_STRING(T) GetTypename<T>()
#endif

#define LUMOS_TYPENAME(T) typeid(T).hash_code()

#pragma once

#include "Core/Core.h"

namespace Lumos
{ 
	namespace Internal 
	{
	// Low-level System operations
	class LUMOS_EXPORT CoreSystem
	{
	public:
		static void Init(bool enableProfiler = true);
		static void Shutdown();
	};

} 

};

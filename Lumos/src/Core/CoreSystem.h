#pragma once

#include "lmpch.h"

namespace Lumos
{ 
	namespace Internal 
	{
	// Low-level System operations
	class LUMOS_EXPORT CoreSystem
	{
	public:
		static void Init();
		static void Shutdown();
	};

} 

};

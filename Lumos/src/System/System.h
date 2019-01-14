#pragma once

#include "JM.h"

namespace jm
{ 
	namespace internal 
	{
	// Low-level system operations
	class JM_EXPORT System
	{
	public:
		static void Init();
		static void Shutdown();
	};

} 

};

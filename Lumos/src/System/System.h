#pragma once

#include "LM.h"

namespace Lumos
{ 
	namespace internal 
	{
	// Low-level system operations
	class LUMOS_EXPORT System
	{
	public:
		static void Init();
		static void Shutdown();
	};

} 

};

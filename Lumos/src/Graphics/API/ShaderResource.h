#pragma once

#include "lmpch.h"

namespace Lumos
{
	namespace Graphics
	{
		class ShaderResourceDeclaration
		{
		public:
			virtual ~ShaderResourceDeclaration() = default;
			virtual const std::string& GetName() const = 0;
			virtual u32 GetRegister() const = 0;
			virtual u32 GetCount() const = 0;
		};

		typedef std::vector<ShaderResourceDeclaration*> ShaderResourceList;
	}
}
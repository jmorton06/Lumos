#pragma once
#include "LM.h"
#include "Texture.h"

namespace Lumos
{
	namespace Graphics
	{
		class LUMOS_EXPORT TextureDepth : public Texture
		{
		public:
			static TextureDepth* Create(u32 width, u32 height);

			virtual void Resize(u32 width, u32 height) = 0;
		};
	}
}
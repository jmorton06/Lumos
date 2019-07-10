#pragma once
#include "LM.h"
#include "Texture.h"

namespace Lumos
{
	namespace Graphics
	{
		class LUMOS_EXPORT TextureDepthArray : public Texture
		{
		public:
			static TextureDepthArray* Create(u32 width, u32 height, u32 count);

			virtual void Init() = 0;
			virtual void Resize(u32 width, u32 height, u32 count) = 0;
		};
	}
}
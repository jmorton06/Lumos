#pragma once
#include "LM.h"
#include "Texture.h"

namespace Lumos
{

	class LUMOS_EXPORT TextureDepthArray : public Texture
	{
	public:
		static TextureDepthArray* Create(uint width, uint height, uint count);

		virtual void Init() = 0;
		virtual void Resize(uint width, uint height, uint count) = 0;
	};
}
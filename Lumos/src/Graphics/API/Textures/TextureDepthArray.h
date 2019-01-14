#pragma once
#include "JM.h"
#include "Texture.h"

namespace jm
{

	class JM_EXPORT TextureDepthArray : public Texture
	{
	public:
		static TextureDepthArray* Create(uint width, uint height, uint count);

		virtual void Init() = 0;
		virtual void Resize(uint width, uint height, uint count) = 0;
	};
}
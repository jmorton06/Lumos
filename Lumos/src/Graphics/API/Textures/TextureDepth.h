#pragma once
#include "LM.h"
#include "Texture.h"

namespace Lumos
{

	class LUMOS_EXPORT TextureDepth : public Texture
	{
	public:
		static TextureDepth* Create(uint width, uint height);

		virtual uint GetHandle() const override = 0;

		virtual void Resize(uint width, uint height) = 0;
	};
}
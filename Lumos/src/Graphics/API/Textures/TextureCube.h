#pragma once
#include "LM.h"
#include "Texture.h"

namespace lumos
{
	namespace graphics
	{
		class LUMOS_EXPORT TextureCube : public Texture
		{
		protected:
			enum class InputFormat
			{
				VERTICAL_CROSS,
				HORIZONTAL_CROSS
			};
		public:
			static TextureCube* Create(uint size);
			static TextureCube* CreateFromFile(const String& filepath);
			static TextureCube* CreateFromFiles(const String* files);
			static TextureCube* CreateFromVCross(const String* files, int32 mips);
		};
	}
}
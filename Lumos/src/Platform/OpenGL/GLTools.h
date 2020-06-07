#pragma once
#include "lmpch.h"

namespace Lumos
{
	namespace Graphics
	{
		enum class CullMode;
		enum class DescriptorType;
		enum class ShaderType : int;
		enum class TextureFormat;
		enum class Format;
		enum class TextureWrap;
		enum class StencilType;
		enum class DataType;
		enum class DrawType;
		enum class RendererBlendFunction;

		namespace GLTools
		{
			u32 TextureFormatToGL(TextureFormat format);
			u32 TextureWrapToGL(TextureWrap wrap);
			u32 TextureFormatToInternalFormat(u32 format);
			u32 StencilTypeToGL(const StencilType type);

			u32 RendererBufferToGL(u32 buffer);
			u32 RendererBlendFunctionToGL(RendererBlendFunction function);
			u32 DataTypeToGL(DataType dataType);
			u32 DrawTypeToGL(DrawType drawType);
		}
	}
}

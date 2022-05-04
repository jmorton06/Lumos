#pragma once
#include "Graphics/RHI/Definitions.h"

namespace Lumos
{
    namespace Graphics
    {
        namespace GLTools
        {
            uint32_t FormatToGL(Format format, bool srgb = true);
            uint32_t TextureWrapToGL(TextureWrap wrap);
            uint32_t FormatToInternalFormat(uint32_t format);
            uint32_t StencilTypeToGL(const StencilType type);

            uint32_t RendererBufferToGL(uint32_t buffer);
            uint32_t RendererBlendFunctionToGL(RendererBlendFunction function);
            uint32_t DataTypeToGL(DataType dataType);
            uint32_t DrawTypeToGL(DrawType drawType);
        }
    }
}

#pragma once

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
            uint32_t TextureFormatToGL(TextureFormat format, bool srgb = true);
            uint32_t TextureWrapToGL(TextureWrap wrap);
            uint32_t TextureFormatToInternalFormat(uint32_t format);
            uint32_t StencilTypeToGL(const StencilType type);

            uint32_t RendererBufferToGL(uint32_t buffer);
            uint32_t RendererBlendFunctionToGL(RendererBlendFunction function);
            uint32_t DataTypeToGL(DataType dataType);
            uint32_t DrawTypeToGL(DrawType drawType);
        }
    }
}

#include "Precompiled.h"
#include "GLTools.h"
#include "GL.h"
#include "GLRenderer.h"
#include "GLTexture.h"

namespace Lumos
{
    namespace Graphics
    {
        uint32_t GLTools::TextureFormatToGL(const TextureFormat format, bool srgb)
        {
            switch(format)
            {
            case TextureFormat::RGBA:
                return GL_RGBA; //srgb ? GL_SRGB8 :
            case TextureFormat::RGB:
                return GL_RGB;
            case TextureFormat::R8:
                return GL_R8;
            case TextureFormat::RG8:
                return GL_RG8;
            case TextureFormat::RGB8:
                return srgb ? GL_SRGB8 : GL_RGB8;
            case TextureFormat::RGBA8:
                return srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8;
            case TextureFormat::RGB16:
                return GL_RGB16F;
            case TextureFormat::RGBA16:
                return GL_RGBA16F;
            case TextureFormat::RGB32:
                return GL_RGB32F;
            case TextureFormat::RGBA32:
                return GL_RGBA32F;
            case TextureFormat::DEPTH:
                return GL_DEPTH24_STENCIL8;
            default:
                LUMOS_ASSERT(false, "[Texture] Unsupported TextureFormat");
                return 0;
            }
        }

        uint32_t GLTools::TextureWrapToGL(const TextureWrap wrap)
        {
            switch(wrap)
            {
#ifndef LUMOS_PLATFORM_MOBILE
            case TextureWrap::CLAMP:
                return GL_CLAMP;
            case TextureWrap::CLAMP_TO_BORDER:
                return GL_CLAMP_TO_BORDER;
#endif
            case TextureWrap::CLAMP_TO_EDGE:
                return GL_CLAMP_TO_EDGE;
            case TextureWrap::REPEAT:
                return GL_REPEAT;
            case TextureWrap::MIRRORED_REPEAT:
                return GL_MIRRORED_REPEAT;
            default:
                LUMOS_ASSERT(false, "[Texture] Unsupported TextureWrap");
                return 0;
            }
        }

        uint32_t GLTools::TextureFormatToInternalFormat(uint32_t format)
        {
            switch(format)
            {
            case GL_SRGB8:
                return GL_RGB;
            case GL_SRGB8_ALPHA8:
                return GL_RGBA;
            case GL_RGBA:
                return GL_RGBA;
            case GL_RGB:
                return GL_RGB;
            case GL_R8:
                return GL_RED;
            case GL_RG8:
                return GL_RG;
            case GL_RGB8:
                return GL_RGB;
            case GL_RGBA8:
                return GL_RGBA;
            case GL_RGB16:
                return GL_RGB;
            case GL_RGBA16:
                return GL_RGBA;
            case GL_RGBA16F:
                return GL_RGBA;
            case GL_RGB32F:
                return GL_RGB;
            case GL_RGBA32F:
                return GL_RGBA;
            case GL_SRGB:
                return GL_RGB;
            case GL_SRGB_ALPHA:
                return GL_RGBA;
            case GL_LUMINANCE:
                return GL_LUMINANCE;
            case GL_LUMINANCE_ALPHA:
                return GL_LUMINANCE_ALPHA;

            default:
                LUMOS_ASSERT(false, "[Texture] Unsupported Texture Format");
                return 0;
            }
        }

        uint32_t GLTools::StencilTypeToGL(const StencilType type)
        {
            switch(type)
            {
            case StencilType::EQUAL:
                return GL_EQUAL;
            case StencilType::NOTEQUAL:
                return GL_NOTEQUAL;
            case StencilType::KEEP:
                return GL_KEEP;
            case StencilType::REPLACE:
                return GL_REPLACE;
            case StencilType::ZERO:
                return GL_ZERO;
            case StencilType::ALWAYS:
                return GL_ALWAYS;
            default:
                LUMOS_ASSERT(false, "Unsupported StencilType");
                return 0;
            }
        }

        uint32_t GLTools::RendererBufferToGL(uint32_t buffer)
        {
            uint32_t result = 0;
            if(buffer & RENDERER_BUFFER_COLOUR)
                result |= GL_COLOR_BUFFER_BIT;
            if(buffer & RENDERER_BUFFER_DEPTH)
                result |= GL_DEPTH_BUFFER_BIT;
            if(buffer & RENDERER_BUFFER_STENCIL)
                result |= GL_STENCIL_BUFFER_BIT;
            return result;
        }

        uint32_t GLTools::RendererBlendFunctionToGL(RendererBlendFunction function)
        {
            switch(function)
            {
            case RendererBlendFunction::ZERO:
                return GL_ZERO;
            case RendererBlendFunction::ONE:
                return GL_ONE;
            case RendererBlendFunction::SOURCE_ALPHA:
                return GL_SRC_ALPHA;
            case RendererBlendFunction::DESTINATION_ALPHA:
                return GL_DST_ALPHA;
            case RendererBlendFunction::ONE_MINUS_SOURCE_ALPHA:
                return GL_ONE_MINUS_SRC_ALPHA;
            default:
                return 0;
            }
        }

        uint32_t GLTools::DataTypeToGL(DataType dataType)
        {
            switch(dataType)
            {
            case DataType::FLOAT:
                return GL_FLOAT;
            case DataType::UNSIGNED_INT:
                return GL_UNSIGNED_INT;
            case DataType::UNSIGNED_BYTE:
                return GL_UNSIGNED_BYTE;
            default:
                LUMOS_LOG_ERROR("Unsupported DataType");
                break;
            }
            return 0;
        }

        uint32_t GLTools::DrawTypeToGL(DrawType drawType)
        {
            switch(drawType)
            {
            case DrawType::POINT:
                return GL_POINTS;
            case DrawType::LINES:
                return GL_LINES;
            case DrawType::TRIANGLE:
                return GL_TRIANGLES;
            default:
                LUMOS_LOG_ERROR("Unsupported DrawType");
                break;
            }
            return 0;
        }
    }
}

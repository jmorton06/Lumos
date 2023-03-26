#include "Precompiled.h"
#include "GLUtilities.h"
#include "GL.h"
#include "GLRenderer.h"
#include "GLTexture.h"

namespace Lumos
{
    namespace Graphics
    {
        uint32_t GLUtilities::FormatToGL(const RHIFormat format, bool srgb)
        {
            switch(format)
            {
            case RHIFormat::R8_Unorm:
                return GL_R8;
            case RHIFormat::R8G8_Unorm:
                return GL_RG8;
            case RHIFormat::R8G8B8_Unorm:
                return srgb ? GL_SRGB8 : GL_RGB8;
            case RHIFormat::R8G8B8A8_Unorm:
                return srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8;
            case RHIFormat::R11G11B10_Float:
                return GL_R11F_G11F_B10F;
            case RHIFormat::R10G10B10A2_Unorm:
                return GL_RGB10_A2;
            case RHIFormat::R16G16B16_Float:
                return GL_RGB16F;
            case RHIFormat::R16G16B16A16_Float:
                return GL_RGBA16F;
            case RHIFormat::R32G32B32_Float:
                return GL_RGB32F;
            case RHIFormat::R32G32B32A32_Float:
                return GL_RGBA32F;
            case RHIFormat::D24_Unorm_S8_UInt:
                return GL_DEPTH24_STENCIL8;
            case RHIFormat::D32_Float:
                return GL_DEPTH_COMPONENT32F;
            case RHIFormat::D32_Float_S8_UInt:
                return GL_DEPTH32F_STENCIL8;
            default:
                LUMOS_ASSERT(false, "[Texture] Unsupported Format");
                return 0;
            }
        }

        uint32_t GLUtilities::TextureWrapToGL(const TextureWrap wrap)
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

        uint32_t GLUtilities::FormatToInternalFormat(uint32_t format)
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
            case GL_R11F_G11F_B10F:
                return GL_RGB;
            case GL_RGB10_A2:
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

        uint32_t GLUtilities::StencilTypeToGL(const StencilType type)
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

        uint32_t GLUtilities::GetGLTypefromFormat(RHIFormat format)
        {
            switch (format)
            {
            case RHIFormat::R8_Unorm:
            case RHIFormat::R8G8_Unorm:
            case RHIFormat::R8G8B8_Unorm:
            case RHIFormat::R8G8B8A8_Unorm:
            case RHIFormat::R10G10B10A2_Unorm:
                return GL_UNSIGNED_BYTE;
            case RHIFormat::R11G11B10_Float:
            case RHIFormat::R16G16B16_Float:
            case RHIFormat::R16G16B16A16_Float:
            case RHIFormat::R32G32B32_Float:
            case RHIFormat::R32G32B32A32_Float:
                return GL_FLOAT;
            default:
                LUMOS_ASSERT(false, "[Texture] Unsupported Format");
                return GL_UNSIGNED_BYTE;
            }
        }

        uint32_t GLUtilities::RendererBufferToGL(uint32_t buffer)
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

        uint32_t GLUtilities::RendererBlendFunctionToGL(RendererBlendFunction function)
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

        uint32_t GLUtilities::DataTypeToGL(DataType dataType)
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

        uint32_t GLUtilities::DrawTypeToGL(DrawType drawType)
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

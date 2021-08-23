#pragma once

namespace Lumos
{
    namespace Graphics
    {
        class Shader;
        class RenderPass;
        class CommandBuffer;
        class DescriptorSet;
        struct VertexInputDescription;
        struct DescriptorLayoutInfo;
        struct DescriptorPoolInfo;

        class Texture;
        class Texture2D;
        class TextureCube;
        class TextureDepth;
        class TextureDepthArray;
        class Framebuffer;
        enum class TextureType : int;
        enum class TextureFormat;
        enum class TextureType;
        class RenderPass;

        static const uint8_t MAX_RENDER_TARGETS = 8;
        static const uint8_t SHADOWMAP_MAX = 16;
        static const uint8_t MAX_MIPS = 32;

        enum class CullMode
        {
            FRONT,
            BACK,
            FRONTANDBACK,
            NONE
        };

        enum class PolygonMode
        {
            FILL,
            LINE,
            POINT
        };

        enum class BlendMode
        {
            None = 0,
            OneZero,
            ZeroSrcColor,
            SrcAlphaOneMinusSrcAlpha,
        };

        enum class TextureWrap
        {
            NONE,
            REPEAT,
            CLAMP,
            MIRRORED_REPEAT,
            CLAMP_TO_EDGE,
            CLAMP_TO_BORDER
        };

        enum class TextureFilter
        {
            NONE,
            LINEAR,
            NEAREST
        };

        enum class TextureFormat
        {
            NONE,
            R8,
            RG8,
            RGB8,
            RGBA8,
            RGB16,
            RGBA16,
            RGB32,
            RGBA32,
            RGB,
            RGBA,
            DEPTH,
            STENCIL,
            DEPTH_STENCIL,
            SCREEN
        };

        enum class TextureType
        {
            COLOUR,
            DEPTH,
            DEPTHARRAY,
            CUBE,
            OTHER
        };

        struct RenderPassDesc
        {
            Texture** attachments;
            TextureType* attachmentTypes;
            uint32_t attachmentCount;
            bool clear = true;
        };

        enum SubPassContents
        {
            INLINE,
            SECONDARY
        };

        struct TextureParameters
        {
            TextureFormat format;
            TextureFilter minFilter;
            TextureFilter magFilter;
            TextureWrap wrap;
            bool srgb = false;
            uint16_t msaaLevel;
            uint16_t flags;

            TextureParameters()
            {
                format = TextureFormat::RGBA8;
                minFilter = TextureFilter::NEAREST;
                magFilter = TextureFilter::NEAREST;
                wrap = TextureWrap::REPEAT;
                msaaLevel = 1;
            }

            TextureParameters(TextureFormat format, TextureFilter minFilter, TextureFilter magFilter, TextureWrap wrap)
                : format(format)
                , minFilter(minFilter)
                , magFilter(magFilter)
                , wrap(wrap)
            {
            }

            TextureParameters(TextureFilter minFilter, TextureFilter magFilter)
                : format(TextureFormat::RGBA8)
                , minFilter(minFilter)
                , magFilter(magFilter)
                , wrap(TextureWrap::CLAMP)
            {
            }

            TextureParameters(TextureFilter minFilter, TextureFilter magFilter, TextureWrap wrap)
                : format(TextureFormat::RGBA8)
                , minFilter(minFilter)
                , magFilter(magFilter)
                , wrap(wrap)
            {
            }

            TextureParameters(TextureWrap wrap)
                : format(TextureFormat::RGBA8)
                , minFilter(TextureFilter::LINEAR)
                , magFilter(TextureFilter::LINEAR)
                , wrap(wrap)
            {
            }

            TextureParameters(TextureFormat format)
                : format(format)
                , minFilter(TextureFilter::LINEAR)
                , magFilter(TextureFilter::LINEAR)
                , wrap(TextureWrap::CLAMP)
            {
            }
        };

        struct TextureLoadOptions
        {
            bool flipX;
            bool flipY;
            bool generateMipMaps;

            TextureLoadOptions()
            {
                flipX = false;
                flipY = false;
                generateMipMaps = true;
            }

            TextureLoadOptions(bool flipX, bool flipY, bool genMips = true)
                : flipX(flipX)
                , flipY(flipY)
                , generateMipMaps(genMips)
            {
            }
        };

        enum TextureFlags : uint16_t
        {
            Texture_Sampled = BIT(0),
            Texture_Storage = BIT(1),
            Texture_RenderTarget = BIT(2),
            Texture_DepthStencil = BIT(3),
            Texture_DepthStencilReadOnly = BIT(4)
        };

        enum class ImageLayout
        {
            Undefined,
            General,
            Color_Attachment_Optimal,
            Depth_Stencil_Attachment_Optimal,
            Depth_Stencil_Read_Only_Optimal,
            Shader_Read_Only_Optimal,
            Transfer_Dst_Optimal,
            Present_Src
        };

    }

}

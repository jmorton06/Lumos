#pragma once
#include "Core/LMLog.h"
#include "Core/DataStructures/TDArray.h"
#include <cstring>

namespace Lumos
{
    namespace Graphics
    {
        class Shader;
        class RenderPass;
        class CommandBuffer;
        class DescriptorSet;
        class Pipeline;
        class Shader;
        class UniformBuffer;
        class Framebuffer;
        class RenderPass;
        class GraphicsContext;
        class Texture;
        class Texture2D;
        class TextureCube;
        class TextureDepth;
        class TextureDepthArray;

        static constexpr uint8_t MAX_RENDER_TARGETS = 8;
        static constexpr uint8_t SHADOWMAP_MAX      = 16;
        static constexpr uint8_t MAX_MIPS           = 32;

        // Descriptor set limits
        static constexpr uint16_t DESCRIPTOR_MAX_STORAGE_TEXTURES         = 1024;
        static constexpr uint16_t DESCRIPTOR_MAX_STORAGE_BUFFERS          = 1024;
        static constexpr uint16_t DESCRIPTOR_MAX_CONSTANT_BUFFERS         = 1024;
        static constexpr uint16_t DESCRIPTOR_MAX_CONSTANT_BUFFERS_DYNAMIC = 1024;
        static constexpr uint16_t DESCRIPTOR_MAX_SAMPLERS                 = 1024;
        static constexpr uint16_t DESCRIPTOR_MAX_TEXTURES                 = 1024;

        static constexpr uint8_t DESCRIPTOR_MAX_SETS        = 8;
        static constexpr uint8_t DESCRIPTOR_MAX_DESCRIPTORS = 16;

        enum class CullMode : uint8_t
        {
            FRONT = 0,
            BACK,
            FRONTANDBACK,
            NONE
        };

        enum class PolygonMode : uint8_t
        {
            FILL = 0,
            LINE,
            POINT
        };

        enum class BlendMode : uint8_t
        {
            None = 0,
            OneZero,
            ZeroSrcColor,
            OneMinusSrcAlpha,
            SrcAlphaOneMinusSrcAlpha,
            SrcAlphaOne
        };

        enum class TextureWrap : uint8_t
        {
            NONE = 0,
            REPEAT,
            CLAMP,
            MIRRORED_REPEAT,
            CLAMP_TO_EDGE,
            CLAMP_TO_BORDER
        };

        enum class TextureFilter : uint8_t
        {
            NONE = 0,
            LINEAR,
            NEAREST
        };

        enum class RHIFormat : uint8_t
        {
            NONE = 0,
            R8_Unorm,
            R8G8_Unorm,
            R8G8B8_Unorm,
            R8G8B8A8_Unorm,

            R8_UInt,

            R11G11B10_Float,
            R10G10B10A2_Unorm,

            R32_Int,
            R32G32_Int,
            R32G32B32_Int,
            R32G32B32A32_Int,

            R32_UInt,
            R32G32_UInt,
            R32G32B32_UInt,
            R32G32B32A32_UInt,

            R16_Float,
            R16G16_Float,
            R16G16B16_Float,
            R16G16B16A16_Float,

            R32_Float,
            R32G32_Float,
            R32G32B32_Float,
            R32G32B32A32_Float,

            D16_Unorm,
            D32_Float,
            D16_Unorm_S8_UInt,
            D24_Unorm_S8_UInt,
            D32_Float_S8_UInt,
            SCREEN
        };

        enum class BufferUsage : uint8_t
        {
            STATIC,
            DYNAMIC,
            STREAM
        };

        enum class DescriptorType : uint8_t
        {
            UNIFORM_BUFFER,
            UNIFORM_BUFFER_DYNAMIC,
            IMAGE_SAMPLER,
            IMAGE_STORAGE
        };

        enum class ShaderDataType : uint8_t
        {
            NONE = 0,
            FLOAT32,
            VEC2,
            VEC3,
            VEC4,
            IVEC2,
            IVEC3,
            IVEC4,
            MAT3,
            MAT4,
            INT32,
            INT,
            UINT,
            BOOL,
            STRUCT,
            MAT4ARRAY
        };

        enum class ShaderType : uint8_t
        {
            VERTEX = 0,
            FRAGMENT,
            GEOMETRY,
            TESSELLATION_CONTROL,
            TESSELLATION_EVALUATION,
            COMPUTE,
            UNKNOWN
        };

        enum class TextureType : uint8_t
        {
            COLOUR = 0,
            DEPTH,
            DEPTHARRAY,
            CUBE,
            OTHER
        };

        enum SubPassContents : uint8_t
        {
            INLINE = 0,
            SECONDARY
        };

        enum TextureFlags : uint8_t
        {
            Texture_Sampled              = BIT(0),
            Texture_Storage              = BIT(1),
            Texture_RenderTarget         = BIT(2),
            Texture_DepthStencil         = BIT(3),
            Texture_DepthStencilReadOnly = BIT(4),
            Texture_CreateMips           = BIT(5),
            Texture_MipViews             = BIT(6)
        };

        enum RendererBufferType : uint8_t
        {
            RENDERER_BUFFER_COLOUR  = BIT(0),
            RENDERER_BUFFER_DEPTH   = BIT(1),
            RENDERER_BUFFER_STENCIL = BIT(2),
            RENDERER_BUFFER_NONE    = BIT(3)

        };

        enum class DrawType : uint8_t
        {
            POINT = 0,
            TRIANGLE,
            LINES
        };

        enum class StencilType : uint8_t
        {
            EQUAL = 0,
            NOTEQUAL,
            KEEP,
            REPLACE,
            ZERO,
            ALWAYS
        };

        enum class PixelPackType : uint8_t
        {
            PACK = 0,
            UNPACK
        };

        enum class RendererBlendFunction : uint8_t
        {
            NONE = 0,
            ZERO,
            ONE,
            SOURCE_ALPHA,
            DESTINATION_ALPHA,
            ONE_MINUS_SOURCE_ALPHA
        };

        enum class RendererBlendEquation : uint8_t
        {
            NONE = 0,
            ADD,
            SUBTRACT
        };

        enum class RenderMode : uint8_t
        {
            FILL = 0,
            WIREFRAME
        };

        enum class DataType : uint8_t
        {
            FLOAT = 0,
            UNSIGNED_INT,
            UNSIGNED_BYTE
        };

        enum class PhysicalDeviceType : uint8_t
        {
            DISCRETE   = 0,
            INTEGRATED = 1,
            VIRTUAL    = 2,
            CPU        = 3,
            UNKNOWN    = 4
        };

        struct BufferMemberInfo
        {
            uint32_t size;
            uint32_t offset;
            ShaderDataType type;
            std::string name;
            std::string fullName;
        };

        struct VertexInputDescription
        {
            uint32_t binding;
            uint32_t location;
            RHIFormat format;
            uint32_t offset;
        };

        struct DescriptorPoolInfo
        {
            DescriptorType type;
            uint32_t size;
        };

        struct DescriptorLayoutInfo
        {
            DescriptorType type;
            ShaderType stage;
            uint32_t binding = 0;
            uint32_t setID   = 0;
            uint32_t count   = 1;
        };

        struct DescriptorLayout
        {
            uint32_t count;
            DescriptorLayoutInfo* layoutInfo;
        };

        struct DescriptorDesc
        {
            uint32_t layoutIndex;
            Shader* shader;
            uint32_t count = 1;
        };

        struct Descriptor
        {
            Texture** textures;
            Texture* texture;
            UniformBuffer* buffer;

            uint32_t offset;
            uint32_t size;
            uint32_t binding;
            uint32_t textureCount = 1;
            uint32_t mipLevel     = 0;

            TextureType textureType;
            DescriptorType type = DescriptorType::IMAGE_SAMPLER;
            ShaderType shaderType;

            TDArray<BufferMemberInfo> m_Members;
            std::string name;
        };

        enum class CubeFace : uint8_t
        {
            PositiveX = 0,
            NegativeX,
            PositiveY,
            NegativeY,
            PositiveZ,
            NegativeZ
        };

        struct FramebufferDesc
        {
            uint32_t width           = 0;
            uint32_t height          = 0;
            uint32_t layer           = 0;
            uint32_t attachmentCount = 0;
            uint32_t samples         = 1;
            int mipIndex             = 0;
            bool screenFBO           = false;
            Texture** attachments;
            TextureType* attachmentTypes;
            Graphics::RenderPass* renderPass;
        };

        struct RenderPassDesc
        {
            Texture** attachments;
            TextureType* attachmentTypes;
            uint32_t attachmentCount;
            bool clear           = true;
            bool swapchainTarget = false;
            int cubeMapIndex     = -1;
            int mipIndex         = 0;
            int samples          = 1;
            std::string DebugName;
            Texture* resolveTexture = nullptr;
        };

        struct TextureDesc
        {
            RHIFormat format;
            TextureFilter minFilter;
            TextureFilter magFilter;
            TextureWrap wrap;
            uint8_t samples           = 1;
            uint16_t flags            = TextureFlags::Texture_CreateMips;
            bool srgb                 = false;
            bool generateMipMaps      = true;
            bool anisotropicFiltering = true;

            TextureDesc()
            {
                format    = RHIFormat::R8G8B8A8_Unorm;
                minFilter = TextureFilter::NEAREST;
                magFilter = TextureFilter::NEAREST;
                wrap      = TextureWrap::REPEAT;
                samples   = 1;
            }

            TextureDesc(RHIFormat format, TextureFilter minFilter, TextureFilter magFilter, TextureWrap wrap)
                : format(format)
                , minFilter(minFilter)
                , magFilter(magFilter)
                , wrap(wrap)
            {
            }

            TextureDesc(TextureFilter minFilter, TextureFilter magFilter)
                : format(RHIFormat::R8G8B8A8_Unorm)
                , minFilter(minFilter)
                , magFilter(magFilter)
                , wrap(TextureWrap::CLAMP)
            {
            }

            TextureDesc(TextureFilter minFilter, TextureFilter magFilter, TextureWrap wrap)
                : format(RHIFormat::R8G8B8A8_Unorm)
                , minFilter(minFilter)
                , magFilter(magFilter)
                , wrap(wrap)
            {
            }

            TextureDesc(TextureWrap wrap)
                : format(RHIFormat::R8G8B8A8_Unorm)
                , minFilter(TextureFilter::LINEAR)
                , magFilter(TextureFilter::LINEAR)
                , wrap(wrap)
            {
            }

            TextureDesc(RHIFormat format)
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

            TextureLoadOptions()
            {
                flipX = false;
                flipY = false;
            }

            TextureLoadOptions(bool flipX, bool flipY)
                : flipX(flipX)
                , flipY(flipY)
            {
            }
        };

        struct PushConstant
        {
            uint32_t size;
            ShaderType shaderStage;
            uint8_t* data;
            uint32_t offset = 0;
            std::string name;

            TDArray<BufferMemberInfo> m_Members;

            inline void SetValue(const std::string& name, void* value)
            {
                for(auto& member : m_Members)
                {
                    if(member.name == name)
                    {
                        memcpy(&data[member.offset], value, member.size);
                        return;
                    }
                }

                LWARN("Pushconst not found %s", name.c_str());
            }

            inline void SetData(void* value)
            {
                memcpy(data, value, size);
            }
        };

        struct DescriptorSetInfo
        {
            TDArray<Descriptor> descriptors;
        };
    }
}

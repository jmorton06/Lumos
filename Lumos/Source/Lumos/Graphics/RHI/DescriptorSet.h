#pragma once

namespace Lumos
{
    namespace Graphics
    {
        class Pipeline;
        class Shader;
        class Texture;
        class UniformBuffer;
        enum class TextureType : int;
        enum class ShaderType : int;

        enum class DescriptorType
        {
            UNIFORM_BUFFER,
            UNIFORM_BUFFER_DYNAMIC,
            IMAGE_SAMPLER
        };

        enum class Format
        {
            R32G32B32A32_UINT,
            R32G32B32_UINT,
            R32G32_UINT,
            R32_UINT,
            R8_UINT,
            R32G32B32A32_INT,
            R32G32B32_INT,
            R32G32_INT,
            R32_INT,
            R32G32B32A32_FLOAT,
            R32G32B32_FLOAT,
            R32G32_FLOAT,
            R32_FLOAT
        };

        enum class ShaderDataType
        {
            NONE,
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
            Format format;
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
            uint32_t setID = 0;
            uint32_t count = 1;
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
            std::string name;

            TextureType textureType;
            DescriptorType type = DescriptorType::IMAGE_SAMPLER;
            ShaderType shaderType;

            std::vector<BufferMemberInfo> m_Members;
        };

        struct DescriptorSetInfo
        {
            std::vector<Descriptor> descriptors;
        };

        class DescriptorSet
        {
        public:
            virtual ~DescriptorSet() = default;
            static DescriptorSet* Create(const DescriptorDesc& info);

            virtual void Update(std::vector<Descriptor>& descriptors) = 0;
            virtual void SetDynamicOffset(uint32_t offset) = 0;
            virtual uint32_t GetDynamicOffset() const = 0;

        protected:
            static DescriptorSet* (*CreateFunc)(const DescriptorDesc&);
        };
    }
}

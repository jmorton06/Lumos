#pragma once
#include "DescriptorSet.h"

namespace spirv_cross
{
    class SPIRType;
}
namespace Lumos
{
    namespace Graphics
    {
        enum class ShaderType : int
        {
            VERTEX,
            FRAGMENT,
            GEOMETRY,
            TESSELLATION_CONTROL,
            TESSELLATION_EVALUATION,
            COMPUTE,
            UNKNOWN
        };

        struct PushConstant
        {
            uint32_t size;
            ShaderType shaderStage;
            uint8_t* data;
            uint32_t offset = 0;
            std::string name;

            std::vector<BufferMemberInfo> m_Members;

            void SetValue(const std::string& name, void* value)
            {
                for(auto& member : m_Members)
                {
                    if(member.name == name)
                    {
                        memcpy(&data[member.offset], value, member.size);
                        break;
                    }
                }
            }

            void SetData(void* value)
            {
                memcpy(data, value, size);
            }
        };

        struct ShaderEnumClassHash
        {
            template <typename T>
            std::size_t operator()(T t) const
            {
                return static_cast<std::size_t>(t);
            }
        };

        class CommandBuffer;
        class Pipeline;
        class DescriptorSet;

        template <typename Key>
        using HashType = typename std::conditional<std::is_enum<Key>::value, ShaderEnumClassHash, std::hash<Key>>::type;

        template <typename Key, typename T>
        using UnorderedMap = std::unordered_map<Key, T, HashType<Key>>;

        class LUMOS_EXPORT Shader
        {
        public:
            static const Shader* s_CurrentlyBound;

        public:
            virtual void Bind() const = 0;
            virtual void Unbind() const = 0;

            virtual ~Shader() = default;

            virtual const std::vector<ShaderType> GetShaderTypes() const = 0;
            virtual const std::string& GetName() const = 0;
            virtual const std::string& GetFilePath() const = 0;

            virtual void* GetHandle() const = 0;

            virtual std::vector<PushConstant>& GetPushConstants() = 0;
            virtual PushConstant* GetPushConstant(uint32_t index) { return nullptr; }
            virtual void BindPushConstants(Graphics::CommandBuffer* cmdBuffer, Graphics::Pipeline* pipeline) = 0;
            virtual DescriptorSet* CreateDescriptorSet(uint32_t index) { return nullptr; };
            virtual DescriptorSetInfo GetDescriptorInfo(uint32_t index) { return DescriptorSetInfo(); }

            ShaderDataType SPIRVTypeToLumosDataType(const spirv_cross::SPIRType type);

        public:
            static Shader* CreateFromFile(const std::string& filepath);

        protected:
            static Shader* (*CreateFunc)(const std::string&);
        };
    }
}

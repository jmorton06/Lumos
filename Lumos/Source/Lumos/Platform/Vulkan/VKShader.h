#pragma once

#include "VK.h"
#include "Graphics/RHI/Shader.h"
#include "Graphics/RHI/DescriptorSet.h"

namespace Lumos
{
    namespace Graphics
    {
        class VKShader : public Shader
        {
        public:
            VKShader(const char* filePath);
            VKShader(const uint32_t* vertData, uint32_t vertDataSize, const uint32_t* fragData, uint32_t fragDataSize);
            VKShader(const uint32_t* compData, uint32_t compDataSize);
            ~VKShader();

            bool Init();
            void Unload();

            VkPipelineShaderStageCreateInfo* GetShaderStages() const;
            uint32_t GetStageCount() const;

            void Bind() const override { };
            void Unbind() const override { };

            const TDArray<ShaderType> GetShaderTypes() const override
            {
                return TDArray<ShaderType>();
            };

            const char* GetName() const override
            {
                return m_Name.c_str();
            }

            const char* GetFilePath() const override
            {
                return m_FilePath.c_str();
            };

            bool IsCompiled() const override
            {
                return m_Compiled;
            }

            bool IsCompute()
            {
                if(m_ShaderTypes.Size() > 0)
                    return m_ShaderTypes[0] == ShaderType::COMPUTE;

                return false;
            }

            PushConstant* GetPushConstant(uint32_t index) override
            {
                ASSERT(index < m_PushConstants.Size(), "Push constants out of bounds");
                return &m_PushConstants[index];
            }

            TDArray<PushConstant>& GetPushConstants() override
            {
                return m_PushConstants;
            }

            VkDescriptorSetLayout* GetDescriptorLayout(int id)
            {
                return &m_DescriptorSetLayouts[id];
            };

            VkPipelineLayout GetPipelineLayout() const { return m_PipelineLayout; }

            DescriptorSetInfo GetDescriptorInfo(uint32_t index) override
            {
                if(m_DescriptorInfos.find(index) != m_DescriptorInfos.end())
                {
                    return m_DescriptorInfos[index];
                }

                LWARN("DescriptorDesc not found. Index = %s", index);
                return DescriptorSetInfo();
            }

            const TDArray<DescriptorLayoutInfo>& GetDescriptorLayout() const { return m_DescriptorLayoutInfo; }
            const TDArray<VkDescriptorSetLayout>& GetDescriptorLayouts() const { return m_DescriptorSetLayouts; }
            void BindPushConstants(Graphics::CommandBuffer* commandBuffer, Graphics::Pipeline* pipeline) override;

            static void PreProcess(const std::string& source, std::map<ShaderType, std::string>* sources);
            static void ReadShaderFile(const TDArray<std::string>& lines, std::map<ShaderType, std::string>* shaders);

            static void MakeDefault();

            void* GetHandle() const override
            {
                return nullptr;
            }

            const TDArray<VkVertexInputAttributeDescription>& GetVertexInputAttributeDescription() const { return m_VertexInputAttributeDescriptions; }
            const uint32_t GetVertexInputStride() const { return m_VertexInputStride; }

            uint64_t GetHash() const override { return m_Hash; }

        protected:
            static Shader* CreateFuncVulkan(const char*);
            static Shader* CreateFromEmbeddedFuncVulkan(const uint32_t* vertData, uint32_t vertDataSize, const uint32_t* fragData, uint32_t fragDataSize);
            static Shader* CreateCompFromEmbeddedFuncVulkan(const uint32_t* compData, uint32_t compDataSize);
            void LoadFromData(const uint32_t* data, uint32_t size, ShaderType type, int currentShaderStage);
            void CreatePipelineLayout();

        private:
            std::unordered_map<uint32_t, DescriptorSetInfo> m_DescriptorInfos;

            VkPipelineShaderStageCreateInfo* m_ShaderStages;
            uint32_t m_StageCount = 0;
            std::string m_Name;
            std::string m_FilePath;
            std::string m_Source;
            TDArray<ShaderType> m_ShaderTypes;
            bool m_Compiled = false;

            TDArray<VkVertexInputAttributeDescription> m_VertexInputAttributeDescriptions;
            uint32_t m_VertexInputStride = 0;
            uint64_t m_Hash              = 0;

            VkPipelineLayout m_PipelineLayout;
            TDArray<PushConstant> m_PushConstants;
            TDArray<Graphics::DescriptorLayoutInfo> m_DescriptorLayoutInfo;
            TDArray<VkDescriptorSetLayout> m_DescriptorSetLayouts;
        };
    }
}

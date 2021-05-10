#pragma once

#include "VK.h"
#include "Graphics/API/Shader.h"
#include "Graphics/API/DescriptorSet.h"

namespace Lumos
{
    namespace Graphics
    {
        class VKShader : public Shader
        {
        public:
            VKShader(const std::string& filePath);
            ~VKShader();

            bool Init();
            void Unload() const;

            VkPipelineShaderStageCreateInfo* GetShaderStages() const;
            uint32_t GetStageCount() const;

            void Bind() const override {};
            void Unbind() const override {};

            const std::vector<ShaderType> GetShaderTypes() const override
            {
                return std::vector<ShaderType>();
            };

            const std::string& GetName() const override
            {
                return m_Name;
            }
            const std::string& GetFilePath() const override
            {
                return m_FilePath;
            };

            PushConstant* GetPushConstant(uint32_t index) override
            {
                LUMOS_ASSERT(index < m_PushConstants.size(), "Push constants out of bounds");
                return &m_PushConstants[index];
            }

            std::vector<PushConstant>& GetPushConstants() override { return m_PushConstants; }
            const std::vector<DescriptorLayoutInfo>& GetDescriptorLayout() const { return m_DescriptorLayoutInfo; }
            void BindPushConstants(Graphics::CommandBuffer* cmdBuffer, Graphics::Pipeline* pipeline) override;

            static void PreProcess(const std::string& source, std::map<ShaderType, std::string>* sources);
            static void ReadShaderFile(std::vector<std::string> lines, std::map<ShaderType, std::string>* shaders);

            static void MakeDefault();

            void* GetHandle() const override
            {
                return nullptr;
            }

            const std::vector<VkVertexInputAttributeDescription>& GetVertexInputAttributeDescription() const { return m_VertexInputAttributeDescriptions; }
            const uint32_t GetVertexInputStride() const { return m_VertexInputStride; }

        protected:
            static Shader* CreateFuncVulkan(const std::string&);

        private:
            VkPipelineShaderStageCreateInfo* m_ShaderStages;
            uint32_t m_StageCount;
            std::string m_Name;
            std::string m_FilePath;
            std::string m_Source;
            std::vector<ShaderType> m_ShaderTypes;

            std::vector<VkVertexInputAttributeDescription> m_VertexInputAttributeDescriptions;
            uint32_t m_VertexInputStride = 0;

            VkPipelineLayout m_PipelineLayout;
            std::vector<PushConstant> m_PushConstants;
            std::vector<Graphics::DescriptorLayoutInfo> m_DescriptorLayoutInfo;
        };
    }
}

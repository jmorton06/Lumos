#pragma once

#include "VK.h"
#include "Graphics/API/Shader.h"

namespace Lumos
{
	namespace Graphics
	{
		class VKShader : public Shader
		{
		public:
			VKShader(const std::string& shaderName, const std::string& filePath);
			~VKShader();

			bool Init();
			void Unload() const;

			VkPipelineShaderStageCreateInfo* GetShaderStages() const;
			uint32_t GetStageCount() const;

			void Bind() const override {};
			void Unbind() const override {};

			void SetSystemUniformBuffer(ShaderType type, u8* data, u32 size, u32 slot = 0) override {};
			void SetUserUniformBuffer(ShaderType type, u8* data, u32 size) override {};

			const ShaderUniformBufferList GetSystemUniforms(ShaderType type) const override { return ShaderUniformBufferList(); };
			const ShaderUniformBufferDeclaration* GetUserUniformBuffer(ShaderType type) const override { return nullptr; };

			const std::vector<ShaderType> GetShaderTypes() const override { return std::vector<ShaderType>(); };

			const String& GetName() const override { return m_Name; }
			const String& GetFilePath() const override { return m_FilePath; };

			static void PreProcess(const String& source, std::map<ShaderType, String>* sources);
			static void ReadShaderFile(std::vector<String> lines, std::map<ShaderType, String>* shaders);

            static void MakeDefault();
        
            void* GetHandle() const override { return nullptr; }
        protected:
            static Shader* CreateFuncVulkan(const String&, const String&);
            
		private:
			VkPipelineShaderStageCreateInfo* 	m_ShaderStages;
			uint32_t 							m_StageCount;
			String 								m_Name;
			String								m_FilePath;
			String 								m_Source;
			std::vector<ShaderType> 			m_ShaderTypes;
		};
	}
}

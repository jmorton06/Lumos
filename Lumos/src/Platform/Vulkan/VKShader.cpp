#include "Precompiled.h"
#include "VKShader.h"
#include "VKDevice.h"
#include "VKTools.h"
#include "Core/OS/FileSystem.h"
#include "Core/VFS.h"

namespace Lumos
{
	namespace Graphics
	{
		static ShaderType type = ShaderType::UNKNOWN;

		VKShader::VKShader(const std::string& shaderName, const std::string& filePath)
			: m_StageCount(0)
			, m_Name(shaderName)
			, m_FilePath(filePath)
		{
			m_ShaderStages = VK_NULL_HANDLE;

			m_Source = VFS::Get()->ReadTextFile(filePath + shaderName + ".shader");

			Init();
		}

		VKShader::~VKShader()
		{
			Unload();
			delete[] m_ShaderStages;
			m_ShaderStages = VK_NULL_HANDLE;
		}

		bool VKShader::Init()
		{
			uint32_t currentShaderStage = 0;
			m_StageCount = 0;

			std::map<ShaderType, std::string>* files = new std::map<ShaderType, std::string>();
			PreProcess(m_Source, files);

			for(auto& source : *files)
			{
				m_ShaderTypes.push_back(source.first);
				m_StageCount++;
			}

			m_ShaderStages = new VkPipelineShaderStageCreateInfo[m_StageCount];

			for(uint32_t i = 0; i < m_StageCount; i++)
				m_ShaderStages[i] = VkPipelineShaderStageCreateInfo();

			for(auto& file : *files)
			{
				auto fileSize = FileSystem::GetFileSize(m_FilePath + file.second); //TODO: once process
				u8* source = FileSystem::ReadFile(m_FilePath + file.second);
				VkShaderModuleCreateInfo vertexShaderCI{};
				vertexShaderCI.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
				vertexShaderCI.codeSize = fileSize;
				vertexShaderCI.pCode = reinterpret_cast<uint32_t*>(source);
				vertexShaderCI.pNext = VK_NULL_HANDLE;

				m_ShaderStages[currentShaderStage].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				m_ShaderStages[currentShaderStage].stage = VKTools::ShaderTypeToVK(file.first);
				m_ShaderStages[currentShaderStage].pName = "main";
				m_ShaderStages[currentShaderStage].pNext = VK_NULL_HANDLE;

				VK_CHECK_RESULT(vkCreateShaderModule(VKDevice::Get().GetDevice(), &vertexShaderCI, nullptr, &m_ShaderStages[currentShaderStage].module));

				delete[] source;

				currentShaderStage++;
			}

			delete files;
			return true;
		}

		void VKShader::Unload() const
		{
			for(uint32_t i = 0; i < m_StageCount; i++)
			{
				vkDestroyShaderModule(VKDevice::Get().GetDevice(), m_ShaderStages[i].module, nullptr);
			}
		}

		VkPipelineShaderStageCreateInfo* VKShader::GetShaderStages() const
		{
			return m_ShaderStages;
		}

		uint32_t VKShader::GetStageCount() const
		{
			return m_StageCount;
		}

		void VKShader::PreProcess(const std::string& source, std::map<ShaderType, std::string>* sources)
		{
			type = ShaderType::UNKNOWN;
			std::vector<std::string> lines = StringUtilities::GetLines(source);
			ReadShaderFile(lines, sources);
		}

		void VKShader::ReadShaderFile(std::vector<std::string> lines, std::map<ShaderType, std::string>* shaders)
		{
			for(u32 i = 0; i < lines.size(); i++)
			{
				std::string str = std::string(lines[i]);
				str = StringUtilities::StringReplace(str, '\t');

				if(StringUtilities::StartsWith(str, "#shader"))
				{
					if(StringUtilities::StringContains(str, "vertex"))
					{
						type = ShaderType::VERTEX;
						std::map<ShaderType, std::string>::iterator it = shaders->begin();
						shaders->insert(it, std::pair<ShaderType, std::string>(type, ""));
					}
					else if(StringUtilities::StringContains(str, "geometry"))
					{
						type = ShaderType::GEOMETRY;
						std::map<ShaderType, std::string>::iterator it = shaders->begin();
						shaders->insert(it, std::pair<ShaderType, std::string>(type, ""));
					}
					else if(StringUtilities::StringContains(str, "fragment"))
					{
						type = ShaderType::FRAGMENT;
						std::map<ShaderType, std::string>::iterator it = shaders->begin();
						shaders->insert(it, std::pair<ShaderType, std::string>(type, ""));
					}
					else if(StringUtilities::StringContains(str, "tess_cont"))
					{
						type = ShaderType::TESSELLATION_CONTROL;
						std::map<ShaderType, std::string>::iterator it = shaders->begin();
						shaders->insert(it, std::pair<ShaderType, std::string>(type, ""));
					}
					else if(StringUtilities::StringContains(str, "tess_eval"))
					{
						type = ShaderType::TESSELLATION_EVALUATION;
						std::map<ShaderType, std::string>::iterator it = shaders->begin();
						shaders->insert(it, std::pair<ShaderType, std::string>(type, ""));
					}
					else if(StringUtilities::StringContains(str, "compute"))
					{
						type = ShaderType::COMPUTE;
						std::map<ShaderType, std::string>::iterator it = shaders->begin();
						shaders->insert(it, std::pair<ShaderType, std::string>(type, ""));
					}
					else if(StringUtilities::StringContains(str, "end"))
					{
						type = ShaderType::UNKNOWN;
					}
				}
				else if(type != ShaderType::UNKNOWN)
				{
					shaders->at(type).append(lines[i]);
				}
			}
		}

		void VKShader::MakeDefault()
		{
			CreateFunc = CreateFuncVulkan;
		}

		Shader* VKShader::CreateFuncVulkan(const std::string& name, const std::string& source)
		{
			std::string physicalPath;
			Lumos::VFS::Get()->ResolvePhysicalPath(source, physicalPath, true);
			return new VKShader(name, physicalPath);
		}

	}
}

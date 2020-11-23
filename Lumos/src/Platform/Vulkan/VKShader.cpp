#include "Precompiled.h"
#include "VKShader.h"
#include "VKDevice.h"
#include "VKTools.h"
#include "Core/OS/FileSystem.h"
#include "Core/VFS.h"
#include "VKDescriptorSet.h"

#include <spirv_cross.hpp>

#define SHADER_LOG_ENABLED 0

#if SHADER_LOG_ENABLED
#define SHADER_LOG(x) x
#else
#define SHADER_LOG(x)
#endif

namespace Lumos
{
	namespace Graphics
	{
		static ShaderType type = ShaderType::UNKNOWN;

		VKShader::VKShader(const std::string& filePath)
			: m_StageCount(0)
		{
			m_ShaderStages = VK_NULL_HANDLE;
            m_Name = StringUtilities::GetFileName(filePath);
            m_FilePath = StringUtilities::GetFileLocation(filePath);
			m_Source = VFS::Get()->ReadTextFile(filePath);

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

            std::map<ShaderType, std::string> files;
			PreProcess(m_Source, &files);

			for(auto& source : files)
			{
				m_ShaderTypes.push_back(source.first);
				m_StageCount++;
			}

			m_ShaderStages = new VkPipelineShaderStageCreateInfo[m_StageCount];

			for(uint32_t i = 0; i < m_StageCount; i++)
				m_ShaderStages[i] = VkPipelineShaderStageCreateInfo();

            LUMOS_LOG_INFO("Loading Shader : {0}", m_Name);

			for(auto& file : files)
			{
				u32 fileSize = u32(FileSystem::GetFileSize(m_FilePath + file.second));
                u32* source = reinterpret_cast<u32*>(FileSystem::ReadFile(m_FilePath + file.second));

				VkShaderModuleCreateInfo shaderCreateInfo{};
                shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
                shaderCreateInfo.codeSize = fileSize;
                shaderCreateInfo.pCode = source;
                shaderCreateInfo.pNext = VK_NULL_HANDLE;
                
                std::vector<u32> spv(source, source + fileSize / sizeof(u32));

                spirv_cross::Compiler comp(std::move(spv));
                // The SPIR-V is now parsed, and we can perform reflection on it.
                spirv_cross::ShaderResources resources = comp.get_shader_resources();
                
                for (auto &u : resources.uniform_buffers)
                {
                    uint32_t set = comp.get_decoration(u.id, spv::DecorationDescriptorSet);
                    uint32_t binding = comp.get_decoration(u.id, spv::DecorationBinding);
                    auto& type = comp.get_type(u.type_id);

                    SHADER_LOG(LUMOS_LOG_INFO("Found UBO {0} at set = {1}, binding = {2}", u.name.c_str(), set, binding));
                    m_DescriptorLayoutInfo.push_back({Graphics::DescriptorType::UNIFORM_BUFFER, file.first, binding, set, type.array.size() ? u32(type.array[0]) : 1});

                }
                
                for (auto &u : resources.push_constant_buffers)
                {
                    uint32_t set = comp.get_decoration(u.id, spv::DecorationDescriptorSet);
                    uint32_t binding = comp.get_decoration(u.id, spv::DecorationBinding);
                    
                    uint32_t binding3 = comp.get_decoration(u.id, spv::DecorationOffset);

                    auto& type = comp.get_type(u.type_id);
                    
                    auto ranges = comp.get_active_buffer_ranges(u.id);
                    
                    u32 size = 0;
                    for(auto& range : ranges)
                    {
                        SHADER_LOG(LUMOS_LOG_INFO("Accessing Member {0} offset {1}, size {2}", range.index, range.offset, range.range));
                        size += range.range;
                    }

                    SHADER_LOG(LUMOS_LOG_INFO("Found Push Constant {0} at set = {1}, binding = {2}", u.name.c_str(), set, binding, type.array.size() ? u32(type.array[0]) : 1));
                    
                    m_PushConstants.push_back({size, file.first});
                }
                
                for (auto &u : resources.sampled_images)
                {
                    uint32_t set = comp.get_decoration(u.id, spv::DecorationDescriptorSet);
                    uint32_t binding = comp.get_decoration(u.id, spv::DecorationBinding);
                    
                    auto& type = comp.get_type(u.type_id);
                    SHADER_LOG(LUMOS_LOG_INFO("Found Sampled Image {0} at set = {1}, binding = {2}", u.name.c_str(), set, binding));
                    
                    m_DescriptorLayoutInfo.push_back({Graphics::DescriptorType::IMAGE_SAMPLER, file.first, binding, set, type.array.size() ? u32(type.array[0]) : 1});

                }
                
				m_ShaderStages[currentShaderStage].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				m_ShaderStages[currentShaderStage].stage = VKTools::ShaderTypeToVK(file.first);
				m_ShaderStages[currentShaderStage].pName = "main";
				m_ShaderStages[currentShaderStage].pNext = VK_NULL_HANDLE;

				VK_CHECK_RESULT(vkCreateShaderModule(VKDevice::Get().GetDevice(), &shaderCreateInfo, nullptr, &m_ShaderStages[currentShaderStage].module));

				delete[] source;

				currentShaderStage++;
			}

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

		Shader* VKShader::CreateFuncVulkan(const std::string& filepath)
		{
			std::string physicalPath;
			Lumos::VFS::Get()->ResolvePhysicalPath(filepath, physicalPath, false);
			return new VKShader(physicalPath);
		}

	}
}

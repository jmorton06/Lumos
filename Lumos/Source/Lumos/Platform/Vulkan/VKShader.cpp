#include "Precompiled.h"
#include "VKShader.h"
#include "VKDevice.h"
#include "VKUtilities.h"
#include "VKDescriptorSet.h"
#include "VKPipeline.h"
#include "VKCommandBuffer.h"
#include "VKInitialisers.h"
#include "VKUniformBuffer.h"
#include "Graphics/Material.h"
#include "Utilities/CombineHash.h"
#include "Core/OS/FileSystem.h"
#include "Core/OS/FileSystem.h"
#include "Utilities/StringUtilities.h"
#include "Maths/Vector3.h"
#include "Maths/IVector4.h"
#include "Core/Algorithms/Sort.h"

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

        VkFormat GetVulkanFormat(const spirv_cross::SPIRType& type)
        {
            using namespace spirv_cross;
            if(type.basetype == SPIRType::Struct || type.basetype == SPIRType::Sampler)
            {
                LWARN("Tried to convert a structure or SPIR sampler into a VkFormat enum value!");
                return VK_FORMAT_UNDEFINED;
            }
            else if(type.basetype == SPIRType::Image || type.basetype == SPIRType::SampledImage)
            {
                switch(type.image.format)
                {
                case spv::ImageFormatR8:
                    return VK_FORMAT_R8_UNORM;
                case spv::ImageFormatR8Snorm:
                    return VK_FORMAT_R8_SNORM;
                case spv::ImageFormatR8ui:
                    return VK_FORMAT_R8_UINT;
                case spv::ImageFormatR8i:
                    return VK_FORMAT_R8_SINT;
                case spv::ImageFormatRg8:
                    return VK_FORMAT_R8G8_UNORM;
                case spv::ImageFormatRg8Snorm:
                    return VK_FORMAT_R8G8_SNORM;
                case spv::ImageFormatRg8ui:
                    return VK_FORMAT_R8G8_UINT;
                case spv::ImageFormatRg8i:
                    return VK_FORMAT_R8G8_SINT;
                case spv::ImageFormatRgba8i:
                    return VK_FORMAT_R8G8B8A8_SINT;
                case spv::ImageFormatRgba8ui:
                    return VK_FORMAT_R8G8B8A8_UINT;
                case spv::ImageFormatRgba8:
                    return VK_FORMAT_R8G8B8A8_UNORM;
                case spv::ImageFormatRgba8Snorm:
                    return VK_FORMAT_R8G8B8A8_SNORM;
                case spv::ImageFormatR32i:
                    return VK_FORMAT_R32_SINT;
                case spv::ImageFormatR32ui:
                    return VK_FORMAT_R32_UINT;
                case spv::ImageFormatRg32i:
                    return VK_FORMAT_R32G32_SINT;
                case spv::ImageFormatRg32ui:
                    return VK_FORMAT_R32G32_UINT;
                case spv::ImageFormatRgba32f:
                    return VK_FORMAT_R32G32B32A32_SFLOAT;
                case spv::ImageFormatRgba16f:
                    return VK_FORMAT_R16G16B16A16_SFLOAT;
                case spv::ImageFormatR32f:
                    return VK_FORMAT_R32_SFLOAT;
                case spv::ImageFormatRg32f:
                    return VK_FORMAT_R32G32_SFLOAT;
                case spv::ImageFormatR16f:
                    return VK_FORMAT_R16_SFLOAT;
                case spv::ImageFormatRgba32i:
                    return VK_FORMAT_R32G32B32A32_SINT;
                case spv::ImageFormatRgba32ui:
                    return VK_FORMAT_R32G32B32A32_UINT;
                default:
                    LWARN("Failed to convert an image format to a VkFormat enum.");
                    return VK_FORMAT_UNDEFINED;
                }
            }
            else if(type.vecsize == 1)
            {
                if(type.width == 8)
                {
                    switch(type.basetype)
                    {
                    case SPIRType::Int:
                        return VK_FORMAT_R8_SINT;
                    case SPIRType::UInt:
                        return VK_FORMAT_R8_UINT;
                    default:
                        return VK_FORMAT_UNDEFINED;
                    }
                }
                else if(type.width == 16)
                {
                    switch(type.basetype)
                    {
                    case SPIRType::Int:
                        return VK_FORMAT_R16_SINT;
                    case SPIRType::UInt:
                        return VK_FORMAT_R16_UINT;
                    case SPIRType::Float:
                        return VK_FORMAT_R16_SFLOAT;
                    default:
                        return VK_FORMAT_UNDEFINED;
                    }
                }
                else if(type.width == 32)
                {
                    switch(type.basetype)
                    {
                    case SPIRType::Int:
                        return VK_FORMAT_R32_SINT;
                    case SPIRType::UInt:
                        return VK_FORMAT_R32_UINT;
                    case SPIRType::Float:
                        return VK_FORMAT_R32_SFLOAT;
                    default:
                        return VK_FORMAT_UNDEFINED;
                    }
                }
                else if(type.width == 64)
                {
                    switch(type.basetype)
                    {
                    case SPIRType::Int64:
                        return VK_FORMAT_R64_SINT;
                    case SPIRType::UInt64:
                        return VK_FORMAT_R64_UINT;
                    case SPIRType::Double:
                        return VK_FORMAT_R64_SFLOAT;
                    default:
                        return VK_FORMAT_UNDEFINED;
                    }
                }
                else
                {
                    LWARN("Invalid type width for conversion of SPIR-Type to VkFormat enum value!");
                    return VK_FORMAT_UNDEFINED;
                }
            }
            else if(type.vecsize == 2)
            {
                if(type.width == 8)
                {
                    switch(type.basetype)
                    {
                    case SPIRType::Int:
                        return VK_FORMAT_R8G8_SINT;
                    case SPIRType::UInt:
                        return VK_FORMAT_R8G8_UINT;
                    default:
                        return VK_FORMAT_UNDEFINED;
                    }
                }
                else if(type.width == 16)
                {
                    switch(type.basetype)
                    {
                    case SPIRType::Int:
                        return VK_FORMAT_R16G16_SINT;
                    case SPIRType::UInt:
                        return VK_FORMAT_R16G16_UINT;
                    case SPIRType::Float:
                        return VK_FORMAT_R16G16_SFLOAT;
                    default:
                        return VK_FORMAT_UNDEFINED;
                    }
                }
                else if(type.width == 32)
                {
                    switch(type.basetype)
                    {
                    case SPIRType::Int:
                        return VK_FORMAT_R32G32_SINT;
                    case SPIRType::UInt:
                        return VK_FORMAT_R32G32_UINT;
                    case SPIRType::Float:
                        return VK_FORMAT_R32G32_SFLOAT;
                    default:
                        return VK_FORMAT_UNDEFINED;
                    }
                }
                else if(type.width == 64)
                {
                    switch(type.basetype)
                    {
                    case SPIRType::Int64:
                        return VK_FORMAT_R64G64_SINT;
                    case SPIRType::UInt64:
                        return VK_FORMAT_R64G64_UINT;
                    case SPIRType::Double:
                        return VK_FORMAT_R64G64_SFLOAT;
                    default:
                        return VK_FORMAT_UNDEFINED;
                    }
                }
                else
                {
                    LWARN("Invalid type width for conversion of SPIR-Type to VkFormat enum value!");
                    return VK_FORMAT_UNDEFINED;
                }
            }
            else if(type.vecsize == 3)
            {
                if(type.width == 8)
                {
                    switch(type.basetype)
                    {
                    case SPIRType::Int:
                        return VK_FORMAT_R8G8B8_SINT;
                    case SPIRType::UInt:
                        return VK_FORMAT_R8G8B8_UINT;
                    default:
                        return VK_FORMAT_UNDEFINED;
                    }
                }
                else if(type.width == 16)
                {
                    switch(type.basetype)
                    {
                    case SPIRType::Int:
                        return VK_FORMAT_R16G16B16_SINT;
                    case SPIRType::UInt:
                        return VK_FORMAT_R16G16B16_UINT;
                    case SPIRType::Float:
                        return VK_FORMAT_R16G16B16_SFLOAT;
                    default:
                        return VK_FORMAT_UNDEFINED;
                    }
                }
                else if(type.width == 32)
                {
                    switch(type.basetype)
                    {
                    case SPIRType::Int:
                        return VK_FORMAT_R32G32B32_SINT;
                    case SPIRType::UInt:
                        return VK_FORMAT_R32G32B32_UINT;
                    case SPIRType::Float:
                        return VK_FORMAT_R32G32B32_SFLOAT;
                    default:
                        return VK_FORMAT_UNDEFINED;
                    }
                }
                else if(type.width == 64)
                {
                    switch(type.basetype)
                    {
                    case SPIRType::Int64:
                        return VK_FORMAT_R64G64B64_SINT;
                    case SPIRType::UInt64:
                        return VK_FORMAT_R64G64B64_UINT;
                    case SPIRType::Double:
                        return VK_FORMAT_R64G64B64_SFLOAT;
                    default:
                        return VK_FORMAT_UNDEFINED;
                    }
                }
                else
                {
                    LWARN("Invalid type width for conversion of SPIR-Type to VkFormat enum value!");
                    return VK_FORMAT_UNDEFINED;
                }
            }
            else if(type.vecsize == 4)
            {
                if(type.width == 8)
                {
                    switch(type.basetype)
                    {
                    case SPIRType::Int:
                        return VK_FORMAT_R8G8B8A8_SINT;
                    case SPIRType::UInt:
                        return VK_FORMAT_R8G8B8A8_UINT;
                    default:
                        return VK_FORMAT_UNDEFINED;
                    }
                }
                else if(type.width == 16)
                {
                    switch(type.basetype)
                    {
                    case SPIRType::Int:
                        return VK_FORMAT_R16G16B16A16_SINT;
                    case SPIRType::UInt:
                        return VK_FORMAT_R16G16B16A16_UINT;
                    case SPIRType::Float:
                        return VK_FORMAT_R16G16B16A16_SFLOAT;
                    default:
                        return VK_FORMAT_UNDEFINED;
                    }
                }
                else if(type.width == 32)
                {
                    switch(type.basetype)
                    {
                    case SPIRType::Int:
                        return VK_FORMAT_R32G32B32A32_SINT;
                    case SPIRType::UInt:
                        return VK_FORMAT_R32G32B32A32_UINT;
                    case SPIRType::Float:
                        return VK_FORMAT_R32G32B32A32_SFLOAT;
                    default:
                        return VK_FORMAT_UNDEFINED;
                    }
                }
                else if(type.width == 64)
                {
                    switch(type.basetype)
                    {
                    case SPIRType::Int64:
                        return VK_FORMAT_R64G64B64A64_SINT;
                    case SPIRType::UInt64:
                        return VK_FORMAT_R64G64B64A64_UINT;
                    case SPIRType::Double:
                        return VK_FORMAT_R64G64B64A64_SFLOAT;
                    default:
                        return VK_FORMAT_UNDEFINED;
                    }
                }
                else
                {
                    LWARN("Invalid type width for conversion to a VkFormat enum");
                    return VK_FORMAT_UNDEFINED;
                }
            }
            else
            {
                LWARN("Vector size in vertex input attributes isn't explicitly supported for parsing from SPIRType->VkFormat");
                return VK_FORMAT_UNDEFINED;
            }
        }

        uint32_t GetStrideFromVulkanFormat(VkFormat format)
        {
            switch(format)
            {
            case VK_FORMAT_R8_SINT:
                return sizeof(int);
            case VK_FORMAT_R32_SFLOAT:
                return sizeof(float);
            case VK_FORMAT_R32G32_SFLOAT:
                return sizeof(Vec2);
            case VK_FORMAT_R32G32B32_SFLOAT:
                return sizeof(Vec3);
            case VK_FORMAT_R32G32B32A32_SFLOAT:
                return sizeof(Vec4);
            case VK_FORMAT_R32G32B32A32_SINT:
                return sizeof(IVec4);
            case VK_FORMAT_R32G32_SINT:
            case VK_FORMAT_R32G32B32_SINT:
            case VK_FORMAT_R32G32_UINT:
            case VK_FORMAT_R32G32B32_UINT:
            case VK_FORMAT_R32G32B32A32_UINT:
            default:
                LERROR("Unsupported Format %i", (int)format);
                return 0;
            }

            return 0;
        }

        VKShader::VKShader(const char* filePath)
            : m_StageCount(0)
            , m_PipelineLayout(VK_NULL_HANDLE)
            , m_ShaderStages(nullptr)
        {
            m_Name     = StringUtilities::GetFileName(std::string(filePath)).c_str();
            m_FilePath = StringUtilities::GetFileLocation(std::string(filePath)).c_str();
			Arena* arena = ArenaAlloc(Kilobytes(256));
            m_Source   = (const char*)FileSystem::Get().ReadTextFile(arena, Str8C((char*)filePath)).str;

            if(m_Source.empty())
            {
                m_Compiled = false;
                LERROR("Failed to load shader %s", filePath);
                return;
            }
            Init();
			ArenaRelease(arena);
        }

        VKShader::VKShader(const uint32_t* vertData, uint32_t vertDataSize, const uint32_t* fragData, uint32_t fragDataSize)
            : m_StageCount(0)
            , m_PipelineLayout(VK_NULL_HANDLE)
            , m_ShaderStages(nullptr)
        {
            m_Name         = "";
            m_FilePath     = "Embedded";
            m_ShaderTypes  = { ShaderType::VERTEX, ShaderType::FRAGMENT };
            m_StageCount   = 2;
            m_ShaderStages = new VkPipelineShaderStageCreateInfo[m_StageCount];

            for(uint32_t i = 0; i < m_StageCount; i++)
                m_ShaderStages[i] = VkPipelineShaderStageCreateInfo();

            LoadFromData(vertData, vertDataSize, ShaderType::VERTEX, 0);
            LoadFromData(fragData, fragDataSize, ShaderType::FRAGMENT, 1);

            HashCombine(m_Hash, m_Name, vertData, vertData, fragData, fragDataSize);

            CreatePipelineLayout();
        }

        VKShader::VKShader(const uint32_t* compData, uint32_t compDataSize)
            : m_StageCount(0)
            , m_PipelineLayout(VK_NULL_HANDLE)
            , m_ShaderStages(nullptr)
        {
            m_Name         = "";
            m_FilePath     = "Embedded";
            m_ShaderTypes  = { ShaderType::COMPUTE };
            m_StageCount   = 1;
            m_ShaderStages = new VkPipelineShaderStageCreateInfo[m_StageCount];

            for(uint32_t i = 0; i < m_StageCount; i++)
                m_ShaderStages[i] = VkPipelineShaderStageCreateInfo();

            LoadFromData(compData, compDataSize, ShaderType::COMPUTE, 0);

            CreatePipelineLayout();
        }

        VKShader::~VKShader()
        {
            Unload();
        }

        bool VKShader::Init()
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            m_StageCount = 0;

            std::map<ShaderType, std::string> files;
            PreProcess(m_Source, &files);

            for(auto& source : files)
            {
                m_ShaderTypes.PushBack(source.first);
                m_StageCount++;
            }

            m_ShaderStages = new VkPipelineShaderStageCreateInfo[m_StageCount];

            for(uint32_t i = 0; i < m_StageCount; i++)
                m_ShaderStages[i] = VkPipelineShaderStageCreateInfo();

            LINFO("Loading Shader : %s", m_Name.c_str());

            uint32_t currentShaderStage = 0;
            HashCombine(m_Hash, m_Name);

            for(auto& file : files)
            {

				Arena* arena = ArenaAlloc(Kilobytes(256));
				String8 debugName = PushStr8F(arena, "%s%s", m_FilePath.c_str(), file.second.c_str());

                HashCombine(m_Hash, debugName.str);

                uint32_t fileSize = uint32_t(FileSystem::GetFileSize(debugName));
                uint32_t* source  = reinterpret_cast<uint32_t*>(FileSystem::ReadFile(arena, debugName));

                if(source)
                {
                    LoadFromData(source, fileSize, file.first, currentShaderStage);

                    currentShaderStage++;
                }

				ArenaRelease(arena);
            }

            if(files.empty())
                LERROR("Failed to load shader %s", m_Name.c_str());

            CreatePipelineLayout();

            return true;
        }

        void VKShader::CreatePipelineLayout()
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            TDArray<TDArray<Graphics::DescriptorLayoutInfo>> layouts;

            for(auto& descriptorLayout : GetDescriptorLayout())
            {
                while((uint32_t)layouts.Size() < descriptorLayout.setID + 1)
                {
                    layouts.EmplaceBack();
                }

                layouts[descriptorLayout.setID].PushBack(descriptorLayout);
            }

            for(auto& l : layouts)
            {
                TDArray<VkDescriptorSetLayoutBinding> setLayoutBindings;
                TDArray<VkDescriptorBindingFlags> layoutBindingFlags;
                setLayoutBindings.Reserve(l.Size());
                layoutBindingFlags.Reserve(l.Size());

                for(uint32_t i = 0; i < (uint32_t)l.Size(); i++)
                {
                    auto& info = l[i];

                    int foundIndex = -1;
                    for(uint32_t i = 0; i < (uint32_t)setLayoutBindings.Size(); i++)
                    {
                        if(setLayoutBindings[i].binding == info.binding)
                            foundIndex = i;
                    }

                    VkDescriptorSetLayoutBinding& setLayoutBinding = foundIndex >= 0 ? setLayoutBindings[foundIndex] : setLayoutBindings.EmplaceBack();

                    setLayoutBinding.descriptorType = VKUtilities::DescriptorTypeToVK(info.type);
                    if(foundIndex >= 0)
                    {
                        setLayoutBinding.stageFlags |= VKUtilities::ShaderTypeToVK(info.stage);
                    }
                    else
                        setLayoutBinding.stageFlags = VKUtilities::ShaderTypeToVK(info.stage);

                    setLayoutBinding.binding         = info.binding;
                    setLayoutBinding.descriptorCount = info.count;

                    bool isArray = info.count > 1;

                    if(foundIndex < 0)
                        layoutBindingFlags.PushBack(isArray ? VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT : 0);
                }

                VkDescriptorSetLayoutBindingFlagsCreateInfo flagsInfo = {};
                flagsInfo.sType                                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
                flagsInfo.pNext                                       = nullptr;
                flagsInfo.bindingCount                                = static_cast<uint32_t>(layoutBindingFlags.Size());
                flagsInfo.pBindingFlags                               = layoutBindingFlags.Data();

                // Pipeline layout
                VkDescriptorSetLayoutCreateInfo setLayoutCreateInfo = {};
                setLayoutCreateInfo.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                setLayoutCreateInfo.bindingCount                    = static_cast<uint32_t>(setLayoutBindings.Size());
                setLayoutCreateInfo.pBindings                       = setLayoutBindings.Data();
                setLayoutCreateInfo.pNext                           = &flagsInfo;

                VkDescriptorSetLayout layout;
                vkCreateDescriptorSetLayout(VKDevice::Get().GetDevice(), &setLayoutCreateInfo, VK_NULL_HANDLE, &layout);

                m_DescriptorSetLayouts.PushBack(layout);
            }

            const auto& pushConsts = GetPushConstants();
            TDArray<VkPushConstantRange> pushConstantRanges;

            for(auto& pushConst : pushConsts)
            {
                pushConstantRanges.PushBack(VKInitialisers::PushConstantRange(VKUtilities::ShaderTypeToVK(pushConst.shaderStage), pushConst.size, pushConst.offset));
            }

            auto& descriptorSetLayouts = GetDescriptorLayouts();

            VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
            pipelineLayoutCreateInfo.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutCreateInfo.setLayoutCount             = static_cast<uint32_t>(descriptorSetLayouts.Size());
            pipelineLayoutCreateInfo.pSetLayouts                = descriptorSetLayouts.Data();
            pipelineLayoutCreateInfo.pushConstantRangeCount     = uint32_t(pushConstantRanges.Size());
            pipelineLayoutCreateInfo.pPushConstantRanges        = pushConstantRanges.Data();

            VK_CHECK_RESULT(vkCreatePipelineLayout(VKDevice::Get().GetDevice(), &pipelineLayoutCreateInfo, VK_NULL_HANDLE, &m_PipelineLayout));
        }

        static const char* ShaderStageToString(const VkShaderStageFlagBits stage)
        {
            switch(stage)
            {
            case VK_SHADER_STAGE_VERTEX_BIT:
                return "vert";
            case VK_SHADER_STAGE_FRAGMENT_BIT:
                return "frag";
            case VK_SHADER_STAGE_COMPUTE_BIT:
                return "comp";
            }
            ASSERT(false);
            return "UNKNOWN";
        }

        void VKShader::LoadFromData(const uint32_t* source, uint32_t fileSize, ShaderType shaderType, int currentShaderStage)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            VkShaderModuleCreateInfo shaderCreateInfo = {};
            shaderCreateInfo.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            shaderCreateInfo.codeSize                 = fileSize;
            shaderCreateInfo.pCode                    = source;
            shaderCreateInfo.pNext                    = VK_NULL_HANDLE;

            spirv_cross::Compiler comp(source, size_t(fileSize / sizeof(uint32_t)));
            // The SPIR-V is now parsed, and we can perform reflection on it.
            spirv_cross::ShaderResources resources = comp.get_shader_resources();

            if(shaderType == ShaderType::VERTEX)
            {
                // Vertex Layout
                m_VertexInputStride = 0;

                for(const spirv_cross::Resource& resource : resources.stage_inputs)
                {
                    const spirv_cross::SPIRType& InputType = comp.get_type(resource.type_id);

                    VkVertexInputAttributeDescription Description = {};
                    Description.binding                           = comp.get_decoration(resource.id, spv::DecorationBinding);
                    Description.location                          = comp.get_decoration(resource.id, spv::DecorationLocation);
                    Description.offset                            = m_VertexInputStride;
                    Description.format                            = GetVulkanFormat(InputType);
                    m_VertexInputAttributeDescriptions.PushBack(Description);

                    m_VertexInputStride += GetStrideFromVulkanFormat(Description.format);
                }
            }

            // Descriptor Layout
            for(auto& u : resources.uniform_buffers)
            {
                uint32_t set     = comp.get_decoration(u.id, spv::DecorationDescriptorSet);
                uint32_t binding = comp.get_decoration(u.id, spv::DecorationBinding);
                auto& type       = comp.get_type(u.type_id);

                SHADER_LOG(LINFO("Found UBO %s at set = %i, binding = %i", u.name.c_str(), set, binding));
                m_DescriptorLayoutInfo.PushBack({ Graphics::DescriptorType::UNIFORM_BUFFER, shaderType, binding, set, type.array.size() ? uint32_t(type.array[0]) : 1 });

                auto& bufferType      = comp.get_type(u.base_type_id);
                auto bufferSize       = comp.get_declared_struct_size(bufferType);
                int memberCount       = (int)bufferType.member_types.size();
                auto& descriptorInfo  = m_DescriptorInfos[set];
                auto& descriptor      = descriptorInfo.descriptors.EmplaceBack();
                descriptor.binding    = binding;
                descriptor.size       = (uint32_t)bufferSize;
                descriptor.name       = u.name;
                descriptor.offset     = 0;
                descriptor.shaderType = shaderType;
                descriptor.type       = Graphics::DescriptorType::UNIFORM_BUFFER;
                descriptor.buffer     = nullptr;

                for(int i = 0; i < memberCount; i++)
                {
                    auto type              = comp.get_type(bufferType.member_types[i]);
                    const auto& memberName = comp.get_member_name(bufferType.self, i);
                    auto size              = comp.get_declared_struct_member_size(bufferType, i);
                    auto offset            = comp.type_struct_member_offset(bufferType, i);

                    std::string uniformName = u.name + "." + memberName;

                    auto& member  = descriptor.m_Members.EmplaceBack();
                    member.name   = memberName;
                    member.offset = offset;
                    member.size   = (uint32_t)size;

                    SHADER_LOG(LINFO("%s - Size %i, offset %i", uniformName.c_str(), size, offset));
                }
            }

            for(auto& u : resources.push_constant_buffers)
            {
                uint32_t set     = comp.get_decoration(u.id, spv::DecorationDescriptorSet);
                uint32_t binding = comp.get_decoration(u.id, spv::DecorationBinding);

                uint32_t binding3 = comp.get_decoration(u.id, spv::DecorationOffset);

                auto& type = comp.get_type(u.type_id);

                auto ranges = comp.get_active_buffer_ranges(u.id);

                uint32_t size = 0;
                for(auto& range : ranges)
                {
                    SHADER_LOG(LINFO("Accessing Member %i offset %i, size %i", range.index, range.offset, range.range));
                    size += uint32_t(range.range);
                }

                SHADER_LOG(LINFO("Found Push Constant %s at set = %i, binding = %i", u.name.c_str(), set, binding, type.array.size() ? uint32_t(type.array[0]) : 1));

                m_PushConstants.PushBack({ size, shaderType });
                m_PushConstants.Back().data = new uint8_t[size];

                auto& bufferType = comp.get_type(u.base_type_id);
                auto bufferSize  = comp.get_declared_struct_size(bufferType);
                int memberCount  = (int)bufferType.member_types.size();

                for(int i = 0; i < memberCount; i++)
                {
                    auto type               = comp.get_type(bufferType.member_types[i]);
                    const auto& memberName  = comp.get_member_name(bufferType.self, i);
                    auto size               = comp.get_declared_struct_member_size(bufferType, i);
                    auto offset             = comp.type_struct_member_offset(bufferType, i);
                    std::string uniformName = u.name + "." + memberName;
                    auto& member            = m_PushConstants.Back().m_Members.EmplaceBack();
                    member.size             = (uint32_t)size;
                    member.offset           = offset;
                    member.type             = SPIRVTypeToLumosDataType(type);
                    member.fullName         = uniformName;
                    member.name             = memberName;
                }
            }

            for(auto& u : resources.sampled_images)
            {
                uint32_t set         = comp.get_decoration(u.id, spv::DecorationDescriptorSet);
                uint32_t binding     = comp.get_decoration(u.id, spv::DecorationBinding);
                auto& descriptorInfo = m_DescriptorInfos[set];
                auto& descriptor     = descriptorInfo.descriptors.EmplaceBack();

                auto& type = comp.get_type(u.type_id);
                SHADER_LOG(LINFO("Found Sampled Image %s at set = %i, binding = %i", u.name.c_str(), set, binding));

                m_DescriptorLayoutInfo.PushBack({ Graphics::DescriptorType::IMAGE_SAMPLER, shaderType, binding, set, type.array.size() ? uint32_t(type.array[0]) : 1 });

                descriptor.binding      = binding;
                descriptor.textureCount = 1;
                descriptor.name         = u.name;
                descriptor.texture      = Graphics::Material::GetDefaultTexture().get(); // TODO: Move
            }

            for(auto& u : resources.storage_images)
            {
                const auto& name       = u.name;
                auto& type             = comp.get_type(u.type_id);
                uint32_t binding       = comp.get_decoration(u.id, spv::DecorationBinding);
                uint32_t descriptorSet = comp.get_decoration(u.id, spv::DecorationDescriptorSet);
                uint32_t dimension     = type.image.dim;
                uint32_t arraySize     = type.array[0];
                if(arraySize == 0)
                    arraySize = 1;

                auto& descriptorInfo = m_DescriptorInfos[descriptorSet];
                auto& descriptor     = descriptorInfo.descriptors.EmplaceBack();

                SHADER_LOG(LINFO("Found Storage Image %s at set = %i, binding = %i", u.name.c_str(), descriptorSet, binding));

                m_DescriptorLayoutInfo.PushBack({ Graphics::DescriptorType::IMAGE_STORAGE, shaderType, binding, descriptorSet, type.array.size() ? uint32_t(type.array[0]) : 1 });

                descriptor.type         = Graphics::DescriptorType::IMAGE_STORAGE;
                descriptor.binding      = binding;
                descriptor.textureCount = 1;
                descriptor.name         = u.name;
                descriptor.texture      = Graphics::Material::GetDefaultTexture().get(); // TODO: Move
            }

            m_ShaderStages[currentShaderStage].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            m_ShaderStages[currentShaderStage].stage = VKUtilities::ShaderTypeToVK(shaderType);
            m_ShaderStages[currentShaderStage].pName = "main";
            m_ShaderStages[currentShaderStage].pNext = VK_NULL_HANDLE;

            VkResult result = vkCreateShaderModule(VKDevice::Get().GetDevice(), &shaderCreateInfo, nullptr, &m_ShaderStages[currentShaderStage].module);

			Arena* arena = ArenaAlloc(260);
            String8 debugName = PushStr8F(arena, "%s%s", m_Name.c_str(), ShaderStageToString(m_ShaderStages[currentShaderStage].stage));
            VKUtilities::SetDebugUtilsObjectName(VKDevice::Get().GetDevice(), VK_OBJECT_TYPE_SHADER_MODULE, (const char*)debugName.str, m_ShaderStages[currentShaderStage].module);
			ArenaRelease(arena);

            if(result == VK_SUCCESS)
            {
                m_Compiled = true;
            }

            for(auto& infos : m_DescriptorInfos)
            {
                Algorithms::BubbleSort(infos.second.descriptors.begin(), infos.second.descriptors.end(),
                                       [](Descriptor& a, Descriptor& b)
                                       {
                                           return a.binding < b.binding;
                                       });

                /*      infos.second.descriptors.Unique([](Descriptor& a, Descriptor& b)
                          {
                              return a.binding == b.binding;
                          });*/
            }

            VK_CHECK_RESULT(result);
        }

        void VKShader::Unload()
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            for(uint32_t i = 0; i < m_StageCount; i++)
                vkDestroyShaderModule(VKDevice::Get().GetDevice(), m_ShaderStages[i].module, nullptr);

            for(auto& descriptorLayout : m_DescriptorSetLayouts)
                vkDestroyDescriptorSetLayout(VKDevice::Get().GetDevice(), descriptorLayout, VK_NULL_HANDLE);

            if(m_PipelineLayout)
                vkDestroyPipelineLayout(VKDevice::Get().GetDevice(), m_PipelineLayout, VK_NULL_HANDLE);

            delete[] m_ShaderStages;

            for(auto& pc : m_PushConstants)
                delete[] pc.data;

            m_StageCount = 0;
        }

        void VKShader::BindPushConstants(Graphics::CommandBuffer* commandBuffer, Graphics::Pipeline* pipeline)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            uint32_t index = 0;
            for(auto& pc : m_PushConstants)
            {
                vkCmdPushConstants(static_cast<Graphics::VKCommandBuffer*>(commandBuffer)->GetHandle(), static_cast<Graphics::VKPipeline*>(pipeline)->GetPipelineLayout(), VKUtilities::ShaderTypeToVK(pc.shaderStage), index, pc.size, pc.data);
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
            type                       = ShaderType::UNKNOWN;
            TDArray<std::string> lines = StringUtilities::GetLines(source);
            ReadShaderFile(lines, sources);
        }

        void VKShader::ReadShaderFile(const TDArray<std::string>& lines, std::map<ShaderType, std::string>* shaders)
        {
            LUMOS_PROFILE_FUNCTION_LOW();
            for(uint32_t i = 0; i < lines.Size(); i++)
            {
                std::string str = std::string(lines[i]);
                str             = StringUtilities::StringReplace(str, '\t');

                if(StringUtilities::StartsWith(str, "#shader"))
                {
                    if(StringUtilities::StringContains(str, "vertex"))
                    {
                        type                                           = ShaderType::VERTEX;
                        std::map<ShaderType, std::string>::iterator it = shaders->begin();
                        shaders->insert(it, std::pair<ShaderType, const char*>(type, ""));
                    }
                    else if(StringUtilities::StringContains(str, "geometry"))
                    {
                        type                                           = ShaderType::GEOMETRY;
                        std::map<ShaderType,std::string>::iterator it = shaders->begin();
                        shaders->insert(it, std::pair<ShaderType, const char*>(type, ""));
                    }
                    else if(StringUtilities::StringContains(str, "fragment"))
                    {
                        type                                           = ShaderType::FRAGMENT;
                        std::map<ShaderType, std::string>::iterator it = shaders->begin();
                        shaders->insert(it, std::pair<ShaderType, const char*>(type, ""));
                    }
                    else if(StringUtilities::StringContains(str, "tess_cont"))
                    {
                        type                                           = ShaderType::TESSELLATION_CONTROL;
                        std::map<ShaderType, std::string>::iterator it = shaders->begin();
                        shaders->insert(it, std::pair<ShaderType, const char*>(type, ""));
                    }
                    else if(StringUtilities::StringContains(str, "tess_eval"))
                    {
                        type                                           = ShaderType::TESSELLATION_EVALUATION;
                        std::map<ShaderType, std::string>::iterator it = shaders->begin();
                        shaders->insert(it, std::pair<ShaderType, const char*>(type, ""));
                    }
                    else if(StringUtilities::StringContains(str, "compute"))
                    {
                        type                                           = ShaderType::COMPUTE;
                        std::map<ShaderType, std::string>::iterator it = shaders->begin();
                        shaders->insert(it, std::pair<ShaderType, const char*>(type, ""));
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
            CreateFunc                 = CreateFuncVulkan;
            CreateFuncFromEmbedded     = CreateFromEmbeddedFuncVulkan;
            CreateCompFuncFromEmbedded = CreateCompFromEmbeddedFuncVulkan;
        }

        Shader* VKShader::CreateFuncVulkan(const char* filepath)
        {
            return new VKShader(filepath);
        }

        Shader* VKShader::CreateFromEmbeddedFuncVulkan(const uint32_t* vertData, uint32_t vertDataSize, const uint32_t* fragData, uint32_t fragDataSize)
        {
            return new VKShader(vertData, vertDataSize, fragData, fragDataSize);
        }

        Shader* VKShader::CreateCompFromEmbeddedFuncVulkan(const uint32_t* compData, uint32_t compDataSize)
        {
            return new VKShader(compData, compDataSize);
        }
    }
}

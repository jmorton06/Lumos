#include "Precompiled.h"
#include "GLShader.h"

#include "Platform/OpenGL/GL.h"
#include "Core/VFS.h"
#include "Core/OS/FileSystem.h"
#include "Core/StringUtilities.h"

enum root_signature_spaces
{
    PUSH_CONSTANT_REGISTER_SPACE = 0,
    DYNAMIC_OFFSET_SPACE,
    DESCRIPTOR_TABLE_INITIAL_SPACE,
};

namespace Lumos
{
    namespace Graphics
    {
        bool IGNORE_LINES = false;
        static ShaderType s_Type = ShaderType::UNKNOWN;

        uint32_t GetStrideFromOpenGLFormat(uint32_t format)
        {
            switch(format)
            {
                //                case VK_FORMAT_R8_SINT:
                //                return sizeof(int);
                //                case VK_FORMAT_R32_SFLOAT:
                //                return sizeof(float);
                //                case VK_FORMAT_R32G32_SFLOAT:
                //                return sizeof(Maths::Vector2);
                //                case VK_FORMAT_R32G32B32_SFLOAT:
                //                return sizeof(Maths::Vector3);
                //                case VK_FORMAT_R32G32B32A32_SFLOAT:
                //                return sizeof(Maths::Vector4);
            default:
                //LUMOS_LOG_ERROR("Unsupported Format {0}", format);
                return 0;
            }

            return 0;
        }

        void PushTypeToBuffer(const spirv_cross::SPIRType type, Graphics::BufferLayout& layout)
        {
            switch(type.basetype)
            {
            case spirv_cross::SPIRType::Float:
                switch(type.vecsize)
                {
                case 1:
                    layout.Push<float>("");
                    break;
                case 2:
                    layout.Push<Maths::Vector2>("");
                    break;
                case 3:
                    layout.Push<Maths::Vector3>("");
                    break;
                case 4:
                    layout.Push<Maths::Vector4>("");
                    break;
                }
            case spirv_cross::SPIRType::Double:
                break;
            default:
                break;
            }
        }

        GLShader::GLShader(const std::string& filePath)
        {
            m_Name = StringUtilities::GetFileName(filePath);
            m_Path = StringUtilities::GetFileLocation(filePath);

            m_Source = VFS::Get()->ReadTextFile(filePath);

            Init();
        }

        GLShader::~GLShader()
        {
            Shutdown();

            for(auto& pc : m_PushConstants)
                delete[] pc.data;
        }

        void GLShader::Init()
        {
            LUMOS_PROFILE_FUNCTION();
            std::map<ShaderType, std::string>* sources = new std::map<ShaderType, std::string>();
            PreProcess(m_Source, sources);

            for(auto& file : *sources)
            {
                auto fileSize = FileSystem::GetFileSize(m_Path + file.second); //TODO: once process
                uint32_t* source = reinterpret_cast<uint32_t*>(FileSystem::ReadFile(m_Path + file.second));
                std::vector<unsigned int> spv(source, source + fileSize / sizeof(unsigned int));

                spirv_cross::CompilerGLSL* glsl = new spirv_cross::CompilerGLSL(std::move(spv));

                // The SPIR-V is now parsed, and we can perform reflection on it.
                spirv_cross::ShaderResources resources = glsl->get_shader_resources();

                if(file.first == ShaderType::VERTEX)
                {
                    uint32_t stride = 0;
                    for(const spirv_cross::Resource& resource : resources.stage_inputs)
                    {
                        const spirv_cross::SPIRType& InputType = glsl->get_type(resource.type_id);
                        //Switch to GL layout
                        PushTypeToBuffer(InputType, m_Layout);
                        stride += GetStrideFromOpenGLFormat(0); //InputType.width * InputType.vecsize / 8;
                    }
                }

                // Get all sampled images in the shader.
                for(auto& resource : resources.sampled_images)
                {
                    uint32_t set = glsl->get_decoration(resource.id, spv::DecorationDescriptorSet);
                    uint32_t binding = glsl->get_decoration(resource.id, spv::DecorationBinding);

                    // Modify the decoration to prepare it for GLSL.
                    glsl->unset_decoration(resource.id, spv::DecorationDescriptorSet);

                    // Some arbitrary remapping if we want.
                    glsl->set_decoration(resource.id, spv::DecorationBinding, set * 16 + binding);

                    auto& descriptorInfo = m_DescriptorInfos[set];
                    auto& descriptor = descriptorInfo.descriptors.emplace_back();
                    descriptor.binding = binding;
                    descriptor.name = resource.name;
                    descriptor.shaderType = file.first;
                    descriptor.type = Graphics::DescriptorType::IMAGE_SAMPLER;
                }

//                for(auto const& image : resources.separate_images)
//                {
//                    auto set { glsl->get_decoration(image.id, spv::Decoration::DecorationDescriptorSet) };
//                    glsl->set_decoration(image.id, spv::Decoration::DecorationDescriptorSet, DESCRIPTOR_TABLE_INITIAL_SPACE + 2 * set);
//                }
//                for(auto const& input : resources.subpass_inputs)
//                {
//                    auto set { glsl->get_decoration(input.id, spv::Decoration::DecorationDescriptorSet) };
//                    glsl->set_decoration(input.id, spv::Decoration::DecorationDescriptorSet, DESCRIPTOR_TABLE_INITIAL_SPACE + 2 * set);
//                }
                for(auto const& uniform_buffer : resources.uniform_buffers)
                {
                    auto set { glsl->get_decoration(uniform_buffer.id, spv::Decoration::DecorationDescriptorSet) };
                    glsl->set_decoration(uniform_buffer.id, spv::Decoration::DecorationDescriptorSet, DESCRIPTOR_TABLE_INITIAL_SPACE + 2 * set);

                    uint32_t binding = glsl->get_decoration(uniform_buffer.id, spv::DecorationBinding);
                    auto& bufferType = glsl->get_type(uniform_buffer.type_id);

                    auto bufferSize = glsl->get_declared_struct_size(bufferType);
                    int memberCount = (int)bufferType.member_types.size();

                    auto& descriptorInfo = m_DescriptorInfos[set];
                    auto& descriptor = descriptorInfo.descriptors.emplace_back();
                    descriptor.binding = binding;
                    descriptor.size = (uint32_t)bufferSize;
                    descriptor.name = uniform_buffer.name;
                    descriptor.offset = 0;
                    descriptor.shaderType = file.first;
                    descriptor.type = Graphics::DescriptorType::UNIFORM_BUFFER;
                    descriptor.buffer = nullptr;

                    for(int i = 0; i < memberCount; i++)
                    {
                        auto type = glsl->get_type(bufferType.member_types[i]);
                        const auto& memberName = glsl->get_member_name(bufferType.self, i);
                        auto size = glsl->get_declared_struct_member_size(bufferType, i);
                        auto offset = glsl->type_struct_member_offset(bufferType, i);

                        std::string uniformName = uniform_buffer.name + "." + memberName;

                        auto& member = descriptor.m_Members.emplace_back();
                        member.size = (uint32_t)size;
                        member.offset = offset;
                        member.type = SPIRVTypeToLumosDataType(type);
                        member.fullName = uniformName;
                        member.name = memberName;
                    }
                }
//                for(auto const& storage_buffer : resources.storage_buffers)
//                {
//                    auto set { glsl->get_decoration(storage_buffer.id, spv::Decoration::DecorationDescriptorSet) };
//                    glsl->set_decoration(storage_buffer.id, spv::Decoration::DecorationDescriptorSet, DESCRIPTOR_TABLE_INITIAL_SPACE + 2 * set);
//                }
//                for(auto const& storage_image : resources.storage_images)
//                {
//                    auto set { glsl->get_decoration(storage_image.id, spv::Decoration::DecorationDescriptorSet) };
//                    glsl->set_decoration(storage_image.id, spv::Decoration::DecorationDescriptorSet, DESCRIPTOR_TABLE_INITIAL_SPACE + 2 * set);
//                }
//
//                for(auto const& sampler : resources.separate_samplers)
//                {
//                    auto set { glsl->get_decoration(sampler.id, spv::Decoration::DecorationDescriptorSet) };
//                    glsl->set_decoration(sampler.id, spv::Decoration::DecorationDescriptorSet, DESCRIPTOR_TABLE_INITIAL_SPACE + 2 * set + 1);
//                }

                for(auto& u : resources.push_constant_buffers)
                {
                   // uint32_t set = glsl->get_decoration(u.id, spv::DecorationDescriptorSet);
                   // uint32_t binding = glsl->get_decoration(u.id, spv::DecorationBinding);

                    auto& type = glsl->get_type(u.type_id);
                    auto name = glsl->get_name(u.id);

                    auto ranges = glsl->get_active_buffer_ranges(u.id);

                    uint32_t size = 0;
                    for(auto& range : ranges)
                    {
                        size += uint32_t(range.range);
                    }

                    auto& bufferType = glsl->get_type(u.base_type_id);
                    auto bufferSize = glsl->get_declared_struct_size(bufferType);
                    int memberCount = (int)bufferType.member_types.size();

                    m_PushConstants.push_back({ size, file.first });
                    m_PushConstants.back().data = new uint8_t[size];

                    for(int i = 0; i < memberCount; i++)
                    {
                        auto type = glsl->get_type(bufferType.member_types[i]);
                        const auto& memberName = glsl->get_member_name(bufferType.self, i);
                        auto size = glsl->get_declared_struct_member_size(bufferType, i);
                        auto offset = glsl->type_struct_member_offset(bufferType, i);

                        std::string uniformName = u.name + "." + memberName;

                        auto& member = m_PushConstants.back().m_Members.emplace_back();
                        member.size = (uint32_t)size;
                        member.offset = offset;
                        member.type = SPIRVTypeToLumosDataType(type);
                        member.fullName = uniformName;
                        member.name = memberName;
                    }
                }

                spirv_cross::CompilerGLSL::Options options;
                options.version = 410;
                options.es = false;
                options.vulkan_semantics = false;
                options.separate_shader_objects = false;
                options.enable_420pack_extension = false;
                options.emit_push_constant_as_uniform_buffer = false;
                glsl->set_common_options(options);

                // Compile to GLSL, ready to give to GL driver.
                std::string glslSource = glsl->compile();
                file.second = glslSource;

                m_ShaderCompilers.push_back(glsl);
            }

            for(auto& source : *sources)
            {
                m_ShaderTypes.push_back(source.first);
            }

            GLShaderErrorInfo error;
            m_Handle = Compile(sources, error);

            if(!m_Handle)
            {
                LUMOS_LOG_ERROR("{0} - {1}", error.message[error.shader], m_Name);
            }
            else
            {
                LUMOS_LOG_INFO("Successfully compiled shader: {0}", m_Name);
            }

            CreateLocations();

            delete sources;
        }

        void GLShader::Shutdown() const
        {
            LUMOS_PROFILE_FUNCTION();
            GLCall(glDeleteProgram(m_Handle));
        }

        GLuint HashValue(const char* szString)
        {
            LUMOS_PROFILE_FUNCTION();
            const char* c = szString;
            GLuint dwHashValue = 0x00000000;

            while(*c)
            {
                dwHashValue = (dwHashValue << 5) - dwHashValue + (*c == '/' ? '\\' : *c);
                c++;
            }

            return dwHashValue ? dwHashValue : 0xffffffff;
        }

        void GLShader::SetUniform(ShaderDataType type, uint8_t* data, uint32_t size, uint32_t offset, const std::string& name)
        {
            LUMOS_PROFILE_FUNCTION();

            GLuint hashName = HashValue(name.c_str());

            if(m_UniformLocations.find(hashName) == m_UniformLocations.end())
            {
                GLuint location = glGetUniformLocation(m_Handle, name.c_str());

                if(location != GL_INVALID_INDEX)
                {
                    m_UniformLocations[hashName] = location;
                }
                else
                {
                    LUMOS_LOG_WARN("Invalid uniform location {0}", name);
                }
            }

            auto location = m_UniformLocations[hashName];
            if(location == -1)
            {
                LUMOS_LOG_ERROR("Couldnt Find Uniform In Shader: {0}", name);
                return;
            }

            switch(type)
            {
            case ShaderDataType::FLOAT32:
                SetUniform1f(location, *reinterpret_cast<float*>(&data[offset]));
                break;
            case ShaderDataType::INT32:
                SetUniform1i(location, *reinterpret_cast<int32_t*>(&data[offset]));
                break;
            case ShaderDataType::UINT:
                SetUniform1ui(location, *reinterpret_cast<uint32_t*>(&data[offset]));
                break;
            case ShaderDataType::INT:
                SetUniform1i(location, *reinterpret_cast<int*>(&data[offset]));
                break;
            case ShaderDataType::VEC2:
                SetUniform2f(location, *reinterpret_cast<Maths::Vector2*>(&data[offset]));
                break;
            case ShaderDataType::VEC3:
                SetUniform3f(location, *reinterpret_cast<Maths::Vector3*>(&data[offset]));
                break;
            case ShaderDataType::VEC4:
                SetUniform4f(location, *reinterpret_cast<Maths::Vector4*>(&data[offset]));
                break;
            case ShaderDataType::MAT3:
                SetUniformMat3(location, *reinterpret_cast<Maths::Matrix3*>(&data[offset]));
                break;
            case ShaderDataType::MAT4:
                SetUniformMat4(location, *reinterpret_cast<Maths::Matrix4*>(&data[offset]));
                break;
            default:
                LUMOS_ASSERT(false, "Unknown type!");
            }
        }

        void GLShader::BindPushConstants(Graphics::CommandBuffer* cmdBuffer, Graphics::Pipeline* pipeline)
        {
            LUMOS_PROFILE_FUNCTION();
            int index = 0;
            for(auto pc : m_PushConstants)
            {
                for(auto& member : pc.m_Members)
                {
                    SetUniform(member.type, pc.data, member.size, member.offset, member.fullName);
                }
            }
        }

        bool GLShader::CreateLocations()
        {
            LUMOS_PROFILE_FUNCTION();
            for(auto& compiler : m_ShaderCompilers)
            {
                const spirv_cross::ShaderResources shaderResources = compiler->get_shader_resources();

                for(const auto& itUniform : shaderResources.uniform_buffers)
                {
                    if(compiler->get_type(itUniform.type_id).basetype == spirv_cross::SPIRType::Struct)
                    {
                        SetUniformLocation(itUniform.name.c_str());
                    }
                }

                for(const auto& itUniform : shaderResources.push_constant_buffers)
                {
                    if(compiler->get_type(itUniform.type_id).basetype == spirv_cross::SPIRType::Struct)
                    {
                        auto name = compiler->get_name(itUniform.base_type_id);
                        SetUniformLocation(name.c_str());
                    }
                }
            }
            return true;
        }

        void GLShader::BindUniformBuffer(GLUniformBuffer* buffer, uint32_t slot, const std::string& name)
        {
            LUMOS_PROFILE_FUNCTION();
            GLuint nameInt = HashValue(name.c_str());
            const auto& itLocation = m_UniformBlockLocations.find(nameInt);
            GLCall(glUniformBlockBinding(m_Handle, itLocation->second, slot));
        }

        bool GLShader::SetUniformLocation(const char* szName)
        {
            LUMOS_PROFILE_FUNCTION();
            GLuint name = HashValue(szName);

            if(m_UniformBlockLocations.find(name) == m_UniformBlockLocations.end())
            {
                GLuint location = glGetUniformBlockIndex(m_Handle, szName);

                if(location != GL_INVALID_INDEX)
                {
                    m_UniformBlockLocations[name] = location;
                    glUniformBlockBinding(m_Handle, location, location);
                    return true;
                }
            }

            return false;
        }

        void GLShader::PreProcess(const std::string& source, std::map<ShaderType, std::string>* sources)
        {
            LUMOS_PROFILE_FUNCTION();
            s_Type = ShaderType::UNKNOWN;
            std::vector<std::string> lines = StringUtilities::GetLines(source);
            ReadShaderFile(lines, sources);
        }

        void GLShader::ReadShaderFile(std::vector<std::string> lines, std::map<ShaderType, std::string>* shaders)
        {
            LUMOS_PROFILE_FUNCTION();
            for(uint32_t i = 0; i < lines.size(); i++)
            {
                std::string str = std::string(lines[i]);
                str = StringUtilities::StringReplace(str, '\t');

                if(IGNORE_LINES)
                {
                    if(StringUtilities::StartsWith(str, "#end"))
                    {
                        IGNORE_LINES = false;
                    }
                }
                else if(StringUtilities::StartsWith(str, "#shader"))
                {
                    if(StringUtilities::StringContains(str, "vertex"))
                    {
                        s_Type = ShaderType::VERTEX;
                        std::map<ShaderType, std::string>::iterator it = shaders->begin();
                        shaders->insert(it, std::pair<ShaderType, std::string>(s_Type, ""));
                    }
                    else if(StringUtilities::StringContains(str, "geometry"))
                    {
                        s_Type = ShaderType::GEOMETRY;
                        std::map<ShaderType, std::string>::iterator it = shaders->begin();
                        shaders->insert(it, std::pair<ShaderType, std::string>(s_Type, ""));
                    }
                    else if(StringUtilities::StringContains(str, "fragment"))
                    {
                        s_Type = ShaderType::FRAGMENT;
                        std::map<ShaderType, std::string>::iterator it = shaders->begin();
                        shaders->insert(it, std::pair<ShaderType, std::string>(s_Type, ""));
                    }
                    else if(StringUtilities::StringContains(str, "tess_cont"))
                    {
                        s_Type = ShaderType::TESSELLATION_CONTROL;
                        std::map<ShaderType, std::string>::iterator it = shaders->begin();
                        shaders->insert(it, std::pair<ShaderType, std::string>(s_Type, ""));
                    }
                    else if(StringUtilities::StringContains(str, "tess_eval"))
                    {
                        s_Type = ShaderType::TESSELLATION_EVALUATION;
                        std::map<ShaderType, std::string>::iterator it = shaders->begin();
                        shaders->insert(it, std::pair<ShaderType, std::string>(s_Type, ""));
                    }
                    else if(StringUtilities::StringContains(str, "compute"))
                    {
                        s_Type = ShaderType::COMPUTE;
                        std::map<ShaderType, std::string>::iterator it = shaders->begin();
                        shaders->insert(it, std::pair<ShaderType, std::string>(s_Type, ""));
                    }
                    else if(StringUtilities::StringContains(str, "end"))
                    {
                        s_Type = ShaderType::UNKNOWN;
                    }
                }
                else if(StringUtilities::StartsWith(str, "#include"))
                {
                    std::string rem = "#include ";
                    std::string file = std::string(str);
                    if(strstr(file.c_str(), rem.c_str()))
                    {
                        std::string::size_type j = file.find(rem);
                        if(j != std::string::npos)
                            file.erase(j, rem.length());
                        file = StringUtilities::StringReplace(file, '\"');
                        LUMOS_LOG_WARN("Including file \'{0}\' into shader.", file);
                        VFS::Get()->ReadTextFile(file);
                        ReadShaderFile(StringUtilities::GetLines(VFS::Get()->ReadTextFile(file)), shaders);
                    }
                }
                else if(StringUtilities::StartsWith(str, "#if"))
                {
                    std::string rem = "#if ";
                    std::string def = std::string(str);
                    if(strstr(def.c_str(), rem.c_str()))
                    {
                        std::string::size_type j = def.find(rem);
                        if(j != std::string::npos)
                            def.erase(j, rem.length());
                        def = StringUtilities::StringReplace(def, '\"');

                        if(def == "0")
                        {
                            IGNORE_LINES = true;
                        }
                    }
                }
                else if(s_Type != ShaderType::UNKNOWN)
                {
                    shaders->at(s_Type).append(lines[i]);
                    ///Shaders->at(s_Type).append("\n");
                }
            }
        }

        uint32_t GLShader::Compile(std::map<ShaderType, std::string>* sources, GLShaderErrorInfo& info)
        {
            LUMOS_PROFILE_FUNCTION();
            GLCall(uint32_t program = glCreateProgram());

            std::vector<GLuint> shaders;

            std::string glVersion;

#ifndef LUMOS_PLATFORM_MOBILE
            glVersion = "#version 410 core \n";
#else
            glVersion = "#version 300 es \n precision highp float; \n precision highp int; \n";
#endif

            for(auto source : *sources)
            {
                //source.second.insert(0, glVersion);
                //LUMOS_LOG_INFO(source.second);
                shaders.push_back(CompileShader(source.first, source.second, program, info));
            }

            for(unsigned int shader : shaders)
                glAttachShader(program, shader);

            GLCall(glLinkProgram(program));

            GLint result;
            GLCall(glGetProgramiv(program, GL_LINK_STATUS, &result));
            if(result == GL_FALSE)
            {
                GLint length;
                GLCall(glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length));
                std::vector<char> error(length);
                GLCall(glGetProgramInfoLog(program, length, &length, error.data()));
                std::string errorMessage(error.data(), length);
                int32_t lineNumber = -1;
                sscanf(error.data(), "%*s %*d:%d", &lineNumber);
                info.shader = 3;
                info.message[info.shader] += "Failed to link shader!\n";
                info.line[info.shader] = 0;
                info.message[info.shader] += errorMessage;

                LUMOS_LOG_ERROR(info.message[info.shader]);
                return 0;
            }

            GLCall(glValidateProgram(program));

            for(int z = 0; z < shaders.size(); z++)
                glDetachShader(program, shaders[z]);

            for(int z = 0; z < shaders.size(); z++)
                glDeleteShader(shaders[z]);

            return program;
        }

        GLenum TypeToGL(ShaderType type)
        {
            switch(type)
            {
            case ShaderType::VERTEX:
                return GL_VERTEX_SHADER;
            case ShaderType::FRAGMENT:
                return GL_FRAGMENT_SHADER;
#ifndef LUMOS_PLATFORM_MOBILE
            case ShaderType::GEOMETRY:
                return GL_GEOMETRY_SHADER;
            case ShaderType::TESSELLATION_CONTROL:
                return GL_TESS_CONTROL_SHADER;
            case ShaderType::TESSELLATION_EVALUATION:
                return GL_TESS_EVALUATION_SHADER;
            case ShaderType::COMPUTE:
                return GL_COMPUTE_SHADER;
#endif
            default:
                LUMOS_LOG_ERROR("Unsupported Shader Type");
                return GL_VERTEX_SHADER;
            }
        }

        std::string TypeToString(ShaderType type)
        {
            switch(type)
            {
            case ShaderType::VERTEX:
                return "GL_VERTEX_SHADER";
            case ShaderType::FRAGMENT:
                return "GL_FRAGMENT_SHADER";
            case ShaderType::GEOMETRY:
                return "GL_GEOMETRY_SHADER";
            case ShaderType::TESSELLATION_CONTROL:
                return "GL_TESS_CONTROL_SHADER";
            case ShaderType::TESSELLATION_EVALUATION:
                return "GL_TESS_EVALUATION_SHADER";
            case ShaderType::COMPUTE:
                return "GL_COMPUTE_SHADER";
            case ShaderType::UNKNOWN:
                return "UNKOWN_SHADER";
            }
            return "N/A";
        }

        GLuint GLShader::CompileShader(ShaderType type, std::string source, uint32_t program, GLShaderErrorInfo& info)
        {
            LUMOS_PROFILE_FUNCTION();
            const char* cstr = source.c_str();

            GLCall(GLuint shader = glCreateShader(TypeToGL(type)));
            GLCall(glShaderSource(shader, 1, &cstr, NULL));
            GLCall(glCompileShader(shader));

            GLint result;
            GLCall(glGetShaderiv(shader, GL_COMPILE_STATUS, &result));
            if(result == GL_FALSE)
            {
                GLint length;
                GLCall(glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length));
                std::vector<char> error(length);
                GLCall(glGetShaderInfoLog(shader, length, &length, error.data()));
                std::string errorMessage(error.data(), length);
                int32_t lineNumber;
                sscanf(error.data(), "%*s %*d:%d", &lineNumber);
                info.shader = static_cast<uint32_t>(type);
                info.message[info.shader] += "Failed to compile " + TypeToString(type) + " shader!\n";

                info.line[info.shader] = lineNumber;
                info.message[info.shader] += errorMessage;
                GLCall(glDeleteShader(shader));

                LUMOS_LOG_ERROR(info.message[info.shader]);
                return 0;
            }
            return shader;
        }

        void GLShader::Bind() const
        {
            if(s_CurrentlyBound != this)
            {
                GLCall(glUseProgram(m_Handle));
                s_CurrentlyBound = this;
            }
        }

        void GLShader::Unbind() const
        {
            GLCall(glUseProgram(0));
            s_CurrentlyBound = nullptr;
        }

        bool GLShader::IsTypeStringResource(const std::string& type)
        {
            if(type == "sampler2D")
                return true;
            if(type == "samplerCube")
                return true;
            if(type == "sampler2DShadow")
                return true;
            if(type == "sampler2DArrayShadow")
                return true;
            return false;
        }

        GLint GLShader::GetUniformLocation(const std::string& name)
        {
            LUMOS_PROFILE_FUNCTION();
            GLuint hashName = HashValue(name.c_str());

            if(m_UniformLocations.find(hashName) == m_UniformLocations.end())
            {
                GLuint location = glGetUniformLocation(m_Handle, name.c_str());

                if(location != GL_INVALID_INDEX)
                {
                    m_UniformLocations[hashName] = location;
                }
                else
                {
                    LUMOS_LOG_WARN("Invalid uniform location {0}", name);
                }
            }

            auto location = m_UniformLocations[hashName];
            if(location == -1)
            {
                LUMOS_LOG_ERROR("Couldnt Find Uniform In Shader: {0}", name);
                return location;
            }

            return location;
        }

        void GLShader::SetUniform1f(const std::string& name, float value)
        {
            LUMOS_PROFILE_FUNCTION();
            SetUniform1f(GetUniformLocation(name), value);
        }

        void GLShader::SetUniform1fv(const std::string& name, float* value, int32_t count)
        {
            LUMOS_PROFILE_FUNCTION();
            SetUniform1fv(GetUniformLocation(name), value, count);
        }

        void GLShader::SetUniform1i(const std::string& name, int32_t value)
        {
            LUMOS_PROFILE_FUNCTION();
            SetUniform1i(GetUniformLocation(name), value);
        }

        void GLShader::SetUniform1ui(const std::string& name, uint32_t value)
        {
            LUMOS_PROFILE_FUNCTION();
            SetUniform1ui(GetUniformLocation(name), value);
        }

        void GLShader::SetUniform1iv(const std::string& name, int32_t* value, int32_t count)
        {
            LUMOS_PROFILE_FUNCTION();
            SetUniform1iv(GetUniformLocation(name), value, count);
        }

        void GLShader::SetUniform2f(const std::string& name, const Maths::Vector2& vector)
        {
            LUMOS_PROFILE_FUNCTION();
            SetUniform2f(GetUniformLocation(name), vector);
        }

        void GLShader::SetUniform3f(const std::string& name, const Maths::Vector3& vector)
        {
            LUMOS_PROFILE_FUNCTION();
            SetUniform3f(GetUniformLocation(name), vector);
        }

        void GLShader::SetUniform4f(const std::string& name, const Maths::Vector4& vector)
        {
            LUMOS_PROFILE_FUNCTION();
            SetUniform4f(GetUniformLocation(name), vector);
        }

        void GLShader::SetUniformMat4(const std::string& name, const Maths::Matrix4& matrix)
        {
            LUMOS_PROFILE_FUNCTION();
            SetUniformMat4(GetUniformLocation(name), matrix);
        }

        void GLShader::SetUniform1f(uint32_t location, float value)
        {
            GLCall(glUniform1f(location, value));
        }

        void GLShader::SetUniform1fv(uint32_t location, float* value, int32_t count)
        {
            GLCall(glUniform1fv(location, count, value));
        }

        void GLShader::SetUniform1i(uint32_t location, int32_t value)
        {
            GLCall(glUniform1i(location, value));
        }

        void GLShader::SetUniform1ui(uint32_t location, uint32_t value)
        {
            GLCall(glUniform1ui(location, value));
        }

        void GLShader::SetUniform1iv(uint32_t location, int32_t* value, int32_t count)
        {
            GLCall(glUniform1iv(location, count, value));
        }

        void GLShader::SetUniform2f(uint32_t location, const Maths::Vector2& vector)
        {
            GLCall(glUniform2f(location, vector.x, vector.y));
        }

        void GLShader::SetUniform3f(uint32_t location, const Maths::Vector3& vector)
        {
            GLCall(glUniform3f(location, vector.x, vector.y, vector.z));
        }

        void GLShader::SetUniform4f(uint32_t location, const Maths::Vector4& vector)
        {
            GLCall(glUniform4f(location, vector.x, vector.y, vector.z, vector.w));
        }

        void GLShader::SetUniformMat3(uint32_t location, const Maths::Matrix3& matrix)
        {
            GLCall(glUniformMatrix3fv(location, 1, GL_FALSE /*GLTRUE*/, Maths::ValuePointer(matrix))); // &matrix.values[0]));
        }

        void GLShader::SetUniformMat4(uint32_t location, const Maths::Matrix4& matrix)
        {
            GLCall(glUniformMatrix4fv(location, 1, GL_FALSE /*GLTRUE*/, Maths::ValuePointer(matrix)));
        }

        void GLShader::SetUniformMat4Array(uint32_t location, uint32_t count, const Maths::Matrix4& matrix)
        {
            GLCall(glUniformMatrix4fv(location, count, GL_FALSE /*GLTRUE*/, Maths::ValuePointer(matrix)));
        }

        Shader* GLShader::CreateFuncGL(const std::string& filePath)
        {
            std::string physicalPath;
            Lumos::VFS::Get()->ResolvePhysicalPath(filePath, physicalPath);
            GLShader* result = new GLShader(physicalPath);
            return result;
        }

        void GLShader::MakeDefault()
        {
            CreateFunc = CreateFuncGL;
        }
    }
}

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

        GLShader::GLShader(const std::string& filePath, bool loadSPV)
            : m_LoadSPV(loadSPV)
        {
            m_Name = StringUtilities::GetFileName(filePath);
            m_Path = StringUtilities::GetFileLocation(filePath);

            m_Source = VFS::Get()->ReadTextFile(filePath);

            Init();
        }

        GLShader::~GLShader()
        {
            Shutdown();

            for(auto& resource : m_Resources)
            {
                delete resource;
            }

            for(auto& structs : m_Structs)
            {
                delete structs;
            }

            for(auto& shader : m_UniformBuffers)
            {
                for(auto j : shader.second)
                {
                    delete j;
                }
            }

            for(auto& pc : m_PushConstants)
                delete[] pc.data;

            for(auto& shader : m_UserUniformBuffers)
            {
                delete shader.second;
            }
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
                        //                        GLVertexInputAttributeDescription Description = {};
                        //                        Description.binding  = glsl->get_decoration(resource.id, spv::DecorationBinding);
                        //                        Description.location = glsl->get_decoration(resource.id, spv::DecorationLocation);
                        //                        Description.offset   = stride;
                        //                        Description.format   = GetOpenGLFormat(InputType);
                        //                        m_VertexInputAttributeDescriptions.push_back(Description);

                        stride += GetStrideFromOpenGLFormat(0); //InputType.width * InputType.vecsize / 8;
                    }
                }

                // Get all sampled images in the shader.
                for(auto& resource : resources.sampled_images)
                {
                    unsigned set = glsl->get_decoration(resource.id, spv::DecorationDescriptorSet);
                    unsigned binding = glsl->get_decoration(resource.id, spv::DecorationBinding);

                    // Modify the decoration to prepare it for GLSL.
                    glsl->unset_decoration(resource.id, spv::DecorationDescriptorSet);

                    // Some arbitrary remapping if we want.
                    glsl->set_decoration(resource.id, spv::DecorationBinding, set * 16 + binding);
                }

                for(auto const& image : resources.separate_images)
                {
                    auto set { glsl->get_decoration(image.id, spv::Decoration::DecorationDescriptorSet) };
                    glsl->set_decoration(image.id, spv::Decoration::DecorationDescriptorSet, DESCRIPTOR_TABLE_INITIAL_SPACE + 2 * set);
                }
                for(auto const& input : resources.subpass_inputs)
                {
                    auto set { glsl->get_decoration(input.id, spv::Decoration::DecorationDescriptorSet) };
                    glsl->set_decoration(input.id, spv::Decoration::DecorationDescriptorSet, DESCRIPTOR_TABLE_INITIAL_SPACE + 2 * set);
                }
                for(auto const& uniform_buffer : resources.uniform_buffers)
                {
                    auto set { glsl->get_decoration(uniform_buffer.id, spv::Decoration::DecorationDescriptorSet) };
                    glsl->set_decoration(uniform_buffer.id, spv::Decoration::DecorationDescriptorSet, DESCRIPTOR_TABLE_INITIAL_SPACE + 2 * set);
                }
                for(auto const& storage_buffer : resources.storage_buffers)
                {
                    auto set { glsl->get_decoration(storage_buffer.id, spv::Decoration::DecorationDescriptorSet) };
                    glsl->set_decoration(storage_buffer.id, spv::Decoration::DecorationDescriptorSet, DESCRIPTOR_TABLE_INITIAL_SPACE + 2 * set);
                }
                for(auto const& storage_image : resources.storage_images)
                {
                    auto set { glsl->get_decoration(storage_image.id, spv::Decoration::DecorationDescriptorSet) };
                    glsl->set_decoration(storage_image.id, spv::Decoration::DecorationDescriptorSet, DESCRIPTOR_TABLE_INITIAL_SPACE + 2 * set);
                }
                for(auto const& image : resources.sampled_images)
                {
                    auto set { glsl->get_decoration(image.id, spv::Decoration::DecorationDescriptorSet) };
                    glsl->set_decoration(image.id, spv::Decoration::DecorationDescriptorSet, DESCRIPTOR_TABLE_INITIAL_SPACE + 2 * set); // Sampler offset done in spirv-cross
                }
                for(auto const& sampler : resources.separate_samplers)
                {
                    auto set { glsl->get_decoration(sampler.id, spv::Decoration::DecorationDescriptorSet) };
                    glsl->set_decoration(sampler.id, spv::Decoration::DecorationDescriptorSet, DESCRIPTOR_TABLE_INITIAL_SPACE + 2 * set + 1);
                }

                for(auto& u : resources.push_constant_buffers)
                {
                    uint32_t set = glsl->get_decoration(u.id, spv::DecorationDescriptorSet);
                    uint32_t binding = glsl->get_decoration(u.id, spv::DecorationBinding);

                    uint32_t binding3 = glsl->get_decoration(u.id, spv::DecorationOffset);

                    auto& type = glsl->get_type(u.type_id);

                    auto ranges = glsl->get_active_buffer_ranges(u.id);

                    uint32_t size = 0;
                    for(auto& range : ranges)
                    {
                        size += uint32_t(range.range);
                    }

                    m_PushConstants.push_back({ size, file.first });
                    m_PushConstants.back().data = new uint8_t[size];
                }

                spirv_cross::CompilerGLSL::Options options;
                options.version = 410;
                options.es = false;
                options.vulkan_semantics = false;
                options.separate_shader_objects = false;
                options.enable_420pack_extension = false;
                glsl->set_common_options(options);

                // Compile to GLSL, ready to give to GL driver.
                std::string glslSource = glsl->compile();
                file.second = glslSource;

                m_pShaderCompilers.push_back(glsl);
            }

            Parse(sources);

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

            //LUMOS_ASSERT(m_Handle, "");

            ResolveUniforms();
            ValidateUniforms();
            CreateLocations();

            delete sources;
        }

        void GLShader::Shutdown() const
        {
            LUMOS_PROFILE_FUNCTION();
            GLCall(glDeleteProgram(m_Handle));
        }

        void GLShader::BindPushConstants(Graphics::CommandBuffer* cmdBuffer, Graphics::Pipeline* pipeline)
        {
            LUMOS_PROFILE_FUNCTION();
            for(auto pc : m_PushConstants)
                SetUserUniformBuffer(pc.shaderStage, pc.data, pc.size);
        }

        bool GLShader::CreateLocations()
        {
            LUMOS_PROFILE_FUNCTION();
            for(auto& compiler : m_pShaderCompilers)
            {
                const spirv_cross::ShaderResources shaderResources = compiler->get_shader_resources();

                for(const auto& itUniform : shaderResources.uniform_buffers)
                {
                    if(compiler->get_type(itUniform.type_id).basetype == spirv_cross::SPIRType::Struct)
                    {
                        SetUniformLocation(itUniform.name.c_str());
                    }
                }
            }
            return true;
        }

        GLuint HashValue(const char* szString)
        {
            const char* c = szString;
            GLuint dwHashValue = 0x00000000;

            while(*c)
            {
                dwHashValue = (dwHashValue << 5) - dwHashValue + (*c == '/' ? '\\' : *c);
                c++;
            }

            return dwHashValue ? dwHashValue : 0xffffffff;
        }

        void GLShader::BindUniformBuffer(GLUniformBuffer* buffer, uint32_t slot, const std::string& name)
        {
            GLuint nameInt = HashValue(name.c_str());
            const auto& itLocation = m_uniformBlockLocations.find(nameInt); //

            //auto loc = glGetUniformBlockIndex(m_Handle, name.c_str());
            GLCall(glUniformBlockBinding(m_Handle, itLocation->second, slot));
        }

        bool GLShader::SetUniformLocation(const char* szName)
        {
            LUMOS_PROFILE_FUNCTION();
            GLuint name = HashValue(szName);

            if(m_uniformBlockLocations.find(name) == m_uniformBlockLocations.end())
            {
                GLuint location = glGetUniformBlockIndex(m_Handle, szName);

                if(location != GL_INVALID_INDEX)
                {
                    m_names[name] = szName;
                    m_uniformBlockLocations[name] = location;
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

        void GLShader::Parse(std::map<ShaderType, std::string>* sources)
        {
            for(auto& source : *sources)
            {
                m_UniformBuffers[source.first].push_back(new GLShaderUniformBufferDeclaration("Global", static_cast<uint32_t>(source.first)));

                const char* token;
                const char* str;

                str = source.second.c_str();
                while((token = StringUtilities::FindToken(str, "struct")))
                    ParseUniformStruct(StringUtilities::GetBlock(token, &str), source.first);

                str = source.second.c_str();
                while((token = StringUtilities::FindToken(str, "uniform")))
                    ParseUniform(StringUtilities::GetStatement(token, &str), source.first);
            }
        }

        void GLShader::ParseUniform(const std::string& statement, ShaderType type)
        {
            LUMOS_PROFILE_FUNCTION();
            std::vector<std::string> tokens = StringUtilities::Tokenize(statement);
            uint32_t index = 0;

            index++; // "uniform"
            const std::string typeString = tokens[index++];
            std::string name = tokens[index++];
            // Strip ; from name if present
            if(const char* s = strstr(name.c_str(), ";"))
                name = std::string(name.c_str(), s - name.c_str());

            std::string n(name);
            int32_t count = 1;
            const char* namestr = n.c_str();
            if(const char* s = strstr(namestr, "["))
            {
                name = std::string(namestr, s - namestr);

                const char* end = strstr(namestr, "]");
                std::string c(s + 1, end - s);
                count = atoi(c.c_str());
            }

            if(IsTypeStringResource(typeString))
            {
                ShaderResourceDeclaration* declaration = new GLShaderResourceDeclaration(GLShaderResourceDeclaration::StringToType(typeString), name, count);
                m_Resources.push_back(declaration);
            }
            else
            {

                if(count > 1)
                {
                    for(int countID = 0; countID < count; countID++)
                    {
                        const GLShaderUniformDeclaration::Type t = GLShaderUniformDeclaration::StringToType(typeString, count);
                        GLShaderUniformDeclaration* declaration;

                        name += "[" + StringUtilities::ToString(countID) + "]";
                        bool ISStruct = false;

                        if(t == GLShaderUniformDeclaration::Type::NONE)
                        {
                            ShaderStruct* s = FindStruct(typeString);
                            LUMOS_ASSERT(s, "");
                            declaration = new GLShaderUniformDeclaration(s, name, count);

                            ISStruct = true;
                        }
                        else
                        {
                            declaration = new GLShaderUniformDeclaration(t, name, count);
                        }

                        if(StringUtilities::StartsWith(name, "sys_"))
                        {
                            static_cast<GLShaderUniformBufferDeclaration*>(m_UniformBuffers[type].front())->PushUniform(declaration);
                        }
                        else
                        {

                            if(m_UserUniformBuffers[type] == nullptr)
                                m_UserUniformBuffers[type] = new GLShaderUniformBufferDeclaration("", 0);

                            if(ISStruct)
                            {
                                for(uint32_t id = 0; id < static_cast<uint32_t>(count); id++)
                                {
                                    GLShaderUniformDeclaration* test = new GLShaderUniformDeclaration(*declaration);
                                    test->SetName(name + "[" + StringUtilities::ToString(id) + "]");
                                    test->m_Struct = new ShaderStruct(*declaration->m_Struct);
                                    m_UserUniformBuffers[type]->PushUniform(test);
                                }
                            }
                            else
                            {
                                m_UserUniformBuffers[type]->PushUniform(declaration);
                            }
                        }

                        if(countID > 9)
                            name = name.substr(0, name.size() - 4);
                        else
                            name = name.substr(0, name.size() - 3);
                    }
                }
                else
                {
                    const GLShaderUniformDeclaration::Type t = GLShaderUniformDeclaration::StringToType(typeString, count);
                    GLShaderUniformDeclaration* declaration;

                    bool isStruct = false;
                    if(t == GLShaderUniformDeclaration::Type::NONE)
                    {
                        ///Shaderstruct* s = FindStruct(typeString);
                        //LUMOS_ASSERT(s, "");
                        //declaration = new GLShaderUniformDeclaration(s, name, count);

                        ShaderStruct* s = FindStruct(typeString);
                        if(s)
                        {
                            declaration = new GLShaderUniformDeclaration(s, name, count);

                            isStruct = true;
                        }
                        else
                        {
                            return;
                        }
                    }
                    else
                    {
                        declaration = new GLShaderUniformDeclaration(t, name, count);
                    }

                    if(StringUtilities::StartsWith(name, "sys_"))
                    {
                        static_cast<GLShaderUniformBufferDeclaration*>(m_UniformBuffers[type].front())->PushUniform(declaration);
                    }
                    else
                    {
                        if(m_UserUniformBuffers[type] == nullptr)
                            m_UserUniformBuffers[type] = new GLShaderUniformBufferDeclaration("", 0);

                        if(isStruct)
                        {
                            GLShaderUniformDeclaration* test = new GLShaderUniformDeclaration(*declaration);
                            test->SetName(name);
                            test->m_Struct = new ShaderStruct(*declaration->m_Struct);
                            m_UserUniformBuffers[type]->PushUniform(test);
                        }
                        else
                        {
                            m_UserUniformBuffers[type]->PushUniform(declaration);
                        }
                    }
                }
            }
        }

        void GLShader::ParseUniformStruct(const std::string& block, ShaderType shaderType)
        {
            std::vector<std::string> tokens = StringUtilities::Tokenize(block);

            uint32_t index = 0;
            index++; // struct
            std::string name = tokens[index++];
            ShaderStruct* uniformStruct = new ShaderStruct(name);
            index++; // {
            while(index < tokens.size())
            {
                if(tokens[index] == "}")
                    break;

                const std::string type = tokens[index++];
                name = tokens[index++];

                // Strip ; from name if present
                if(const char* s = strstr(name.c_str(), ";"))
                    name = std::string(name.c_str(), s - name.c_str());

                uint32_t count = 1;
                const char* namestr = name.c_str();
                if(const char* s = strstr(namestr, "["))
                {
                    name = std::string(namestr, s - namestr);

                    const char* end = strstr(namestr, "]");
                    std::string c(s + 1, end - s);
                    count = atoi(c.c_str());
                }

                ShaderUniformDeclaration* field = new GLShaderUniformDeclaration(GLShaderUniformDeclaration::StringToType(type, count), name, count);
                uniformStruct->AddField(field);
            }
            m_Structs.push_back(uniformStruct);
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

        void GLShader::ResolveUniforms()
        {
            LUMOS_PROFILE_FUNCTION();
            Bind();

            for(auto shader : m_UniformBuffers)
            {
                for(auto j : shader.second)
                {
                    GLShaderUniformBufferDeclaration* decl = static_cast<GLShaderUniformBufferDeclaration*>(j);
                    const ShaderUniformList& uniforms = decl->GetUniformDeclarations();
                    for(uint32_t k = 0; k < uniforms.size(); k++)
                    {
                        GLShaderUniformDeclaration* uniform = static_cast<GLShaderUniformDeclaration*>(uniforms[k]);
                        if(uniform->GetType() == GLShaderUniformDeclaration::Type::STRUCT)
                        {
                            const ShaderStruct& s = uniform->GetShaderUniformStruct();
                            const auto& fields = s.GetFields();
                            if(uniform->GetCount() == 1)
                            {
                                for(uint32_t l = 0; l < fields.size(); l++)
                                {
                                    GLShaderUniformDeclaration* field = static_cast<GLShaderUniformDeclaration*>(fields[l]);
                                    field->m_Location = GetUniformLocation(uniform->m_Name + "." + field->m_Name);
                                }
                            }
                            else
                            {
                                for(uint32_t i = 0; i < uniform->GetCount(); i++)
                                {
                                    for(uint32_t l = 0; l < fields.size(); l++)
                                    {
                                        GLShaderUniformDeclaration* field = static_cast<GLShaderUniformDeclaration*>(fields[l]);
                                        field->m_Location = GetUniformLocation(uniform->m_Name + "[" + std::to_string(i) + "]" + "." + field->m_Name);
                                    }
                                }
                            }
                        }
                        else
                        {
                            uniform->m_Location = GetUniformLocation(uniform->m_Name);
                        }
                    }
                }
            }

            for(const auto& shader : m_UserUniformBuffers)
            {
                GLShaderUniformBufferDeclaration* decl = shader.second;
                if(decl)
                {
                    const ShaderUniformList& uniforms = decl->GetUniformDeclarations();
                    for(uint32_t j = 0; j < uniforms.size(); j++)
                    {
                        GLShaderUniformDeclaration* uniform = static_cast<GLShaderUniformDeclaration*>(uniforms[j]);
                        if(uniform->GetType() == GLShaderUniformDeclaration::Type::STRUCT)
                        {
                            const ShaderStruct& s = uniform->GetShaderUniformStruct();
                            const auto& fields = s.GetFields();

                            if(uniform->GetCount() == 1)
                            {
                                for(uint32_t k = 0; k < fields.size(); k++)
                                {
                                    GLShaderUniformDeclaration* field = static_cast<GLShaderUniformDeclaration*>(fields[k]);
                                    field->m_Location = GetUniformLocation(uniform->m_Name + "." + field->m_Name);
                                }
                            }
                            else
                            {
                                for(uint32_t i = 0; i < uniform->GetCount(); i++)
                                {
                                    for(uint32_t k = 0; k < fields.size(); k++)
                                    {
                                        GLShaderUniformDeclaration* field = static_cast<GLShaderUniformDeclaration*>(fields[k]);
                                        field->m_Location = GetUniformLocation(uniform->m_Name + "[" + std::to_string(i) + "]" + "." + field->m_Name);
                                    }
                                }
                            }
                        }
                        else
                        {
                            uniform->m_Location = GetUniformLocation(uniform->m_Name);
                        }
                    }
                }
            }

            uint32_t sampler = 0;
            for(uint32_t i = 0; i < m_Resources.size(); i++)
            {
                GLShaderResourceDeclaration* resource = static_cast<GLShaderResourceDeclaration*>(m_Resources[i]);
                uint32_t location = GetUniformLocation(resource->m_Name);
                if(resource->GetCount() == 1)
                {
                    resource->m_Register = sampler;
                    SetUniform1i(location, sampler++);
                }
                else if(resource->GetCount() > 1)
                {
                    resource->m_Register = 0;
                    uint32_t count = resource->GetCount();
                    int32_t* samplers = new int32_t[count];
                    for(uint32_t s = 0; s < count; s++)
                        samplers[s] = s;
                    SetUniform1iv(resource->GetName(), samplers, count);
                    delete[] samplers;
                }
            }
            Unbind();
        }

        void GLShader::ValidateUniforms()
        {
        }

        bool GLShader::IsSystemUniform(ShaderUniformDeclaration* uniform)
        {
            LUMOS_PROFILE_FUNCTION();
            return StringUtilities::StartsWith(uniform->GetName(), "sys_");
        }

        GLint GLShader::GetUniformLocation(const std::string& name) const
        {
            LUMOS_PROFILE_FUNCTION();
            GLCall(const GLint result = glGetUniformLocation(m_Handle, name.c_str()));
            //if (result == -1)
            //	LUMOS_LOG_WARN("{0} : could not find uniform {1} in shader!",m_Name,name);

            return result;
        }

        void GLShader::SetUniformStruct(GLShaderUniformDeclaration* uniform, uint8_t* data, int32_t offset) const
        {
            LUMOS_PROFILE_FUNCTION();
            const ShaderStruct& s = uniform->GetShaderUniformStruct();
            const auto& fields = s.GetFields();
            for(uint32_t k = 0; k < fields.size(); k++)
            {
                GLShaderUniformDeclaration* field = static_cast<GLShaderUniformDeclaration*>(fields[k]);
                ResolveAndSetUniformField(*field, data, offset, uniform->GetCount());
                offset += field->m_Size;
            }
        }

        ShaderUniformDeclaration* GLShader::FindUniformDeclaration(const std::string& name, const ShaderUniformBufferDeclaration* buffer)
        {
            LUMOS_PROFILE_FUNCTION();
            const ShaderUniformList& uniforms = buffer->GetUniformDeclarations();
            for(uint32_t i = 0; i < uniforms.size(); i++)
            {
                if(uniforms[i]->GetName() == name)
                    return uniforms[i];
            }
            return nullptr;
        }

        ShaderUniformDeclaration* GLShader::FindUniformDeclaration(const std::string& name)
        {
            LUMOS_PROFILE_FUNCTION();
            ShaderUniformDeclaration* result = nullptr;

            for(auto shader : m_UniformBuffers)
            {
                for(uint32_t i = 0; i < shader.second.size(); i++)
                {
                    result = FindUniformDeclaration(name, shader.second[i]);
                    if(result)
                        return result;
                }
            }

            for(const auto& shader : m_UserUniformBuffers)
            {
                result = FindUniformDeclaration(name, shader.second);
                if(result)
                    return result;
            }

            return result;
        }

        void GLShader::SetUserUniformBuffer(ShaderType type, uint8_t* data, uint32_t size)
        {
            LUMOS_PROFILE_FUNCTION();
            ResolveAndSetUniforms(m_UserUniformBuffers[type], data, size);
        }

        ShaderStruct* GLShader::FindStruct(const std::string& name)
        {
            LUMOS_PROFILE_FUNCTION();
            for(ShaderStruct* s : m_Structs)
            {
                if(s->GetName() == name)
                    return s;
            }
            return nullptr;
        }

        void GLShader::ResolveAndSetUniforms(ShaderUniformBufferDeclaration* buffer, uint8_t* data, uint32_t size) const
        {
            LUMOS_PROFILE_FUNCTION();
            const ShaderUniformList& uniforms = buffer->GetUniformDeclarations();
            for(uint32_t i = 0; i < uniforms.size(); i++)
            {
                GLShaderUniformDeclaration* uniform = static_cast<GLShaderUniformDeclaration*>(uniforms[i]);

                ResolveAndSetUniform(uniform, data, size, uniform->GetCount());
            }
        }

        void GLShader::ResolveAndSetUniform(GLShaderUniformDeclaration* uniform, uint8_t* data, uint32_t size, uint32_t count) const
        {
            LUMOS_PROFILE_FUNCTION();
            if(uniform->GetLocation() == -1)
            {
                //LUMOS_LOG_ERROR( "Couldnt Find Uniform In Shader: " + uniform->GetName());
                return;
            }

            const uint32_t offset = uniform->GetOffset();
            switch(uniform->GetType())
            {
            case GLShaderUniformDeclaration::Type::FLOAT32:
                SetUniform1f(uniform->GetLocation(), *reinterpret_cast<float*>(&data[offset]));
                break;
            case GLShaderUniformDeclaration::Type::INT32:
                SetUniform1i(uniform->GetLocation(), *reinterpret_cast<int32_t*>(&data[offset]));
                break;
            case GLShaderUniformDeclaration::Type::UINT:
                SetUniform1ui(uniform->GetLocation(), *reinterpret_cast<uint32_t*>(&data[offset]));
                break;
            case GLShaderUniformDeclaration::Type::INT:
                SetUniform1i(uniform->GetLocation(), *reinterpret_cast<int*>(&data[offset]));
                break;
            case GLShaderUniformDeclaration::Type::VEC2:
                SetUniform2f(uniform->GetLocation(), *reinterpret_cast<Maths::Vector2*>(&data[offset]));
                break;
            case GLShaderUniformDeclaration::Type::VEC3:
                SetUniform3f(uniform->GetLocation(), *reinterpret_cast<Maths::Vector3*>(&data[offset]));
                break;
            case GLShaderUniformDeclaration::Type::VEC4:
                SetUniform4f(uniform->GetLocation(), *reinterpret_cast<Maths::Vector4*>(&data[offset]));
                break;
            case GLShaderUniformDeclaration::Type::MAT3:
                SetUniformMat3(uniform->GetLocation(), *reinterpret_cast<Maths::Matrix3*>(&data[offset]));
                break;
            case GLShaderUniformDeclaration::Type::MAT4:
                SetUniformMat4(uniform->GetLocation(), *reinterpret_cast<Maths::Matrix4*>(&data[offset]));
                break;
            case GLShaderUniformDeclaration::Type::MAT4ARRAY:
                SetUniformMat4Array(uniform->GetLocation(), count, *reinterpret_cast<Maths::Matrix4*>(&data[offset]));
                break;
            case GLShaderUniformDeclaration::Type::STRUCT:
                SetUniformStruct(uniform, data, offset);
                break;
            default:
                LUMOS_ASSERT(false, "Unknown type!");
            }
        }

        void GLShader::SetUniform(const std::string& name, uint8_t* data)
        {
            LUMOS_PROFILE_FUNCTION();
            ShaderUniformDeclaration* uniform = FindUniformDeclaration(name);
            if(!uniform)
            {
                LUMOS_LOG_ERROR("Cannot find uniform in {0} shader with name '{1}'", m_Name, name);
                return;
            }
            ResolveAndSetUniform(static_cast<GLShaderUniformDeclaration*>(uniform), data, 0, uniform->GetCount());
        }

        void GLShader::ResolveAndSetUniformField(const GLShaderUniformDeclaration& field, uint8_t* data, int32_t offset, uint32_t count) const
        {
            LUMOS_PROFILE_FUNCTION();
            //LUMOS_ASSERT(field.GetLocation() < 0, "Couldnt Find Uniform In Shader: " + field.GetName());

            switch(field.GetType())
            {
            case GLShaderUniformDeclaration::Type::FLOAT32:
                SetUniform1f(field.GetLocation(), *reinterpret_cast<float*>(&data[offset]));
                break;
            case GLShaderUniformDeclaration::Type::INT32:
                SetUniform1i(field.GetLocation(), *reinterpret_cast<int32_t*>(&data[offset]));
                break;
            case GLShaderUniformDeclaration::Type::UINT:
                SetUniform1ui(field.GetLocation(), *reinterpret_cast<uint32_t*>(&data[offset]));
                break;
            case GLShaderUniformDeclaration::Type::VEC2:
                SetUniform2f(field.GetLocation(), *reinterpret_cast<Maths::Vector2*>(&data[offset]));
                break;
            case GLShaderUniformDeclaration::Type::VEC3:
                SetUniform3f(field.GetLocation(), *reinterpret_cast<Maths::Vector3*>(&data[offset]));
                break;
            case GLShaderUniformDeclaration::Type::VEC4:
                SetUniform4f(field.GetLocation(), *reinterpret_cast<Maths::Vector4*>(&data[offset]));
                break;
            case GLShaderUniformDeclaration::Type::MAT3:
                SetUniformMat3(field.GetLocation(), *reinterpret_cast<Maths::Matrix3*>(&data[offset]));
                break;
            case GLShaderUniformDeclaration::Type::MAT4:
                SetUniformMat4(field.GetLocation(), *reinterpret_cast<Maths::Matrix4*>(&data[offset]));
                break;
            case GLShaderUniformDeclaration::Type::MAT4ARRAY:
                SetUniformMat4Array(field.GetLocation(), count, *reinterpret_cast<Maths::Matrix4*>(&data[offset]));
                break;
            default:
                LUMOS_ASSERT(false, "Unknown type!");
            }
        }

        void GLShader::SetUniform1f(const std::string& name, float value) const
        {
            LUMOS_PROFILE_FUNCTION();
            SetUniform1f(GetUniformLocation(name), value);
        }

        void GLShader::SetUniform1fv(const std::string& name, float* value, int32_t count) const
        {
            LUMOS_PROFILE_FUNCTION();
            SetUniform1fv(GetUniformLocation(name), value, count);
        }

        void GLShader::SetUniform1i(const std::string& name, int32_t value) const
        {
            LUMOS_PROFILE_FUNCTION();
            SetUniform1i(GetUniformLocation(name), value);
        }

        void GLShader::SetUniform1ui(const std::string& name, uint32_t value) const
        {
            LUMOS_PROFILE_FUNCTION();
            SetUniform1ui(GetUniformLocation(name), value);
        }

        void GLShader::SetUniform1iv(const std::string& name, int32_t* value, int32_t count) const
        {
            LUMOS_PROFILE_FUNCTION();
            SetUniform1iv(GetUniformLocation(name), value, count);
        }

        void GLShader::SetUniform2f(const std::string& name, const Maths::Vector2& vector) const
        {
            LUMOS_PROFILE_FUNCTION();
            SetUniform2f(GetUniformLocation(name), vector);
        }

        void GLShader::SetUniform3f(const std::string& name, const Maths::Vector3& vector) const
        {
            LUMOS_PROFILE_FUNCTION();
            SetUniform3f(GetUniformLocation(name), vector);
        }

        void GLShader::SetUniform4f(const std::string& name, const Maths::Vector4& vector) const
        {
            LUMOS_PROFILE_FUNCTION();
            SetUniform4f(GetUniformLocation(name), vector);
        }

        void GLShader::SetUniformMat4(const std::string& name, const Maths::Matrix4& matrix) const
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
            result->m_Path = filePath;
            return result;
        }

        void GLShader::MakeDefault()
        {
            CreateFunc = CreateFuncGL;
        }
    }
}
